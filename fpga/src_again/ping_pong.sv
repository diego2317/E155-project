// ============================================================================
// Ping-Pong Frame Buffer using iCE40UP5K SPRAM (SB_SPRAM256KA)
// - Same clock domain as OV7670 capture (cam_pclk)
// - Assumes bit-masked pixels (1 bit per pixel) from capture module
// - Packs 16 pixels into one 16-bit SPRAM word
// - Alternates (ping-pong) between two SPRAM blocks per frame
//   * While writing into one buffer, the other buffer is considered "readable"
//   * Exposes which buffer holds the most recent complete frame
//   * Provides a read port (rd_addr/rd_data) for downstream SPI logic
// ============================================================================

module ping_pong #(
    // Resolution and pixel format
    parameter int H_PIXELS    = 320,  // horizontal luma samples
    parameter int V_LINES     = 240,  // vertical lines
    parameter int PIXEL_BITS  = 1,    // bit-masked pixel width (must divide 16)
    parameter int ADDR_WIDTH  = 14    // enough to address all words in buffer
)(
    input  logic               cam_pclk,     // same clock as capture module (PCLK)
    input  logic               rst_n,        // active-low synchronous reset

    // From pixel capture / threshold module
    input  logic               pixel_valid,  // 1 when pixel_bit is valid
    input  logic               pixel_bit,    // bit-masked pixel (e.g., thresholded Y)

    // Frame-complete indication and buffer selection
    output logic               frame_done,   // 1-cycle pulse when a frame has just been written
    output logic               read_buf_sel, // buffer index holding latest complete frame (0 or 1)

    // Read side (for SPI or other consumer) reads from the "read" buffer
    input  logic [ADDR_WIDTH-1:0] rd_addr,   // word address into frame buffer
    output logic [15:0]           rd_data    // 16-bit word from selected read buffer
);

    // ------------------------------------------------------------------------
    // Derived constants
    // ------------------------------------------------------------------------
    localparam int FRAME_PIXELS    = H_PIXELS * V_LINES; // total pixels per frame
    localparam int WORD_BITS       = 16;
    localparam int PIXELS_PER_WORD = WORD_BITS / PIXEL_BITS;

    localparam int PIXCNT_WIDTH    = $clog2(FRAME_PIXELS);
    localparam int BITIDX_WIDTH    = $clog2(PIXELS_PER_WORD);

    // ------------------------------------------------------------------------
    // State registers
    // ------------------------------------------------------------------------
    logic [PIXCNT_WIDTH-1:0] pixel_count_q, pixel_count_d; // pixels written in current frame
    logic [BITIDX_WIDTH-1:0] bit_index_q,  bit_index_d;    // bit index inside 16-bit word
    logic [ADDR_WIDTH-1:0]   wr_addr_q,    wr_addr_d;      // word address inside current buffer
    logic [15:0]             wr_word_q,    wr_word_d;      // word being packed
    logic                    wr_buf_sel_q, wr_buf_sel_d;   // 0 = buffer0 write, 1 = buffer1 write
    logic                    frame_done_q, frame_done_d;   // registered frame_done pulse

    // "Read" buffer is always the one not currently being written
    // (latest completed frame)
    assign read_buf_sel = ~wr_buf_sel_q;
    assign frame_done   = frame_done_q;

    // ------------------------------------------------------------------------
    // Next-state / write control
    // ------------------------------------------------------------------------
    logic is_last_pixel;
    logic flush_word;      // write current packed word into SPRAM this cycle

    always_comb begin
        // Default: hold state
        pixel_count_d = pixel_count_q;
        bit_index_d   = bit_index_q;
        wr_addr_d     = wr_addr_q;
        wr_word_d     = wr_word_q;
        wr_buf_sel_d  = wr_buf_sel_q;
        frame_done_d  = 1'b0;

        is_last_pixel = (pixel_count_q == FRAME_PIXELS-1);
        flush_word    = 1'b0;

        if (pixel_valid) begin
            // Pack incoming bit into current word at bit_index_q
            wr_word_d                = wr_word_q;
            wr_word_d[bit_index_q]   = pixel_bit;

            // Decide if this word should be flushed to SPRAM:
            //  - when we fill all PIXELS_PER_WORD bits, or
            //  - on the very last pixel of the frame (partial final word)
            flush_word = (bit_index_q == (PIXELS_PER_WORD-1)) || is_last_pixel;

            if (is_last_pixel) begin
                // End of frame: reset counters and toggle write buffer
                pixel_count_d = '0;
                bit_index_d   = '0;
                wr_addr_d     = '0;
                wr_buf_sel_d  = ~wr_buf_sel_q;
                frame_done_d  = 1'b1;
            end else begin
                // Continue within frame
                pixel_count_d = pixel_count_q + 1'b1;

                if (bit_index_q == (PIXELS_PER_WORD-1)) begin
                    bit_index_d = '0;
                    wr_addr_d   = wr_addr_q + 1'b1;
                end else begin
                    bit_index_d = bit_index_q + 1'b1;
                end
            end
        end
    end

    // ------------------------------------------------------------------------
    // Sequential state update (all state synchronous to cam_pclk)
    // ------------------------------------------------------------------------
    always_ff @(posedge cam_pclk) begin
        if (!rst_n) begin
            pixel_count_q <= '0;
            bit_index_q   <= '0;
            wr_addr_q     <= '0;
            wr_word_q     <= '0;
            wr_buf_sel_q  <= 1'b0;
            frame_done_q  <= 1'b0;
        end else begin
            pixel_count_q <= pixel_count_d;
            bit_index_q   <= bit_index_d;
            wr_addr_q     <= wr_addr_d;
            wr_word_q     <= wr_word_d;
            wr_buf_sel_q  <= wr_buf_sel_d;
            frame_done_q  <= frame_done_d;
        end
    end

    // ------------------------------------------------------------------------
    // SPRAM write interface (two SB_SPRAM256KA blocks, ping-pong by wr_buf_sel_q)
    // ------------------------------------------------------------------------
    // Zero-extend write and read addresses to 14 bits required by SPRAM.
    wire [13:0] wr_addr_ext = {{(14-ADDR_WIDTH){1'b0}}, wr_addr_q};
    wire [13:0] rd_addr_ext = {{(14-ADDR_WIDTH){1'b0}}, rd_addr};

    // Address for each SPRAM:
    //  - The buffer being written uses wr_addr_ext
    //  - The other buffer uses rd_addr_ext for reading
    wire [13:0] addr0 = (wr_buf_sel_q == 1'b0) ? wr_addr_ext : rd_addr_ext;
    wire [13:0] addr1 = (wr_buf_sel_q == 1'b1) ? wr_addr_ext : rd_addr_ext;

    // Shared data input: packed word
    wire [15:0] spram_datain = wr_word_q;

    // Write enables per buffer (only one buffer written per frame)
    wire wren_buf0 = flush_word && (wr_buf_sel_q == 1'b0);
    wire wren_buf1 = flush_word && (wr_buf_sel_q == 1'b1);

    // SPRAM outputs
    logic [15:0] spram_dout0, spram_dout1;

    // Select read data from the "read" buffer
    always_comb begin
        rd_data = (read_buf_sel == 1'b0) ? spram_dout0 : spram_dout1;
    end

    // ------------------------------------------------------------------------
    // SPRAM instance 0 (buffer 0)
    // ------------------------------------------------------------------------
     SP256K spram_buf0 (
		.AD(addr0),
		.DI(spram_datain),
        .MASKWE(4'b1111),         // Write all bytes
        .WE(wren_buf0),
        .CS(1'b1),
        .CK(cam_pclk),            // All SPRAM ops on sys_clk
        .STDBY(1'b0),
        .SLEEP(1'b0),
        .PWROFF_N(1'b1),
        .DO(spram_dout0)
    );
   
    SP256K spram_buf1 (
	    .AD(addr1),
	    .DI(spram_datain),
        .MASKWE(4'b1111),
        .WE(wren_buf1),
        .CS(1'b1),
        .CK(cam_pclk),
        .STDBY(1'b0),
        .SLEEP(1'b0),
        .PWROFF_N(1'b1),
        .DO(spram_dout1)
    );

endmodule
