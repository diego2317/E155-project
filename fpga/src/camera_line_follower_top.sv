// ============================================================================
// Camera Line Follower - Complete FPGA Design (VSYNC POLARITY FIXED)
// For iCE40UP5K (UPduino v3.0)
// Uses internal HSOSC at 48 MHz
// XCLK provided by MCU, Camera PCLK at 1.25-2.5 MHz
// ============================================================================

// ----------------------------------------------------------------------------
// Top Module
// ----------------------------------------------------------------------------
module camera_line_follower_top (
    // System
    input  wire nreset,             // External reset (active low)
   
    // Camera Interface (OV7670)
    // Note: XCLK comes from MCU (PA11), not FPGA
    input  wire cam_pclk,           // Pixel clock from camera (1.25-2.5 MHz)
    input  wire cam_vsync,          // Frame sync (HIGH=idle, LOW=active frame)
    input  wire cam_href,           // Line valid (HIGH during valid pixel data)
    input  wire [7:0] cam_data,     // Pixel data D[7:0]
   
    // SPI Interface (to MCU)
    input  wire spi_sck,            // SPI clock from MCU
    input  wire spi_mosi,           // Data from MCU (unused for now)
    output logic spi_miso,           // Data to MCU
    input  wire spi_ncs,            // Chip select (active low)
   
    // Status outputs
    output wire led_red,            // Debug LED - pulses on frame complete
    output wire led_green,          // Debug LED - on when buffer ready
    output wire led_blue            // Debug LED - on during SPI transfer
);

    // Clock signals
    wire clk_48mhz;                 // System clock from HSOSC
    wire pll_locked;                // Always 1 for HSOSC
   
    // Camera capture signals (cam_pclk domain - 1.25-2.5 MHz)
    wire [16:0] cam_wr_addr;
    wire cam_wr_data;               // 1-bit bitmask
    wire cam_wr_en;
    wire cam_frame_done;
   
    // Fixed threshold value (can be changed here or made a parameter)
    localparam [7:0] THRESHOLD = 8'd80;  // Mid-point threshold
    // Adjust this value based on lighting conditions:
    // Lower value (e.g., 64)  = detect darker lines
    // Higher value (e.g., 192) = detect brighter lines
   
    // SPRAM signals (clk_48mhz domain)
    wire [16:0] spram_rd_addr;
    wire [7:0] spram_rd_data;
    wire buffer_ready;
    wire frame_read_complete;
	
	always_ff @(posedge cam_pclk) begin
		if (spi_miso == 1'b1) begin
			spi_miso = 1'b0;
		end else begin
			spi_miso = 1'b1;
		end
	end
		
    // Internal high-speed oscillator (48MHz)
    wire int_osc;
    HSOSC #(.CLKHF_DIV(2'b00))
        hf_osc (.CLKHFPU(1'b1), .CLKHFEN(1'b1), .CLKHF(int_osc));
   
    assign clk_48mhz = int_osc;
    assign pll_locked = 1'b1;  // HSOSC always locked
   
    // Camera capture with threshold (runs on cam_pclk - slow clock domain)
    camera_capture_threshold cam_cap (
        .cam_pclk(cam_pclk),
        .nreset(nreset),
        .cam_vsync(cam_vsync),
        .cam_href(cam_href),
        .cam_data(cam_data),
        .threshold(THRESHOLD),      // Fixed threshold
        .wr_addr(cam_wr_addr),
        .wr_data(cam_wr_data),
        .wr_en(cam_wr_en),
        .frame_done(cam_frame_done)
    );
   
     //Ping-pong SPRAM buffer with CDC (handles slow cam_pclk to fast sys_clk)
    //spram_pingpong_1bit frame_buffer (
        //.sys_clk(clk_48mhz),
        //.cam_clk(cam_pclk),
        //.nreset(nreset),
        //.cam_wr_addr(cam_wr_addr),
        //.cam_wr_data(cam_wr_data),
        //.cam_wr_en(cam_wr_en),
        //.cam_frame_done(cam_frame_done),
        //.spi_rd_addr(spram_rd_addr),
        //.spi_rd_data(spram_rd_data),
        //.buffer_ready(buffer_ready),
        //.frame_read_complete(frame_read_complete)
    //);
   
     //SPI slave controller (runs on sys_clk)
    //spi_slave_camera spi_ctrl (
        //.clk(clk_48mhz),
        //.nreset(nreset),
        //.spi_sck(spi_sck),
        //.spi_mosi(spi_mosi),
        //.spi_miso(spi_miso),
        //.spi_ncs(spi_ncs),
        //.spram_rd_addr(spram_rd_addr),
        //.spram_rd_data(spram_rd_data),
        //.buffer_ready(buffer_ready),
        //.frame_read_complete(frame_read_complete)
    //);
   
    // Debug LEDs
    assign led_red = cam_frame_done;      // Blink on frame complete
    assign led_green = buffer_ready;      // On when frame ready
    assign led_blue = !spi_ncs;           // On during SPI transfer
	//assign spi_miso =cam_wr_data;

endmodule

// ----------------------------------------------------------------------------
// Camera Capture with Threshold Module
// Runs entirely in cam_pclk domain (1.25-2.5 MHz)
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Camera Capture with Threshold Module
// Runs entirely in cam_pclk domain (1.25-2.5 MHz)
// ----------------------------------------------------------------------------
module camera_capture_threshold (
    input  wire cam_pclk,           // Camera pixel clock domain (1.25-2.5 MHz)
    input  wire nreset,
   
    // Camera inputs
    input  wire cam_vsync,          // HIGH=idle, LOW=active frame
    input  wire cam_href,           // HIGH=valid pixel data
    input  wire [7:0] cam_data,     // YUV422 pixel data
   
    // Threshold (from system clock domain, but stable)
    input  wire [7:0] threshold,
   
    // Output to SPRAM (cam_pclk domain)
    output reg [16:0] wr_addr,      // Pixel address (0-76799 for 320x240)
    output wire wr_data,            // 1-bit bitmask (thresholded) - combinational
    output reg wr_en,               // Write enable
    output reg frame_done           // Pulse when frame complete
);

    // Pipeline register for camera signals
    // NOTE: These are already in cam_pclk domain (NOT CDC!)
    // Single register stage for timing closure and edge detection
    reg vsync_d1;
    reg href_d1;
    reg [7:0] data_d1;
   
    always @(posedge cam_pclk, negedge nreset) begin
        if (!nreset) begin
            vsync_d1 <= 1'b1;       // Initialize to idle state (HIGH)
            href_d1 <= 1'b0;
            data_d1 <= 8'h00;
        end else begin
            // Register camera signals (already synchronous to cam_pclk)
            vsync_d1 <= cam_vsync;
            href_d1 <= cam_href;
            data_d1 <= cam_data;
        end
    end
   
    // Edge detection on synchronized signals
    // VSYNC: HIGH when idle, LOW during frame transmission
    // Frame starts on FALLING edge (HIGHâ†’LOW)
    wire vsync_falling = !cam_vsync && vsync_d1;
    wire vsync_rising = cam_vsync && !vsync_d1;
    wire href_valid = href_d1;      // Use href_d1 to match data_d1 timing
    wire href_falling = !cam_href && href_d1;
   
    // Pixel counters
    reg [8:0] pixel_count;          // 0-319 (320 pixels per line)
    reg [7:0] line_count;           // 0-239 (240 lines per frame)
    reg byte_select;                // 0=Y (luma), 1=U/V (chroma)
    reg in_frame;                   // Currently capturing frame
    
    // Threshold comparison (combinational output)
    assign wr_data = (data_d1 > threshold);
   
    // Capture and threshold logic
    always @(posedge cam_pclk, negedge nreset) begin
        if (!nreset) begin
            pixel_count <= 9'd0;
            line_count <= 8'd0;
            byte_select <= 1'b0;
            wr_addr <= 17'd0;
            wr_en <= 1'b0;
            in_frame <= 1'b0;
            frame_done <= 1'b0;
        end else begin
            frame_done <= 1'b0;     // Default: pulse, not level
            wr_en <= 1'b0;          // Default: no write
           
            // Start of new frame - VSYNC falling edge (HIGHâ†’LOW)
            if (vsync_falling) begin
                pixel_count <= 9'd0;
                line_count <= 8'd0;
                byte_select <= 1'b0;
                wr_addr <= 17'd0;
                in_frame <= 1'b1;
            end
           
            // End of frame - VSYNC rising edge (LOWâ†’HIGH)
            else if (vsync_rising && in_frame) begin
                in_frame <= 1'b0;
                frame_done <= 1'b1;
            end
           
            // Capture pixels during valid lines
            // Only process when BOTH current href and delayed href are valid
            // This prevents pipeline drain writes after HREF ends
            else if (in_frame && href_valid && cam_href) begin
                // YUV422 format: Y0 U Y1 V Y2 U Y3 V...
                // Only capture Y (luminance) values (byte_select == 0)
                if (byte_select == 1'b0) begin
                    // wr_data is computed combinationally via assign statement
                    // Just enable the write
                    wr_en <= 1'b1;
                    
                    // Increment address AFTER writing current pixel
                    if (wr_addr < 17'd76799) begin
                        wr_addr <= wr_addr + 1'b1;
                    end
                    
                    pixel_count <= pixel_count + 1'b1;
                    
                    // Toggle byte_select after processing Y byte
                    byte_select <= 1'b1;
                end else begin
                    wr_en <= 1'b0;  // Don't write during U/V bytes
                    
                    // Toggle byte_select after processing U/V byte
                    byte_select <= 1'b0;
                end
            end
           
            // End of line detection (HREF falling edge)
            if (in_frame && href_falling) begin
                pixel_count <= 9'd0;
                byte_select <= 1'b0;
                
                // Check if we've completed all lines
                if (line_count >= 8'd239) begin
                    in_frame <= 1'b0;
                    frame_done <= 1'b1;
                end else begin
                    line_count <= line_count + 1'b1;
                end
            end
        end
    end

endmodule

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

// ----------------------------------------------------------------------------
// SPI Slave Controller
// Runs on sys_clk (48 MHz), interfaces with MCU SPI
// ----------------------------------------------------------------------------
module spi_slave_camera (
    input  wire clk,                // System clock (48 MHz)
    input  wire nreset,
   
    // SPI interface (from MCU)
    input  wire spi_sck,            // SPI clock (much slower than sys_clk)
    input  wire spi_mosi,           // Master out, slave in (unused)
    output reg  spi_miso,           // Master in, slave out
    input  wire spi_ncs,            // Chip select (active low)
   
    // SPRAM read interface (sys_clk domain)
    output reg [16:0] spram_rd_addr, // Byte address (0-9599 for 9600 bytes)
    input  wire [7:0] spram_rd_data, // Data from SPRAM
   
    // Control
    input  wire buffer_ready,       // Frame ready to read
    input  wire which_buffer,       // Which buffer to read from
    output reg  frame_read_complete // Pulse when all bytes sent
);

    // ========================================================================
    // Synchronizers for SPI signals (external async inputs)
    // ========================================================================
   
    reg sck_sync1, sck_sync2, sck_sync3, sck_sync4;
    reg ncs_sync1, ncs_sync2, ncs_sync3;
   
    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            sck_sync1 <= 0;
            sck_sync2 <= 0;
            sck_sync3 <= 0;
            sck_sync4 <= 0;
            ncs_sync1 <= 1;
            ncs_sync2 <= 1;
            ncs_sync3 <= 1;
        end else begin
            sck_sync1 <= spi_sck;
            sck_sync2 <= sck_sync1;
            sck_sync3 <= sck_sync2;
            sck_sync4 <= sck_sync3;
           
            ncs_sync1 <= spi_ncs;
            ncs_sync2 <= ncs_sync1;
            ncs_sync3 <= ncs_sync2;
        end
    end
   
    // Edge detection
    wire sck_rising = sck_sync3 && !sck_sync4;
    wire sck_falling = !sck_sync3 && sck_sync4;
    wire ncs_active = !ncs_sync3;
   
    // ========================================================================
    // SPI State Machine
    // ========================================================================
   
    localparam IDLE = 2'b00;
    localparam TRANSFER = 2'b01;
    localparam DONE = 2'b10;
   
    reg [1:0] state, next_state;
    reg [7:0] shift_reg;
    reg [2:0] bit_count;            // 0-7
   
    always @(posedge clk or negedge nreset) begin
        if (!nreset)
            state <= IDLE;
        else
            state <= next_state;
    end
   
    always @(posedge spi_sck) begin
        case (state)
            IDLE:     next_state = (ncs_active && buffer_ready) ? TRANSFER : IDLE;
            TRANSFER: next_state = (!ncs_active) ? DONE : TRANSFER;
            DONE:     next_state = IDLE;
            default:  next_state = IDLE;
        endcase
    end
   
    // SPI transfer logic
    always @(posedge spi_sck or negedge nreset) begin
        if (!nreset) begin
            spram_rd_addr <= 0;
            shift_reg <= 0;
            bit_count <= 0;
            spi_miso <= 0;
            frame_read_complete <= 0;
        end else begin
            frame_read_complete <= 0;  // Default: pulse
           
            case (state)
                IDLE: begin
                    if (next_state == TRANSFER) begin
                        // Start new transfer
                        spram_rd_addr <= 0;
                        bit_count <= 0;
                        shift_reg <= spram_rd_data;  // Load first byte
                    end
                end
               
                TRANSFER: begin
                    // Shift out on falling edge
                    if (sck_falling) begin
                        spi_miso <= shift_reg[7];
                        shift_reg <= {shift_reg[6:0], 1'b0};
                        bit_count <= bit_count + 1;
                       
                        // Byte complete
                        if (bit_count == 7) begin
                            bit_count <= 0;
                            spram_rd_addr <= spram_rd_addr + 1;
                           
                            // Check if all bytes sent (9600 bytes total)
                            if (spram_rd_addr >= 9599) begin
                                frame_read_complete <= 1;
                            end
                        end
                    end
                   
                    // Load next byte on rising edge after byte complete
                    if (bit_count == 0 && sck_rising) begin
                        shift_reg <= spram_rd_data;
                    end
                end
               
                DONE: begin
                    spi_miso <= 0;
                end
            endcase
        end
    end

endmodule

// No PLL module needed - using internal HSOSC

//module SP256K (
    //input  wire [13:0] AD,          // Address (14 bits = 16K words)
    //input  wire [15:0] DI,          // Data input
    //input  wire [3:0]  MASKWE,      // Write mask (active high)
    //input  wire        WE,          // Write enable
    //input  wire        CS,          // Chip select
    //input  wire        CK,          // Clock
    //input  wire        STDBY,       // Standby mode
    //input  wire        SLEEP,       // Sleep mode
    //input  wire        PWROFF_N,    // Power off (active low)
    //output reg  [15:0] DO           // Data output
//);

    // Memory array
    //reg [15:0] mem [0:16383];
   
     //Initialize memory to zero for simulation
    //integer i;
    //initial begin
        //for (i = 0; i < 16384; i = i + 1) begin
            //mem[i] = 16'h0000;
        //end
        //DO = 16'h0000;
    //end
   
    // Memory operations
    //always @(posedge CK) begin
        //if (CS && PWROFF_N && !STDBY && !SLEEP) begin
            //if (WE) begin
                // Write with byte masking
                //if (MASKWE[0]) mem[AD][3:0]   <= DI[3:0];
                //if (MASKWE[1]) mem[AD][7:4]   <= DI[7:4];
                //if (MASKWE[2]) mem[AD][11:8]  <= DI[11:8];
                //if (MASKWE[3]) mem[AD][15:12] <= DI[15:12];
            //end else begin
                // Read
                //DO <= mem[AD];
            //end
        //end
    //end

//endmodule