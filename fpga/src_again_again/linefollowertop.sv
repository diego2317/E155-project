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

    // Debug / LEDs
    output wire led_frame_indicator, // Toggles with frame ready
    output wire led_cam_active,      // High when camera is capturing
    output wire led_write_active     // Pulses on SPRAM writes
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
    // Reset Synchronization (Recommended for multi-clock designs)
    // ------------------------------------------------------------------------
    // Async assert, sync deassert for each clock domain
    
    // System domain reset synchronizer
    reg [2:0] sys_reset_sync;
    wire sys_nreset;
    always @(posedge clk_48mhz or negedge nreset) begin
        if (!nreset)
            sys_reset_sync <= 3'b000;
        else
            sys_reset_sync <= {sys_reset_sync[1:0], 1'b1};
    end
    assign sys_nreset = sys_reset_sync[2];
    
    // Camera domain reset synchronizer
    reg [2:0] cam_reset_sync;
    wire cam_nreset;
    always @(posedge cam_pclk or negedge nreset) begin
        if (!nreset)
            cam_reset_sync <= 3'b000;
        else
            cam_reset_sync <= {cam_reset_sync[1:0], 1'b1};
    end
    assign cam_nreset = cam_reset_sync[2];
   
    // ------------------------------------------------------------------------
    // Threshold Processing (Camera Domain)
    // ------------------------------------------------------------------------
    wire [16:0] cam_wr_addr;
    wire cam_wr_data;
    wire cam_wr_en;
    wire cam_frame_done;
    wire cam_in_frame;              // Add this signal for LED
    
    // Fixed threshold 
    localparam [7:0] THRESHOLD = 8'd250; 
   
    camera_capture_threshold cam_cap (
        .cam_pclk(cam_pclk),
        .nreset(cam_nreset),        // Use synchronized reset
        .cam_vsync(cam_vsync),
        .cam_href(cam_href),
        .cam_data(cam_data),
        .threshold(THRESHOLD),
        .wr_addr(cam_wr_addr),
        .wr_data(cam_wr_data),
        .wr_en(cam_wr_en),
        .frame_done(cam_frame_done),
        .in_frame(cam_in_frame)     // Connect for LED debug
    );

    // ------------------------------------------------------------------------
    // Frame Buffer & Serializer (System Domain)
    // ------------------------------------------------------------------------
    
    frame_buffer_spram framebuffer (
        // Write Side (Camera)
        .w_clk(cam_pclk),
        .w_rst_n(cam_nreset),       // Use synchronized reset (same domain as w_clk)
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

    // ------------------------------------------------------------------------
    // Debug LED Logic
    // ------------------------------------------------------------------------
    
    // LED 1: Frame ready indicator (already assigned above)
    
    // LED 2: Camera active - shows when camera is capturing a frame
    assign led_cam_active = cam_in_frame;
    
    // LED 3: Write activity - CDC from camera domain to system domain for LED
    reg [2:0] wr_en_sync;
    reg led_wr_pulse;
    reg [15:0] wr_pulse_stretch;  // Stretch pulse so it's visible
    
    always @(posedge clk_48mhz or negedge sys_nreset) begin
        if (!sys_nreset) begin
            wr_en_sync <= 3'b000;
            led_wr_pulse <= 1'b0;
            wr_pulse_stretch <= 16'd0;
        end else begin
            // Synchronize write enable to system clock
            wr_en_sync <= {wr_en_sync[1:0], cam_wr_en};
            
            // Detect edge
            if (wr_en_sync[2:1] == 2'b01) begin
                wr_pulse_stretch <= 16'hFFFF;  // Start stretch counter
            end else if (wr_pulse_stretch != 16'd0) begin
                wr_pulse_stretch <= wr_pulse_stretch - 1'b1;
            end
            
            led_wr_pulse <= (wr_pulse_stretch != 16'd0);
        end
    end
    
    assign led_write_active = led_wr_pulse;

endmodule