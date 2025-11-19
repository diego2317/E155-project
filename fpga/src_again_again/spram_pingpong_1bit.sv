// ----------------------------------------------------------------------------
// Ping-Pong SPRAM Buffer (1-bit per pixel)
// Handles CDC from slow cam_pclk (1.25-2.5 MHz) to fast sys_clk (48 MHz)
// ----------------------------------------------------------------------------
module spram_pingpong_1bit (
    input  wire sys_clk,            // System clock (48MHz)
    input  wire cam_clk,            // Camera clock (1.25-2.5 MHz)
    input  wire nreset,
   
    // Camera write interface (cam_clk domain)
    input  wire [16:0] cam_wr_addr, // Pixel address (0-76799)
    input  wire cam_wr_data,        // 1-bit thresholded pixel
    input  wire cam_wr_en,          // Write enable
    input  wire cam_frame_done,     // Frame complete pulse
   
    // SPI read interface (sys_clk domain)
    input  wire [16:0] spi_rd_addr, // Byte address (0-9599)
    output reg [7:0] spi_rd_data,   // 8 bits packed
   
    // Control
    output reg buffer_ready,        // Frame ready for MCU to read
    input  wire frame_read_complete // MCU finished reading
);

    // ========================================================================
    // Clock Domain Crossing (CDC) Synchronizers
    // ========================================================================
   
    // Synchronize frame_done from cam_clk to sys_clk (3-stage for safety)
    reg frame_done_sync1, frame_done_sync2, frame_done_sync3;
    always @(posedge sys_clk or negedge nreset) begin
        if (!nreset) begin
            frame_done_sync1 <= 0;
            frame_done_sync2 <= 0;
            frame_done_sync3 <= 0;
        end else begin
            frame_done_sync1 <= cam_frame_done;
            frame_done_sync2 <= frame_done_sync1;
            frame_done_sync3 <= frame_done_sync2;
        end
    end
    wire frame_done_synced = frame_done_sync2 && !frame_done_sync3;  // Edge detect
   
    // ========================================================================
    // Bit Packing Logic (cam_clk domain)
    // Pack 16 bits into one 16-bit SPRAM word
    // ========================================================================
   
    reg [3:0] bit_counter;          // 0-15
    reg [15:0] packed_data;         // Accumulate 16 bits
    reg packed_valid;               // Write to SPRAM
    reg [13:0] packed_addr;         // SPRAM address (76800/16 = 4800 words)
   
    always @(posedge cam_clk or negedge nreset) begin
        if (!nreset) begin
            bit_counter <= 0;
            packed_data <= 0;
            packed_valid <= 0;
            packed_addr <= 0;
        end else begin
            packed_valid <= 0;  // Default: pulse
           
            if (cam_wr_en) begin
                // Accumulate bits
                packed_data[bit_counter] <= cam_wr_data;
               
                if (bit_counter == 15) begin
                    // All 16 bits collected - write to SPRAM
                    packed_valid <= 1;
                    packed_addr <= cam_wr_addr[16:4];  // Divide by 16
                    bit_counter <= 0;
                end else begin
                    bit_counter <= bit_counter + 1;
                end
            end
        end
    end
   
    // ========================================================================
    // Dual SPRAM with Ping-Pong Control
    // ========================================================================
   
    reg buffer_select;              // 0=write buf0/read buf1, 1=write buf1/read buf0
    reg buf0_full, buf1_full;       // Buffer status flags
   
    // SPRAM signals for buffer 0
    wire [13:0] spram0_addr;
    wire [15:0] spram0_data_in;
    wire [15:0] spram0_data_out;
    wire spram0_wren;
    wire spram0_cs;
   
    // SPRAM signals for buffer 1
    wire [13:0] spram1_addr;
    wire [15:0] spram1_data_in;
    wire [15:0] spram1_data_out;
    wire spram1_wren;
    wire spram1_cs;
   
    // Instantiate SPRAM blocks (256Kbit each = 16K x 16-bit)
    SP256K spram_buf0 (
		.AD(spram0_addr),
		.DI(spram0_data_in),
        .MASKWE(4'b1111),         // Write all bytes
        .WE(spram0_wren),
        .CS(spram0_cs),
        .CK(sys_clk),            // All SPRAM ops on sys_clk
        .STDBY(1'b0),
        .SLEEP(1'b0),
        .PWROFF_N(1'b1),
        .DO(spram0_data_out)
    );
   
    SP256K spram_buf1 (
	    .AD(spram1_addr),
	    .DI(spram1_data_in),
        .MASKWE(4'b1111),
        .WE(spram1_wren),
        .CS(spram1_cs),
        .CK(sys_clk),
        .STDBY(1'b0),
        .SLEEP(1'b0),
        .PWROFF_N(1'b1),
        .DO(spram1_data_out)
    );
   
    // CDC: Synchronize packed_valid and packed_addr to sys_clk
    reg packed_valid_sync1, packed_valid_sync2, packed_valid_sync3;
    reg [13:0] packed_addr_sync;
    reg [15:0] packed_data_sync;
   
    always @(posedge sys_clk or negedge nreset) begin
        if (!nreset) begin
            packed_valid_sync1 <= 0;
            packed_valid_sync2 <= 0;
            packed_valid_sync3 <= 0;
            packed_addr_sync <= 0;
            packed_data_sync <= 0;
        end else begin
            packed_valid_sync1 <= packed_valid;
            packed_valid_sync2 <= packed_valid_sync1;
            packed_valid_sync3 <= packed_valid_sync2;
           
            if (packed_valid_sync1) begin
                packed_addr_sync <= packed_addr;
                packed_data_sync <= packed_data;
            end
        end
    end
   
    wire packed_valid_pulse = packed_valid_sync2 && !packed_valid_sync3;
   
    // Write port multiplexing (camera writes to active buffer)
    assign spram0_addr = buffer_select ? spi_rd_addr[16:3] : packed_addr_sync;
    assign spram0_data_in = packed_data_sync;
    assign spram0_wren = buffer_select ? 1'b0 : packed_valid_pulse;
    assign spram0_cs = 1'b1;
   
    assign spram1_addr = buffer_select ? packed_addr_sync : spi_rd_addr[16:3];
    assign spram1_data_in = packed_data_sync;
    assign spram1_wren = buffer_select ? packed_valid_pulse : 1'b0;
    assign spram1_cs = 1'b1;
   
    // Read data path - unpack 16-bit words to 8-bit bytes
    // spi_rd_addr[2:0] selects bit position within word
    // spi_rd_addr[3] selects upper/lower byte
    wire [15:0] current_word = buffer_select ? spram0_data_out : spram1_data_out;
   
    always @(*) begin
        if (spi_rd_addr[3]) begin
            spi_rd_data = current_word[15:8];  // Upper byte
        end else begin
            spi_rd_data = current_word[7:0];   // Lower byte
        end
    end
   
    // Ping-pong buffer control (sys_clk domain)
    always @(posedge sys_clk or negedge nreset) begin
        if (!nreset) begin
            buffer_select <= 0;
            buf0_full <= 0;
            buf1_full <= 0;
            buffer_ready <= 0;
        end else begin
            // New frame complete
            if (frame_done_synced) begin
                if (buffer_select == 0) begin
                    buf0_full <= 1;
                    buffer_select <= 1;     // Switch to buf1 for next frame
                    buffer_ready <= 1;      // Buf0 ready to read
                end else begin
                    buf1_full <= 1;
                    buffer_select <= 0;     // Switch to buf0 for next frame
                    buffer_ready <= 1;      // Buf1 ready to read
                end
            end
           
            // Clear buffer_ready when MCU finishes reading
            if (frame_read_complete) begin
                buffer_ready <= 0;
                // Clear the full flag for the buffer that was just read
                if (buffer_select == 0) begin
                    buf1_full <= 0;  // Just read from buf1
                end else begin
                    buf0_full <= 0;  // Just read from buf0
                end
            end
        end
    end

endmodule
