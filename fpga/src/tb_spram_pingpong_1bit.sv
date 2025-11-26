// tb_spram_pingpong_1bit.sv
`timescale 1ns/1ps
module tb_spram_pingpong_1bit;
    reg sys_clk;
    reg cam_clk;
    reg nreset;

    // camera write side
    reg [16:0] cam_wr_addr;
    reg cam_wr_data;
    reg cam_wr_en;
    reg cam_frame_done;

    // spi read side
    reg [16:0] spi_rd_addr;
    wire [7:0] spi_rd_data;
    wire buffer_ready;
    reg frame_read_complete;

    // ------------------------------------------------------------------
    // Provide a behavioral SP256K for simulation (simple 16-bit memory)
    // The actual SP256K primitive in your module is named SP256K; providing
    // a module with same name causes Verilog to bind to this behavioral model.
    // ------------------------------------------------------------------
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
        // small mem for simulation
        reg [15:0] mem [0:16383];
        always @(posedge CK) begin
            if (WE) mem[AD] <= DI;
            DO <= mem[AD];
        end
    endmodule

    // instantiate DUT
    spram_pingpong_1bit dut (
        .sys_clk(sys_clk),
        .cam_clk(cam_clk),
        .nreset(nreset),
        .cam_wr_addr(cam_wr_addr),
        .cam_wr_data(cam_wr_data),
        .cam_wr_en(cam_wr_en),
        .cam_frame_done(cam_frame_done),
        .spi_rd_addr(spi_rd_addr),
        .spi_rd_data(spi_rd_data),
        .buffer_ready(buffer_ready),
        .frame_read_complete(frame_read_complete)
    );
    integer w, b;
    initial begin
        $display("TB: spram_pingpong_1bit starting");
        // clocks
        sys_clk = 0;
        cam_clk = 0;
        nreset = 0;
        cam_wr_addr = 0;
        cam_wr_data = 0;
        cam_wr_en = 0;
        cam_frame_done = 0;
        spi_rd_addr = 0;
        frame_read_complete = 0;
        #200 nreset = 1;
        #200;

        // Simulate writing 3 words (each word needs 16 cam_wr_en pulses)
        
        for (w=0; w<3; w=w+1) begin
            for (b=0; b<16; b=b+1) begin
                // create a pixel bit (alternate)
                cam_wr_data = (w ^ b) & 1;
                cam_wr_en = 1;
                // drive cam_wr_addr to follow pixel index: each Y writes increment addr
                cam_wr_addr = (w*16 + b); // pixel index
                #500; // wait one cam_clk posedge (cam_clk toggles below)
                cam_wr_en = 0;
                #500;
            end
        end

        // pulse frame done
        cam_frame_done = 1;
        #500; cam_frame_done = 0;
        #2000;

        if (buffer_ready) $display("Buffer ready set as expected");

        // read back the first word bytes (word index 0 -> spi_rd_addr 0 (lower byte) and 1 (upper byte))
        spi_rd_addr = 0; #1000;
        $display("spi_rd_data @0 = %02h", spi_rd_data);
        spi_rd_addr = 1; #1000;
        $display("spi_rd_data @1 = %02h", spi_rd_data);

        // simulate MCU reading done
        frame_read_complete = 1;
        #100; frame_read_complete = 0;
        #500;

        $display("TB: spram_pingpong_1bit done");
        $finish;
    end

    // clocks
    always #10 sys_clk = ~sys_clk;   // 50 MHz-ish for convenience (not real)
    always #250 cam_clk = ~cam_clk;  // 2 MHz-ish (period 500ns)

endmodule
