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
    output reg frame_done          // Pulse when frame complete
);

    // Pipeline register for camera signals
    reg vsync_d1;
    reg href_d1;
    reg [7:0] data_d1;
	reg in_frame;
   
    always @(posedge cam_pclk, negedge nreset) begin
        if (!nreset) begin
            vsync_d1 <= 1'b1;       // Initialize to idle state (HIGH)
            href_d1 <= 1'b0;
            data_d1 <= 8'h00;
        end else begin
            vsync_d1 <= cam_vsync;
            href_d1 <= cam_href;
            data_d1 <= cam_data;
        end
    end
   
    // Edge detection
    wire vsync_falling = !cam_vsync && vsync_d1;
    wire vsync_rising = cam_vsync && !vsync_d1;
    wire href_valid = href_d1;
    wire href_falling = !cam_href && href_d1;
   
    // Pixel counters
    reg [8:0] pixel_count;          // 0-319 (320 pixels per line)
    reg [7:0] line_count;           // 0-239 (240 lines per frame)
    reg byte_select;                // 0=Y (luma), 1=U/V (chroma)
    
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
            // Default values (prevent latches)
            frame_done <= 1'b0;
            wr_en <= 1'b0;
           
            // PRIORITY 1: Frame start (VSYNC falling edge)
            if (vsync_falling) begin
                pixel_count <= 9'd0;
                line_count <= 8'd0;
                byte_select <= 1'b0;
                wr_addr <= 17'd0;
                in_frame <= 1'b1;
            end
           
            // PRIORITY 2: Frame end (VSYNC rising edge) - ONLY THIS ENDS FRAMES
            else if (vsync_rising && in_frame) begin
                in_frame <= 1'b0;
                frame_done <= 1'b1;
            end
           
            // PRIORITY 3: Pixel capture (only when in frame)
            else if (in_frame && href_valid) begin
                // YUV422 format: Y0 U Y1 V Y2 U Y3 V...
                // Only capture Y (luminance) values (byte_select == 0)
                if (byte_select == 1'b0) begin
                    wr_en <= 1'b1;
                    
                    // Increment address AFTER writing current pixel
                    if (wr_addr < 17'd76799) begin
                        wr_addr <= wr_addr + 1'b1;
                    end
                    
                    pixel_count <= pixel_count + 1'b1;
                    byte_select <= 1'b1;  // Next byte is U/V
                end else begin
                    byte_select <= 1'b0;  // Next byte is Y
                end
            end
           
            // PRIORITY 4: End of line (reset counters, don't end frame)
            else if (in_frame && href_falling) begin
                pixel_count <= 9'd0;
                byte_select <= 1'b0;
                
                // Increment line counter
                if (line_count < 8'd239) begin
                    line_count <= line_count + 1'b1;
                end
            end
        end
    end

endmodule