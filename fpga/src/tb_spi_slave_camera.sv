// tb_spi_slave_camera.sv
`timescale 1ns/1ps
module tb_spi_slave_camera;
    reg clk;
    reg nreset;
    reg spi_sck;
    reg spi_mosi;
    wire spi_miso;
    reg spi_ncs;

    logic [7:0] mem [0:1023];
    integer i;
    integer b, lit;
    initial for (i=0;i<1024;i=i+1) mem[i] = i[7:0];

    wire [16:0] spram_rd_addr;
    wire [7:0] spram_rd_data;
    reg buffer_ready;
    wire frame_read_complete;
    reg [7:0] recv_byte;
    assign spram_rd_data = mem[spram_rd_addr];

    spi_slave_camera dut (
        .clk(clk),
        .nreset(nreset),
        .spi_sck(spi_sck),
        .spi_mosi(spi_mosi),
        .spi_miso(spi_miso),
        .spi_ncs(spi_ncs),
        .spram_rd_addr(spram_rd_addr),
        .spram_rd_data(spram_rd_data),
        .buffer_ready(buffer_ready),
        .frame_read_complete(frame_read_complete)
    );

    initial begin
        $display("TB: spi_slave_camera starting");
        clk = 0; nreset = 0; spi_sck = 0; spi_mosi = 0; spi_ncs = 1; buffer_ready = 0;
        #200; nreset = 1;
        #200;

        // Mark a buffer ready (allow transfer)
        buffer_ready = 1;
        #200;

        // Activate CS and clock out 8 bytes (example)
        spi_ncs <= 0;
        
        
        for (b=0;b<8;b=b+1) begin
            recv_byte = 8'd0;
            for (lit=7; lit>=0; lit=lit-1) begin
                // SPI mode 0: master toggles SCK: sample MISO on rising (but DUT outputs on falling)
                // We drive SCK high, then low to create falling-edge-driven shift
                #50; spi_sck = 1;
                #50; // allow synched domain to detect rising
                // read MISO after rising? The DUT updates MISO on falling edges; read at next half-phase
                #50; spi_sck = 0;
                // capture lit (master would sample on rising; but we check on rising of next cycle)
                recv_byte = {recv_byte[6:0], spi_miso};
            end
            $display("Received byte %0d = %02h", b, recv_byte);
        end
        spi_ncs <= 1;
        #500;
        $display("End: spram_rd_addr=%0d frame_read_complete=%b", spram_rd_addr, frame_read_complete);
        $finish;
    end

    always #10 clk = ~clk;
endmodule
