// ============================================================================
// Top-level: OV7670 -> Ping-Pong SPRAM -> SPI Slave TX
// Target: iCE40 UP5K
// ============================================================================
module top #(
    parameter int H_PIXELS = 320,
    parameter int V_LINES  = 240
)(
    // Common clock + reset
    input  logic        cam_pclk,
    input  logic        rst_n,        // active-low synchronous reset

    // OV7670 camera interface
    input  logic        cam_vsync,
    input  logic        cam_href,
    input  logic [7:0]  cam_data,

    // SPI slave interface (to MCU)
    input  logic        spi_sck,
    input  logic        spi_ncs,
    output logic        spi_miso,
	output logic 		led_green,
	output logic 		led_blue
);

    // ------------------------------------------------------------------------
    // Derived constants for framebuffer sizing (1 bit per pixel, 16 per word)
    // ------------------------------------------------------------------------
    localparam int FRAME_PIXELS = H_PIXELS * V_LINES;
    localparam int WORD_BITS    = 16;
    localparam int FRAME_WORDS  = (FRAME_PIXELS + WORD_BITS - 1) / WORD_BITS;
    localparam int ADDR_WIDTH   = $clog2(FRAME_WORDS);

    // ------------------------------------------------------------------------
    // Camera capture: OV7670 -> thresholded Y bit
    // ------------------------------------------------------------------------
    logic mask;
	logic y;
	localparam [7:0] y_threshold = 8'd128;

    camera_capture #(
        .H_ACTIVE(H_PIXELS),
        .V_ACTIVE(V_LINES)
    ) u_capture (
        .cam_pclk    (cam_pclk),
        .rst_n       (rst_n),
        .cam_vsync   (cam_vsync),
        .cam_href    (cam_href),
        .cam_data    (cam_data),
        .y_threshold (y_threshold),
        .mask (mask),
		.y (y)
    );

    // ------------------------------------------------------------------------
    // Ping-pong SPRAM framebuffer (write from camera, read to SPI)
    // ------------------------------------------------------------------------
    logic                   frame_done;
    logic                   buf_sel;          // which buffer holds last full frame
    logic [ADDR_WIDTH-1:0]  fb_rd_addr;
    logic [15:0]            fb_rd_data;

    ping_pong #(
        .H_PIXELS   (H_PIXELS),
        .V_LINES    (V_LINES),
        .PIXEL_BITS (1),
        .ADDR_WIDTH (ADDR_WIDTH)
    ) u_fb (
        .cam_pclk     (cam_pclk),
        .rst_n        (rst_n),

        // write side
        .pixel_valid  (y),
        .pixel_bit    (mask),

        // status
        .frame_done   (frame_done),
        .read_buf_sel (buf_sel),

        // read side used by SPI
        .rd_addr      (fb_rd_addr),
        .rd_data      (fb_rd_data)
    );
	assign led_red = buf_sel;      // Blink on frame complete

    // ------------------------------------------------------------------------
    // SPI slave frame transmitter (reads from framebuffer)
    // ------------------------------------------------------------------------
    logic frame_busy;

    //spi #(
        //.ADDR_WIDTH (ADDR_WIDTH),
        //.FRAME_WORDS(FRAME_WORDS)
    //) u_spi (
        //.clk         (cam_pclk),
        //.rst_n       (rst_n),

        //.frame_ready (frame_done),
        //.buf_sel     (buf_sel),
        //.frame_busy  (frame_busy),

        //.rd_addr     (fb_rd_addr),
        //.rd_data     (fb_rd_data),

        //.spi_sck     (spi_sck),
        //.spi_ncs     (spi_ncs),
        //.spi_miso    (spi_miso)
    //);

endmodule
