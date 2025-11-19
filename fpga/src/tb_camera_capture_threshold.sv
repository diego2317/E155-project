// tb_camera_capture_threshold.sv
`timescale 1ns/1ps
module tb_camera_capture_threshold;
    reg cam_pclk;
    reg nreset;
    reg cam_vsync;
    reg cam_href;
    reg [7:0] cam_data;
    reg [7:0] threshold;

    wire [16:0] wr_addr;
    wire wr_data;
    wire wr_en;
    wire frame_done;

    // instantiate DUT
    camera_capture_threshold dut (
        .cam_pclk(cam_pclk),
        .nreset(nreset),
        .cam_vsync(cam_vsync),
        .cam_href(cam_href),
        .cam_data(cam_data),
        .threshold(threshold),
        .wr_addr(wr_addr),
        .wr_data(wr_data),
        .wr_en(wr_en),
        .frame_done(frame_done)
    );
    integer i;
    integer p;
    initial begin
        $display("TB: camera_capture_threshold starting");
        cam_pclk = 0;
        nreset = 0;
        cam_vsync = 1; // idle
        cam_href = 0;
        cam_data = 0;
        threshold = 8'd128;
        #200;
        nreset = 1;
        #200;

        // Start frame: assert VSYNC low for a while (frame active)
        // Note: your module expects VSYNC falling = start
        cam_vsync = 1; #200;
        cam_vsync = 0; #200;

        // Generate two short HREF lines, with 32 Y pixels per line
        
        for (i=0; i<2; i=i+1) begin
            cam_href = 1;
            // for test reduce number of pairs (Y+UV) to keep sim short
            
            for (p=0; p<32; p=p+1) begin
                // Y byte   
                cam_data = (p % 2) ? 8'd200 : 8'd50; // alternating bright/dark
                #200; // one cam_pclk period (half toggles happen elsewhere)
                // U/V byte (ignored)
                cam_data = 8'd128;
                #200;
            end
            cam_href = 0;
            #1000; // short gap between lines (HREF falling edge)
        end

        // End frame
        cam_vsync = 1; #200;
        #1000;

        $display("Final wr_addr=%0d, last wr_data=%b, last wr_en=%b, frame_done=%b", wr_addr, wr_data, wr_en, frame_done);
        $display("TB: camera_capture_threshold done");
        $finish;
    end

    // cam_pclk: use 400ns period (2.5 MHz) -> half-period 200ns
    always #200 cam_pclk = ~cam_pclk;
endmodule
