// ----------------------------------------------------------------------------
// Camera Capture with Threshold Module - MINIMAL FIX VERSION
// Runs entirely in cam_pclk domain (1.25-2.5 MHz)
// ----------------------------------------------------------------------------
module camera_capture_threshold (
    input  wire cam_pclk,
    input  wire nreset,
    input  wire cam_vsync,
    input  wire cam_href,
    input  wire [7:0] cam_data,
    input  wire [7:0] threshold,
    output reg [16:0] wr_addr,
    output wire wr_data,
    output reg wr_en,
    output reg frame_done,
    output reg in_frame              // FIXED: Only ONE declaration
);

    reg vsync_d1;
    reg href_d1;
    reg [7:0] data_d1;
   
    always @(posedge cam_pclk, negedge nreset) begin
        if (!nreset) begin
            vsync_d1 <= 1'b1;
            href_d1 <= 1'b0;
            data_d1 <= 8'h00;
        end else begin
            vsync_d1 <= cam_vsync;
            href_d1 <= cam_href;
            data_d1 <= cam_data;
        end
    end
   
    wire vsync_falling = !cam_vsync && vsync_d1;
    wire vsync_rising = cam_vsync && !vsync_d1;
    wire href_valid = href_d1;
    wire href_falling = !cam_href && href_d1;
   
    reg [8:0] pixel_count;
    reg [7:0] line_count;
    reg byte_select;
    
    assign wr_data = (data_d1 > threshold);
   
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
            frame_done <= 1'b0;
            wr_en <= 1'b0;
           
            // Start of new frame
            if (vsync_falling) begin
                pixel_count <= 9'd0;
                line_count <= 8'd0;
                byte_select <= 1'b0;
                wr_addr <= 17'd0;
                in_frame <= 1'b1;
            end
           
            // ONLY VSYNC RISING ENDS FRAMES (CRITICAL FIX!)
            else if (vsync_rising && in_frame) begin
                in_frame <= 1'b0;
                frame_done <= 1'b1;
            end
           
            // Capture pixels during valid lines
            else if (in_frame && href_valid) begin
                if (byte_select == 1'b0) begin
                    wr_en <= 1'b1;
                    
                    if (wr_addr < 17'd76799) begin
                        wr_addr <= wr_addr + 1'b1;
                    end
                    
                    pixel_count <= pixel_count + 1'b1;
                    byte_select <= 1'b1;
                end else begin
                    byte_select <= 1'b0;
                end
            end
           
            // End of line - just reset counters, DON'T end frame
            else if (in_frame && href_falling) begin
                pixel_count <= 9'd0;
                byte_select <= 1'b0;
                
                // FIXED: Line counter always increments
                if (line_count < 8'd239) begin
                    line_count <= line_count + 1'b1;
                end
                // Note: We do NOT set frame_done here anymore!
            end
        end
    end

endmodule