// -----------------------------------------------------------------------------
// OV7670 YUV 4:2:2 Pixel Capture with Y Threshold
// Target: Lattice iCE40UP5K (but generic, no vendor primitives)
// - Assumes OV7670 is configured for YUV 4:2:2
// - Assumes output byte order: Y U Y V (TSLB[3]=0, COM13[0]=0)
// - Captures only Y bytes while HREF is high, discards U and V
// - Compares each Y to a programmable threshold
// - All state is synchronous to cam_pclk
// -----------------------------------------------------------------------------
module camera_capture #(
    // Active resolution in pixels (luma samples)
    // QVGA default: 320 x 240
    parameter int H_ACTIVE = 320,
    parameter int V_ACTIVE = 240
) (
    input  logic        cam_pclk,      // Pixel clock from OV7670 (PCLK)
    input  logic        rst_n,         // Active-low synchronous reset

    input  logic        cam_vsync,     // VSYNC from OV7670
    input  logic        cam_href,      // HREF from OV7670 (row data valid)
    input  logic [7:0]  cam_data,      // D[7:0] from OV7670

    input  logic [7:0]  y_threshold,   // Threshold to compare Y against

    output logic        mask, // 1 when y_value > y_threshold
	output logic		y // 1 when we read a y pixel

    // Optional debug/position info in frame
    //output logic [9:0]  // x_pos,         // 0 .. H_ACTIVE-1 (column index for Y samples)
    //output logic [8:0]  // y_pos          // 0 .. V_ACTIVE-1 (row index)
);

    // -------------------------------------------------------------------------
    // YUV 4:2:2 byte-phase tracking
    // For YUV 4:2:2 with Y U Y V ordering:
    //   PHASE_Y0  -> first Y
    //   PHASE_U   -> U shared between two Ys
    //   PHASE_Y1  -> second Y
    //   PHASE_V   -> V shared between the same two Ys
    // We only treat PHASE_Y0 and PHASE_Y1 as luminance samples.
    // -------------------------------------------------------------------------
    typedef enum logic [1:0] {
        PHASE_Y0  = 2'd0,
        PHASE_U   = 2'd1,
        PHASE_Y1  = 2'd2,
        PHASE_V   = 2'd3
    } yuv_phase_t;

    yuv_phase_t yuv_phase_q;

    // Previous HREF for end-of-line detection
    logic href_q;
	logic [9:0] y_pos;
	logic [8:0] x_pos;
    // -------------------------------------------------------------------------
    // Main sequential process all logic synchronous to cam_pclk
    // -------------------------------------------------------------------------
    localparam int H_MAX = H_ACTIVE - 1;
    localparam int V_MAX = V_ACTIVE - 1;

    always_ff @(posedge cam_pclk) begin
        if (!rst_n) begin
            yuv_phase_q   <= PHASE_Y0;
            href_q        <= 1'b0;

            // x_pos         <= '0;
            // y_pos         <= '0;

            mask <= 1'b0;
        end else begin
            // Register previous HREF for edge detection
            href_q  <= cam_href;


            // VSYNC high indicates vertical blanking / frame boundary.
            // We reset the line and column counters and phase here.
            if (cam_vsync) begin
                x_pos       <= '0;
                y_pos       <= '0;
                yuv_phase_q <= PHASE_Y0;
            end else begin
                // Detect end-of-line on falling edge of HREF while VSYNC is low
                if (href_q && !cam_href) begin
                    if (y_pos == V_MAX)
                        y_pos <= '0;
                    else
                        y_pos <= y_pos + 1'b1;
                end

                if (!cam_href) begin
                    // Outside active pixels horizontally: reset column and phase
                    x_pos       <= '0;
                    yuv_phase_q <= PHASE_Y0;
                end else begin
                    // Inside active pixel region (HREF = 1):
                    // cam_data is synchronous to cam_pclk; we sample on the clock edge.
                    // Y bytes occur at PHASE_Y0 and PHASE_Y1.
                    unique case (yuv_phase_q)
                        PHASE_Y0,
                        PHASE_Y1: begin
                            // Treat this byte as luminance (Y)
                            mask <= (cam_data > y_threshold);

                            // Advance horizontal Y-pixel position
                            if (x_pos == H_MAX)
                                x_pos <= '0;
                            else
                                x_pos <= x_pos + 1'b1;
                        end

                        default: begin
                            // U and V bytes are discarded
                            // (y_valid remains 0, x_pos unchanged)
                        end
                    endcase

                    // Advance YUV phase every byte while HREF is high
                    unique case (yuv_phase_q)
                        PHASE_Y0: yuv_phase_q <= PHASE_U;
                        PHASE_U : yuv_phase_q <= PHASE_Y1;
                        PHASE_Y1: yuv_phase_q <= PHASE_V;
                        PHASE_V : yuv_phase_q <= PHASE_Y0;
                        default : yuv_phase_q <= PHASE_Y0;
                    endcase
                end
            end
        end
    end

endmodule
