// ----------------------------------------------------------------------------
// SPI Slave Controller for Camera Data Readout
// ----------------------------------------------------------------------------

module spi_slave_camera (
    input  wire clk,
    input  wire nreset,

    // SPI interface
    input  wire spi_sck,
    input  wire spi_mosi,
    output reg  spi_miso,
    input  wire spi_ncs,

    // SPRAM read interface
    output reg [16:0] spram_rd_addr,
    input  wire [7:0] spram_rd_data,

    // Control
    input  wire buffer_ready,
    output reg  frame_read_complete
);

    reg sck_d1, sck_d2, sck_d3;
    reg ncs_d1, ncs_d2;

    always @(posedge clk) begin
        sck_d1 <= spi_sck;
        sck_d2 <= sck_d1;
        sck_d3 <= sck_d2;
        ncs_d1 <= spi_ncs;
        ncs_d2 <= ncs_d1;
    end

    wire sck_rising  = sck_d2 && !sck_d3;
    wire sck_falling = !sck_d2 && sck_d3;
    wire ncs_active  = !ncs_d2;

    reg [7:0] shift_reg;
    reg [2:0] bit_count;

    localparam IDLE = 2'b00;
    localparam TRANSFER = 2'b01;
    localparam DONE = 2'b10;

    reg [1:0] state, next_state;

    always @(posedge clk or negedge nreset) begin
        if (!nreset)
            state <= IDLE;
        else
            state <= next_state;
    end

    always @(*) begin
        case (state)
            IDLE:      next_state = (ncs_active && buffer_ready) ? TRANSFER : IDLE;
            TRANSFER:  next_state = (!ncs_active) ? DONE : TRANSFER;
            DONE:      next_state = IDLE;
            default:   next_state = IDLE;
        endcase
    end

    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            spram_rd_addr <= 0;
            shift_reg <= 0;
            bit_count <= 0;
            spi_miso <= 0;
            frame_read_complete <= 0;
        end else begin
            frame_read_complete <= 0;

            case (state)
                IDLE: if (next_state == TRANSFER) begin
                    spram_rd_addr <= 0;
                    bit_count <= 0;
                    shift_reg <= spram_rd_data;
                end

                TRANSFER: begin
                    if (sck_falling) begin
                        spi_miso <= shift_reg[7];
                        shift_reg <= {shift_reg[6:0], 1'b0};
                        bit_count <= bit_count + 1;

                        if (bit_count == 7) begin
                            bit_count <= 0;
                            spram_rd_addr <= spram_rd_addr + 1;
                            if (spram_rd_addr >= 76799)
                                frame_read_complete <= 1;
                        end
                    end
                    if (bit_count == 0 && sck_rising)
                        shift_reg <= spram_rd_data;
                end

                DONE: begin
                    spi_miso <= 0;
                    spram_rd_addr <= 0;
                end
            endcase
        end
    end
endmodule
