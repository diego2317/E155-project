// ============================================================================
// Camera Line Follower - FPGA Design 
// Target: iCE40UP5K (UPduino v3)
// ============================================================================

module camera_line_follower_top (
    input  wire nreset,

    // Camera Interface (OV7670)
    input  wire cam_pclk,           
    input  wire cam_vsync,          
    input  wire cam_href,           
    input  wire [7:0] cam_data,     

    // SPI Interface (to MCU)
    input  wire spi_sck,            
    input  wire spi_mosi,           
    output wire spi_miso,           
    input  wire spi_ncs,            

    // Status outputs
    output wire led_red,            
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
    // Ping-pong SPRAM buffer
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
    assign led_red   = cam_frame_done;
    assign led_green = buffer_ready;
    assign led_blue  = !spi_ncs;

endmodule
