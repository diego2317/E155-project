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
    output wire spi_miso,           // Data to MCU
    input  wire spi_ncs,            // Chip select (active low)
   
    // Status outputs
    output wire pclk,
	output wire in_frame,
	output wire write_enable,
	output wire data
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
    localparam [7:0] THRESHOLD = 8'd128;  // Mid-point threshold
    // Adjust this value based on lighting conditions:
    // Lower value (e.g., 64)  = detect darker lines
    // Higher value (e.g., 192) = detect brighter lines
   
    // SPRAM signals (clk_48mhz domain)
    wire [16:0] spram_rd_addr;
    wire [7:0] spram_rd_data;
    wire buffer_ready;
    wire frame_read_complete;
   
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
        .frame_done(cam_frame_done),
		.in_frame(in_frame)
    );
	assign pclk = cam_pclk;
	assign write_enable = cam_wr_en;
	assign data = cam_wr_data;
    // Ping-pong SPRAM buffer with CDC (handles slow cam_pclk to fast sys_clk)
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
   
    // SPI slave controller (runs on sys_clk)
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
	assign spi_miso = cam_wr_data;
endmodule






