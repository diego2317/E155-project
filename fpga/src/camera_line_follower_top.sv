// ============================================================================
// Camera Line Follower - Complete FPGA Design
// For iCE40UP5K (UPduino v3.0)
// ============================================================================

// ============================================================================
// Camera Line Follower - FPGA Design (Fixed Threshold + Internal HSOSC)
// Target: iCE40UP5K (UPduino v3)
// ============================================================================

module camera_line_follower_top (
    // System
    input  wire nreset,
    
    // Camera Interface (OV7670)
    input  wire cam_pclk,           // Pixel clock from camera
    input  wire cam_vsync,          // Frame sync
    input  wire cam_href,           // Line valid
    input  wire [7:0] cam_data,     // Pixel data D[7:0]
    
    // SPI Interface (to MCU)
    input  wire spi_sck,            // SPI clock from MCU
    input  wire spi_mosi,           // Data from MCU (unused)
    output wire spi_miso,           // Data to MCU
    input  wire spi_ncs,            // Chip select (active low)
    
    // Status outputs
    output wire led_red,            // Debug LED
    output wire led_green,
    output wire led_blue
);

    // ------------------------------------------------------------------------
    // Internal high-speed oscillator (48 MHz nominal)
    // ------------------------------------------------------------------------
    wire clk_48mhz;

    HSOSC #(
        .CLKHF_DIV(2'b00)    // 00 = 48 MHz
    ) hf_osc (
        .CLKHFPU(1'b1),
        .CLKHFEN(1'b1),
        .CLKHF(clk_48mhz)
    );

    // ------------------------------------------------------------------------
    // Camera capture interface
    // ------------------------------------------------------------------------
    wire [16:0] cam_wr_addr;
    wire        cam_wr_data;
    wire        cam_wr_en;
    wire        cam_frame_done;

    // ------------------------------------------------------------------------
    // SPRAM buffer and SPI interface signals
    // ------------------------------------------------------------------------
    wire [16:0] spram_rd_addr;
    wire [7:0]  spram_rd_data;
    wire        buffer_ready;
    wire        frame_read_complete;

    // ------------------------------------------------------------------------
    // Fixed threshold value (0–255)
    // ------------------------------------------------------------------------
    localparam [7:0] THRESHOLD = 8'd128;  // adjustable cutoff

    // ------------------------------------------------------------------------
    // Camera capture with fixed threshold
    // ------------------------------------------------------------------------
    camera_capture_threshold cam_cap (
        .clk(clk_48mhz),
        .nreset(nreset),
        .cam_pclk(cam_pclk),
        .cam_vsync(cam_vsync),
        .cam_href(cam_href),
        .cam_data(cam_data),
        .threshold(THRESHOLD),
        .wr_addr(cam_wr_addr),
        .wr_data(cam_wr_data),
        .wr_en(cam_wr_en),
        .frame_done(cam_frame_done)
    );

    // ------------------------------------------------------------------------
    // Ping-pong SPRAM buffer (SP256K version)
    // ------------------------------------------------------------------------
    spram_pingpong_1bit frame_buffer (
        .clk(clk_48mhz),
        .nreset(nreset),
        .cam_wr_addr(cam_wr_addr),
        .cam_wr_data(cam_wr_data),
        .cam_wr_en(cam_wr_en),
        .cam_frame_done(cam_frame_done),
        .spi_rd_addr(spram_rd_addr),
        .spi_rd_data(spram_rd_data),
        .buffer_ready(buffer_ready)
    );

    // ------------------------------------------------------------------------
    // SPI slave controller
    // ------------------------------------------------------------------------
    spi_slave_camera spi_ctrl (
        .clk(clk_48mhz),
        .nreset(nreset),
        .spi_sck(spi_sck),
        .spi_mosi(spi_mosi),
        .spi_miso(spi_miso),
        .spi_ncs(spi_ncs),
        .spram_rd_addr(spram_rd_addr),
        .spram_rd_data(spram_rd_data),
        .buffer_ready(buffer_ready),
        .frame_read_complete(frame_read_complete)
    );

    // ------------------------------------------------------------------------
    // Debug LEDs
    // ------------------------------------------------------------------------
    assign led_red   = cam_frame_done;   // toggles when a frame completes
    assign led_green = buffer_ready;     // lights when frame buffer ready
    assign led_blue  = !spi_ncs;         // on during SPI transfer

endmodule

// ----------------------------------------------------------------------------
// Camera Capture with Threshold Module (unchanged)
// ----------------------------------------------------------------------------
module camera_capture_threshold (
    input  wire clk,
    input  wire nreset,
    input  wire cam_pclk,
    input  wire cam_vsync,
    input  wire cam_href,
    input  wire [7:0] cam_data,
    input  wire [7:0] threshold,
    output reg  [16:0] wr_addr,
    output reg         wr_data,
    output reg         wr_en,
    output reg         frame_done
);

    // Synchronize camera signals
    reg vsync_d1, vsync_d2;
    reg href_d1, href_d2;
    reg [7:0] data_d1;

    always @(posedge cam_pclk or negedge nreset) begin
        if (!nreset) begin
            vsync_d1 <= 0;
            vsync_d2 <= 0;
            href_d1  <= 0;
            href_d2  <= 0;
            data_d1  <= 0;
        end else begin
            vsync_d1 <= cam_vsync;
            vsync_d2 <= vsync_d1;
            href_d1  <= cam_href;
            href_d2  <= href_d1;
            data_d1  <= cam_data;
        end
    end

    wire vsync_rising = vsync_d1 && !vsync_d2;
    wire href_valid   = href_d2;

    // Capture logic
    reg [8:0] pixel_count;          
    reg [7:0] line_count;           
    reg byte_select;                
    reg in_frame;

    always @(posedge cam_pclk or negedge nreset) begin
        if (!nreset) begin
            pixel_count <= 0;
            line_count  <= 0;
            byte_select <= 0;
            wr_addr     <= 0;
            wr_data     <= 0;
            wr_en       <= 0;
            in_frame    <= 0;
            frame_done  <= 0;
        end else begin
            frame_done <= 0;

            if (vsync_rising) begin
                pixel_count <= 0;
                line_count  <= 0;
                byte_select <= 0;
                wr_addr     <= 0;
                in_frame    <= 1;
            end

            if (in_frame && href_valid) begin
                if (byte_select == 0) begin
                    wr_data <= (data_d1 > threshold);
                    wr_en   <= 1;
                    wr_addr <= wr_addr + 1;
                    pixel_count <= pixel_count + 1;
                end else begin
                    wr_en <= 0;
                end

                byte_select <= ~byte_select;

                if (pixel_count == 319 && byte_select == 1) begin
                    pixel_count <= 0;
                    byte_select <= 0;
                    line_count  <= line_count + 1;
                    if (line_count == 239) begin
                        in_frame   <= 0;
                        frame_done <= 1;
                    end
                end
            end else begin
                wr_en <= 0;
            end
        end
    end
endmodule


// ----------------------------------------------------------------------------
// Ping-Pong SPRAM Buffer (1-bit per pixel)
// ----------------------------------------------------------------------------

module spram_pingpong_1bit (
    input  wire clk,
    input  wire nreset,
    
    // Camera write interface
    input  wire [16:0] cam_wr_addr,
    input  wire        cam_wr_data,        // 1-bit
    input  wire        cam_wr_en,
    input  wire        cam_frame_done,
    
    // SPI read interface
    input  wire [16:0] spi_rd_addr,
    output reg  [7:0]  spi_rd_data,        // 8 bits packed
    
    output reg         buffer_ready
);

    // ------------------------------------------------------------------------
    // Ping-pong control
    // ------------------------------------------------------------------------
    reg buffer_select;              // 0 = write to buf0/read buf1, 1 = opposite
    reg buf0_full, buf1_full;

    // ------------------------------------------------------------------------
    // Internal SPRAM interface signals
    // ------------------------------------------------------------------------
    wire [13:0] spram0_addr, spram1_addr;
    wire [15:0] spram0_data_in, spram1_data_in;
    wire [15:0] spram0_data_out, spram1_data_out;
    wire        spram0_wren, spram1_wren;
    wire        spram0_cs, spram1_cs;

    // ------------------------------------------------------------------------
    // Bit-packing logic: pack 16 pixels (1-bit) into one 16-bit word
    // ------------------------------------------------------------------------
    reg  [3:0]  bit_counter;
    reg  [15:0] packed_data;

    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            bit_counter <= 0;
            packed_data <= 16'b0;
        end else if (cam_wr_en) begin
            packed_data[bit_counter] <= cam_wr_data;
            if (bit_counter == 15)
                bit_counter <= 0;
            else
                bit_counter <= bit_counter + 1;
        end
    end

    wire write_word = (bit_counter == 15) && cam_wr_en;
    wire [13:0] word_addr = cam_wr_addr[16:3];  // Divide by 16

    // ------------------------------------------------------------------------
    // SPRAM buffer 0 (SP256K primitive for Radiant)
    // ------------------------------------------------------------------------
    SP256K spram_buf0 (
        .AD        (spram0_addr),        // address
        .DI        (spram0_data_in),     // data input
        .MASKWE    (4'b1111),            // enable all nibbles
        .WE        (spram0_wren),        // write enable
        .CS        (spram0_cs),          // chip select
        .CK        (clk),                // clock
        .STDBY     (1'b0),               // normal operation
        .SLEEP     (1'b0),               // normal operation
        .PWROFF_N  (1'b1),               // powered on
        .DO        (spram0_data_out)     // data output
    );

    // ------------------------------------------------------------------------
    // SPRAM buffer 1 (SP256K primitive for Radiant)
    // ------------------------------------------------------------------------
    SP256K spram_buf1 (
        .AD        (spram1_addr),
        .DI        (spram1_data_in),
        .MASKWE    (4'b1111),
        .WE        (spram1_wren),
        .CS        (spram1_cs),
        .CK        (clk),
        .STDBY     (1'b0),
        .SLEEP     (1'b0),
        .PWROFF_N  (1'b1),
        .DO        (spram1_data_out)
    );

    // ------------------------------------------------------------------------
    // Address/data multiplexing logic
    // ------------------------------------------------------------------------
    assign spram0_addr     = buffer_select ? spi_rd_addr[16:3] : word_addr;
    assign spram0_data_in  = packed_data;
    assign spram0_wren     = buffer_select ? 1'b0 : write_word;
    assign spram0_cs       = 1'b1;

    assign spram1_addr     = buffer_select ? word_addr : spi_rd_addr[16:3];
    assign spram1_data_in  = packed_data;
    assign spram1_wren     = buffer_select ? write_word : 1'b0;
    assign spram1_cs       = 1'b1;

    // ------------------------------------------------------------------------
    // SPI read data (8 bits at a time)
    // ------------------------------------------------------------------------
    always @(*) begin
        if (buffer_select)
            spi_rd_data = spram0_data_out[15:8];  // reading buffer 0
        else
            spi_rd_data = spram1_data_out[15:8];  // reading buffer 1
    end

    // ------------------------------------------------------------------------
    // Ping-pong buffer switching
    // ------------------------------------------------------------------------
    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            buffer_select <= 1'b0;
            buf0_full     <= 1'b0;
            buf1_full     <= 1'b0;
            buffer_ready  <= 1'b0;
        end else begin
            if (cam_frame_done) begin
                buffer_ready <= 1'b1;
                if (buffer_select == 1'b0) begin
                    buf0_full     <= 1'b1;
                    buffer_select <= 1'b1;
                end else begin
                    buf1_full     <= 1'b1;
                    buffer_select <= 1'b0;
                end
            end
        end
    end

endmodule


// ----------------------------------------------------------------------------
// SPI Slave Controller (adapted from your reference)
// ----------------------------------------------------------------------------
module spi_slave_camera (
    input  wire clk,
    input  wire nreset,
    
    // SPI interface
    input  wire spi_sck,
    input  wire spi_mosi,
    output reg  spi_miso,
    input  wire spi_ncs,
    
    // SPRAM read interface
    output reg [16:0] spram_rd_addr,
    input  wire [7:0] spram_rd_data,
    
    // Control
    input  wire buffer_ready,
    output reg  frame_read_complete
);

    // Synchronize SPI signals
    reg sck_d1, sck_d2, sck_d3;
    reg ncs_d1, ncs_d2;
    
    always @(posedge clk) begin
        sck_d1 <= spi_sck;
        sck_d2 <= sck_d1;
        sck_d3 <= sck_d2;
        ncs_d1 <= spi_ncs;
        ncs_d2 <= ncs_d1;
    end
    
    wire sck_rising = sck_d2 && !sck_d3;
    wire sck_falling = !sck_d2 && sck_d3;
    wire ncs_active = !ncs_d2;
    
    // SPI shift register
    reg [7:0] shift_reg;
    reg [2:0] bit_count;
    
    // State machine (similar to your reference)
    localparam IDLE = 2'b00;
    localparam TRANSFER = 2'b01;
    localparam DONE = 2'b10;
    
    reg [1:0] state, next_state;
    
    always @(posedge clk or negedge nreset) begin
        if (!nreset)
            state <= IDLE;
        else
            state <= next_state;
    end
    
    always @(*) begin
        case (state)
            IDLE: next_state = (ncs_active && buffer_ready) ? TRANSFER : IDLE;
            TRANSFER: next_state = (!ncs_active) ? DONE : TRANSFER;
            DONE: next_state = IDLE;
            default: next_state = IDLE;
        endcase
    end
    
    // SPI transfer logic
    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            spram_rd_addr <= 0;
            shift_reg <= 0;
            bit_count <= 0;
            spi_miso <= 0;
            frame_read_complete <= 0;
        end else begin
            frame_read_complete <= 0;
            
            case (state)
                IDLE: begin
                    if (next_state == TRANSFER) begin
                        spram_rd_addr <= 0;
                        bit_count <= 0;
                        shift_reg <= spram_rd_data;
                    end
                end
                
                TRANSFER: begin
                    if (sck_falling) begin
                        spi_miso <= shift_reg[7];
                        shift_reg <= {shift_reg[6:0], 1'b0};
                        bit_count <= bit_count + 1;
                        
                        if (bit_count == 7) begin
                            bit_count <= 0;
                            spram_rd_addr <= spram_rd_addr + 1;
                            
                            if (spram_rd_addr >= 76799) begin
                                frame_read_complete <= 1;
                            end
                        end
                    end
                    
                    if (bit_count == 0 && sck_rising) begin
                        shift_reg <= spram_rd_data;
                    end
                end
                
                DONE: begin
                    spi_miso <= 0;
                    spram_rd_addr <= 0;
                end
            endcase
        end
    end

endmodule

