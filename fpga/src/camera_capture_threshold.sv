// ----------------------------------------------------------------------------
// Camera Capture with Threshold Module
// ----------------------------------------------------------------------------

module camera_capture_threshold (
    input  wire clk,
    input  wire nreset,
    input  wire cam_pclk,
    input  wire cam_vsync,
    input  wire cam_href,
    input  wire [7:0] cam_data,
    input  wire [7:0] threshold,
    output reg  [16:0] wr_addr,
    output reg         wr_data,
    output reg         wr_en,
    output reg         frame_done
);

    reg vsync_d1, vsync_d2;
    reg href_d1, href_d2;
    reg [7:0] data_d1;

    always @(posedge cam_pclk or negedge nreset) begin
        if (!nreset) begin
            vsync_d1 <= 0;
            vsync_d2 <= 0;
            href_d1  <= 0;
            href_d2  <= 0;
            data_d1  <= 0;
        end else begin
            vsync_d1 <= cam_vsync;
            vsync_d2 <= vsync_d1;
            href_d1  <= cam_href;
            href_d2  <= href_d1;
            data_d1  <= cam_data;
        end
    end

    wire vsync_rising = vsync_d1 && !vsync_d2;
    wire href_valid   = href_d2;

    reg [8:0] pixel_count;
    reg [7:0] line_count;
    reg byte_select;
    reg in_frame;

    always @(posedge cam_pclk or negedge nreset) begin
        if (!nreset) begin
            pixel_count <= 0;
            line_count  <= 0;
            byte_select <= 0;
            wr_addr     <= 0;
            wr_data     <= 0;
            wr_en       <= 0;
            in_frame    <= 0;
            frame_done  <= 0;
        end else begin
            frame_done <= 0;

            if (vsync_rising) begin
                pixel_count <= 0;
                line_count  <= 0;
                byte_select <= 0;
                wr_addr     <= 0;
                in_frame    <= 1;
            end

            if (in_frame && href_valid) begin
                if (byte_select == 0) begin
                    wr_data <= (data_d1 > threshold);
                    wr_en   <= 1;
                    wr_addr <= wr_addr + 1;
                    pixel_count <= pixel_count + 1;
                end else begin
                    wr_en <= 0;
                end

                byte_select <= ~byte_select;

                if (pixel_count == 319 && byte_select == 1) begin
                    pixel_count <= 0;
                    byte_select <= 0;
                    line_count  <= line_count + 1;
                    if (line_count == 239) begin
                        in_frame   <= 0;
                        frame_done <= 1;
                    end
                end
            end else begin
                wr_en <= 0;
            end
        end
    end
endmodule
