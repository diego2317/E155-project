module camera (
    input logic pclk, reset,
    //input logic href,
    //input logic vsync,
    //input logic [7:0] d,
    //output logic y_pixel,
    output logic mask
    //output logic [8:0] x_addr,
    //output logic [7:0] y_addr
);

    //localparam [7:0] y_threshold = 8'd80;

    //logic r1_vysnc, r2_vsync;
    //logic frame_start, frame_done;

    //initial { r1_vsync, r2_vsync } = 0; 

    //typedef enum logic [1:0] {
        //Y0 = 2'b00,
        //U = 2'b01,
        //Y1 = 2'b10,
        //V = 2'b11
    //} pixel_state;

    //pixel_state state;

    always @(posedge pclk) begin
        // check if we're in a valid row
        if (reset) begin
			mask <= 1'b0; // Reset output to 0
		end else begin
			mask <= ~mask; // Toggle the output
		end 


    end




endmodule