// tb_top_integration.sv
`timescale 1ns/1ps
module tb_top_integration;
    // top ports
    reg nreset;
    reg cam_pclk;
    reg cam_vsync;
    reg cam_href;
    reg [7:0] cam_data;
    reg spi_sck;
    reg spi_mosi;
    wire spi_miso;
    reg spi_ncs;
    wire led_red, led_green, led_blue;
    integer p;
    integer b, lit;

    // provide behavioral SP256K (same as previous tb)
    module SP256K(
        input  wire [13:0] AD,
        input  wire [15:0] DI,
        input  wire [3:0] MASKWE,
        input  wire WE,
        input  wire CS,
        input  wire CK,
        input  wire STDBY,
        input  wire SLEEP,
        input  wire PWROFF_N,
        output reg [15:0] DO
    );
        reg [15:0] mem [0:16383];
        always @(posedge CK) begin
            if (WE) mem[AD] <= DI;
            DO <= mem[AD];
        end
    endmodule

    // instantiate top
    camera_line_follower_top top (
        .nreset(nreset),
        .cam_pclk(cam_pclk),
        .cam_vsync(cam_vsync),
        .cam_href(cam_href),
        .cam_data(cam_data),
        .spi_sck(spi_sck),
        .spi_mosi(spi_mosi),
        .spi_miso(spi_miso),
        .spi_ncs(spi_ncs),
        .led_red(led_red),
        .led_green(led_green),
        .led_blue(led_blue)
    );

    initial begin
        $display("TB: top integration start");
        // init
        nreset = 0; cam_pclk = 0; cam_vsync = 1; cam_href = 0; cam_data = 0;
        spi_sck = 0; spi_mosi = 0; spi_ncs = 1;
        #200 nreset = 1;
        #200;

        // Start short frame (two small lines, 32 Y pixels each)
        cam_vsync = 1; #200; cam_vsync = 0; // falling edge -> start frame
        // line 0
        cam_href = 1;
        
        for (p=0; p<32; p=p+1) begin
            cam_data = (p%2)?8'd200:8'd50; // Y
            #200;
            cam_data = 8'd128; // UV
            #200;
        end
        cam_href = 0;
        #1000;
        // line 1
        cam_href = 1;
        for (p=0; p<32; p=p+1) begin
            cam_data = (p%2)?8'd180:8'd40;
            #200; cam_data = 8'd128; #200;
        end
        cam_href = 0;
        #1000;
        // end frame
        cam_vsync = 1; #200;

        // Wait for buffer_ready (driven on sys_clk domain)
        #5000;
        $display("LEDs: red=%b green=%b blue=%b", led_red, led_green, led_blue);

        // Now simulate MCU reading some bytes via SPI
        spi_ncs = 0;
        
        for (b=0; b<16; b=b+1) begin
            for (lit=7; lit>=0; lit=lit-1) begin
                #50 spi_sck = 1;
                #50 spi_sck = 0;
            end
        end
        spi_ncs = 1;
        #2000;
        $display("TB: top integration done");
        $finish;
    end

    // clocks
    always #200 cam_pclk = ~cam_pclk; // 2.5 MHz style
endmodule
