// ----------------------------------------------------------------------------
// Camera Capture with Threshold Module
// Runs entirely in cam_pclk domain (1.25-2.5 MHz)
// ----------------------------------------------------------------------------
module camera_capture_threshold (
    input  wire cam_pclk,           // Camera pixel clock domain (1.25-2.5 MHz)
    input  wire nreset,
    // CAmera inputs
    input  wire cam_vsync,          // HIGH=idle, LOW=active frame
    input  wire cam_href,           // HIGH=valid pixel data
    input  wire [7:0] cam_data,     // YUV422 pixel data
    // Threshold (from system clock domain, but stable)
    input  wire [7:0] threshold,
    // Output to SPRAM (cam_pclk domain)
    output reg [16:0] wr_addr,      // Pixel address (0-76799 for 320x240)
    output wire wr_data,            // 1-bit bitmask (thresholded) - combinational
    output reg wr_en,               // Write enable
    output reg frame_done,           // Pulse when frame complete
	output reg in_frame
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
    // Frame starts on FALLING edge (HIGHÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢LOW)
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
           
            // Start of new frame - VSYNC falling edge (HIGHÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢LOW)
            if (vsync_falling) begin
                pixel_count <= 9'd0;
                line_count <= 8'd0;
                byte_select <= 1'b0;
                wr_addr <= 17'd0;
                in_frame <= 1'b1;
            end
           
            // End of frame - VSYNC rising edge (LOWÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢HIGH)
            else if (vsync_rising && in_frame) begin
                in_frame <= 1'b0;
                frame_done <= 1'b1;
            end
           
            // Capture pixels during valid lines
            // Only process when BOTH current href and delayed href are valid
            // This prevents pipeline drain writes after HREF ends
            else if (in_frame && href_valid) begin
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