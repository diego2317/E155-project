// ----------------------------------------------------------------------------
// Ping-Pong SPRAM Buffer (1-bit per pixel)
// ----------------------------------------------------------------------------

module spram_pingpong_1bit (
    input  wire clk,
    input  wire nreset,

    // Camera write interface
    input  wire [16:0] cam_wr_addr,
    input  wire        cam_wr_data,
    input  wire        cam_wr_en,
    input  wire        cam_frame_done,

    // SPI read interface
    input  wire [16:0] spi_rd_addr,
    output reg  [7:0]  spi_rd_data,

    output reg         buffer_ready
);

    reg buffer_select;
    reg buf0_full, buf1_full;

    wire [13:0] spram0_addr, spram1_addr;
    wire [15:0] spram0_data_in, spram1_data_in;
    wire [15:0] spram0_data_out, spram1_data_out;
    wire        spram0_wren, spram1_wren;
    wire        spram0_cs, spram1_cs;

    reg  [3:0]  bit_counter;
    reg  [15:0] packed_data;

    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            bit_counter <= 0;
            packed_data <= 0;
        end else if (cam_wr_en) begin
            packed_data[bit_counter] <= cam_wr_data;
            if (bit_counter == 15)
                bit_counter <= 0;
            else
                bit_counter <= bit_counter + 1;
        end
    end

    wire write_word = (bit_counter == 15) && cam_wr_en;
    wire [13:0] word_addr = cam_wr_addr[16:3];

    // ------------------------------------------------------------------------
    // SPRAM buffer 0
    // ------------------------------------------------------------------------
    SP256K spram_buf0 (
        .AD       (spram0_addr),
        .DI       (spram0_data_in),
        .MASKWE   (4'b1111),
        .WE       (spram0_wren),
        .CS       (spram0_cs),
        .CK       (clk),
        .STDBY    (1'b0),
        .SLEEP    (1'b0),
        .PWROFF_N (1'b1),
        .DO       (spram0_data_out)
    );

    // ------------------------------------------------------------------------
    // SPRAM buffer 1
    // ------------------------------------------------------------------------
    SP256K spram_buf1 (
        .AD       (spram1_addr),
        .DI       (spram1_data_in),
        .MASKWE   (4'b1111),
        .WE       (spram1_wren),
        .CS       (spram1_cs),
        .CK       (clk),
        .STDBY    (1'b0),
        .SLEEP    (1'b0),
        .PWROFF_N (1'b1),
        .DO       (spram1_data_out)
    );

    assign spram0_addr     = buffer_select ? spi_rd_addr[16:3] : word_addr;
    assign spram0_data_in  = packed_data;
    assign spram0_wren     = buffer_select ? 1'b0 : write_word;
    assign spram0_cs       = 1'b1;

    assign spram1_addr     = buffer_select ? word_addr : spi_rd_addr[16:3];
    assign spram1_data_in  = packed_data;
    assign spram1_wren     = buffer_select ? write_word : 1'b0;
    assign spram1_cs       = 1'b1;

    always @(*) begin
        spi_rd_data = buffer_select ? spram0_data_out[15:8] : spram1_data_out[15:8];
    end

    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            buffer_select <= 0;
            buf0_full <= 0;
            buf1_full <= 0;
            buffer_ready <= 0;
        end else if (cam_frame_done) begin
            buffer_ready <= 1;
            buffer_select <= ~buffer_select;
        end
    end
endmodule
