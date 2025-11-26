// ============================================================================
// Camera Line Follower - Top Level
// Target: iCE40UP5K
// ----------------------------------------------------------------------------
// Interfaces:
// 1. OV7670 Camera (Input)
// 2. MCU Interface (SPI-like Slave Output)
// ============================================================================

module camera_line_follower_top (
    // System
    input  wire nreset,             // External reset (active low)
   
    // Camera Interface (OV7670)
    input  wire cam_pclk,           // Pixel clock ~2.5 MHz
    input  wire cam_vsync,          // Frame sync
    input  wire cam_href,           // Line valid
    input  wire [7:0] cam_data,     // Pixel data
   
    // MCU Interface
    // 1. Clock Line (Input from MCU)
    input  wire mcu_clk_in,         
    // 2. Data Line (Output to MCU - Serial Stream)
    output wire mcu_data_out,       
    // 3. Frame Ready Line (Output - Toggles on new frame)
    output wire mcu_frame_ready,

    // Debug / LEDs (Optional)
    output wire led_frame_indicator // Toggles with frame ready
);

    // ------------------------------------------------------------------------
    // Clock Generation
    // ------------------------------------------------------------------------
    wire clk_48mhz;
    wire int_osc;
    
    // Internal High-Speed Oscillator (48MHz)
    HSOSC #(.CLKHF_DIV(2'b00)) 
        hf_osc (.CLKHFPU(1'b1), .CLKHFEN(1'b1), .CLKHF(int_osc));
   
    assign clk_48mhz = int_osc;
   
    // ------------------------------------------------------------------------
    // Threshold Processing (Camera Domain)
    // ------------------------------------------------------------------------
    wire [16:0] cam_wr_addr;
    wire cam_wr_data;
    wire cam_wr_en;
    wire cam_frame_done;
    
    // Fixed threshold 
    localparam [7:0] THRESHOLD = 8'd251; 
   
    camera_capture_threshold cam_cap (
        .cam_pclk(cam_pclk),
        .nreset(nreset),
        .cam_vsync(cam_vsync),
        .cam_href(cam_href),
        .cam_data(cam_data),
        .threshold(THRESHOLD),
        .wr_addr(cam_wr_addr),
        .wr_data(cam_wr_data),
        .wr_en(cam_wr_en),
        .frame_done(cam_frame_done),
        .in_frame() // Unused here
    );

    // ------------------------------------------------------------------------
    // Frame Buffer & Serializer (System Domain)
    // ------------------------------------------------------------------------
    
    frame_buffer_spram framebuffer (
        // Write Side (Camera)
        .w_clk(cam_pclk),
        .w_rst_n(nreset),
        .w_en(cam_wr_en),
        .w_addr_pixel(cam_wr_addr),
        .w_data_bit(cam_wr_data),
        .w_frame_done(cam_frame_done),

        // Read Side (MCU)
        .r_clk(clk_48mhz),          // Internal high speed clock
        .i_mcu_sck(mcu_clk_in),     // MCU provides the clock
        .o_mcu_mosi(mcu_data_out),  // FPGA sends the data
        .frame_ready(mcu_frame_ready)
    );

    assign led_frame_indicator = mcu_frame_ready;

endmodule