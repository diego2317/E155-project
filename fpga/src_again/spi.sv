// ============================================================================
// SPI Slave Frame Transmitter for Ping-Pong SPRAM Buffer
// Target: Lattice iCE40UP5K
// ----------------------------------------------------------------------------
// - Streams a full frame buffer over SPI as a slave, transmit-only
// - Shares clock domain with ping-pong SPRAM FSM (e.g., cam_pclk or sysclk)
// - Handshake:
//      * ping-pong asserts frame_ready with buf_sel (0/1) when a buffer is full
//      * this module raises frame_busy while streaming that buffer
// - Memory interface is abstract: rd_addr/rd_data connect to the selected
//   SPRAM buffer via buf_sel at the top level.
// - SPI: mode-0 style (CPOL=0, CPHA=0 assumption). Bits shifted out MSB-first.
//   SCK and CS are sampled and edge-detected in this clock domain.
// ============================================================================

module spi #(
    parameter int ADDR_WIDTH  = 14,     // enough to address all words in buffer
    parameter int FRAME_WORDS = 10240   // number of 16-bit words per frame
)(
    input  logic                  clk,          // same domain as ping-pong FSM
    input  logic                  rst_n,

    // Handshake with ping-pong SPRAM FSM
    input  logic                  frame_ready,  // 1-cycle pulse: new frame available
    input  logic                  buf_sel,      // which buffer (0/1) holds the frame
    output logic                  frame_busy,   // high while this frame is being sent

    // Abstract read interface to selected buffer
    output logic [ADDR_WIDTH-1:0] rd_addr,      // word address into frame buffer
    input  logic [15:0]           rd_data,      // 16-bit word at rd_addr (externally routed)

    // SPI slave interface (transmit-only)
    input  logic                  spi_sck,      // SPI clock from master
    input  logic                  spi_ncs,      // active-low chip select
    output logic                  spi_miso      // data out to master
);

    // ------------------------------------------------------------------------
    // State machine for frame streaming
    // ------------------------------------------------------------------------
    typedef enum logic [1:0] {
        ST_IDLE,        // no frame pending
        ST_WAIT_CS,     // frame ready, waiting for CS low
        ST_STREAM       // streaming bits over SPI
    } state_t;

    state_t state_q, state_d;

    // ------------------------------------------------------------------------
    // SPI signal edge detection (all sampled in clk domain)
    // ------------------------------------------------------------------------
    logic sck_q, sck_qq;   // SCK synchronizer
    logic cs_q, cs_qq;     // CS synchronizer

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            sck_q  <= 1'b0;
            sck_qq <= 1'b0;
            cs_q   <= 1'b1;
            cs_qq  <= 1'b1;
        end else begin
            sck_qq <= sck_q;
            sck_q  <= spi_sck;

            cs_qq  <= cs_q;
            cs_q   <= spi_ncs;
        end
    end

    wire sck_rise = ( sck_q & ~sck_qq);      // SCK rising edge in clk domain
    wire cs_fall  = (~cs_q &  cs_qq);        // CS falling edge
    wire cs_high  = cs_q;                    // CS deasserted

    // ------------------------------------------------------------------------
    // Frame / buffer bookkeeping
    // ------------------------------------------------------------------------
    logic frame_busy_q, frame_busy_d;
    logic buf_sel_q;            // latched buffer select for this frame
    logic [ADDR_WIDTH-1:0] word_addr_q, word_addr_d;

    // Per-word/byte/bit transmit counters
    logic        byte_sel_q, byte_sel_d;     // 0 = upper byte (15:8), 1 = lower byte (7:0)
    logic [2:0]  bit_cnt_q,  bit_cnt_d;      // bit index within byte (7..0)
    logic [7:0]  shift_q,    shift_d;        // shift register for SPI

    // Drive rd_addr from word_addr
    assign rd_addr   = word_addr_q;
    assign frame_busy = frame_busy_q;

    // MISO outputs MSB of shift register
    assign spi_miso  = shift_q[7];

    // ------------------------------------------------------------------------
    // Next-state logic
    // ------------------------------------------------------------------------
    always_comb begin
        // Defaults: hold state
        state_d       = state_q;
        frame_busy_d  = frame_busy_q;
        buf_sel_q     = buf_sel_q;      // self-assignment OK in comb
        word_addr_d   = word_addr_q;
        byte_sel_d    = byte_sel_q;
        bit_cnt_d     = bit_cnt_q;
        shift_d       = shift_q;

        // Accept a new frame when idle and frame_ready pulses
        if (state_q == ST_IDLE && frame_ready) begin
            frame_busy_d = 1'b1;
            buf_sel_q    = buf_sel;     // latch which buffer we will read
            word_addr_d  = '0;
            byte_sel_d   = 1'b0;        // start with upper byte (15:8)
            state_d      = ST_WAIT_CS;
        end

        unique case (state_q)
            ST_IDLE: begin
                // Nothing else to do; wait for frame_ready
            end

            ST_WAIT_CS: begin
                // Wait for CS to go low; pre-load first byte when CS falls
                if (!cs_high && cs_fall) begin
                    // Load first byte of first word into shift register
                    shift_d    = rd_data[15:8];  // MSB-first
                    bit_cnt_d  = 3'd7;
                    byte_sel_d = 1'b0;
                    state_d    = ST_STREAM;
                end
            end

            ST_STREAM: begin
                // If CS goes high, stop streaming and return to idle
                if (cs_high) begin
                    state_d      = ST_IDLE;
                    frame_busy_d = 1'b0;
                end else begin
                    // Only change data on SCK rising edges while CS is low
                    if (sck_rise) begin
                        // Shift out next bit (MISO is shift_q[7])
                        shift_d = {shift_q[6:0], 1'b0};

                        if (bit_cnt_q != 3'd0) begin
                            bit_cnt_d = bit_cnt_q - 3'd1;
                        end else begin
                            // Finished current byte; prepare next
                            if (byte_sel_q == 1'b0) begin
                                // Move to lower byte of the same word
                                byte_sel_d = 1'b1;
                                bit_cnt_d  = 3'd7;
                                shift_d    = rd_data[7:0];
                            end else begin
                                // Finished both bytes of this word
                                if (word_addr_q == FRAME_WORDS-1) begin
                                    // Entire frame sent
                                    state_d      = ST_IDLE;
                                    frame_busy_d = 1'b0;
                                end else begin
                                    // Advance to next word
                                    word_addr_d  = word_addr_q + 1'b1;
                                    byte_sel_d   = 1'b0;
                                    bit_cnt_d    = 3'd7;
                                    // rd_data is assumed to reflect new word_addr_d
                                    shift_d      = rd_data[15:8];
                                end
                            end
                        end
                    end
                end
            end

            default: begin
                state_d = ST_IDLE;
            end
        endcase
    end

    // ------------------------------------------------------------------------
    // Sequential updates
    // ------------------------------------------------------------------------
    always_ff @(posedge clk) begin
        if (!rst_n) begin
            state_q      <= ST_IDLE;
            frame_busy_q <= 1'b0;
            buf_sel_q    <= 1'b0;
            word_addr_q  <= '0;
            byte_sel_q   <= 1'b0;
            bit_cnt_q    <= 3'd0;
            shift_q      <= 8'd0;
        end else begin
            state_q      <= state_d;
            frame_busy_q <= frame_busy_d;
            buf_sel_q    <= buf_sel_q;   // latched when frame_ready accepted
            word_addr_q  <= word_addr_d;
            byte_sel_q   <= byte_sel_d;
            bit_cnt_q    <= bit_cnt_d;
            shift_q      <= shift_d;
        end
    end

endmodule
