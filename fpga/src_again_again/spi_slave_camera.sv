// ----------------------------------------------------------------------------
// SPI Slave Controller
// Runs on sys_clk (48 MHz), interfaces with MCU SPI
// ----------------------------------------------------------------------------
module spi_slave_camera (
    input  wire clk,                // System clock (48 MHz)
    input  wire nreset,
   
    // SPI interface (from MCU)
    input  wire spi_sck,            // SPI clock (much slower than sys_clk)
    input  wire spi_mosi,           // Master out, slave in (unused)
    output reg  spi_miso,           // Master in, slave out
    input  wire spi_ncs,            // Chip select (active low)
   
    // SPRAM read interface (sys_clk domain)
    output reg [16:0] spram_rd_addr, // Byte address (0-9599 for 9600 bytes)
    input  wire [7:0] spram_rd_data, // Data from SPRAM
   
    // Control
    input  wire buffer_ready,       // Frame ready to read
    output reg  frame_read_complete // Pulse when all bytes sent
);

    // ========================================================================
    // Synchronizers for SPI signals (external async inputs)
    // ========================================================================
   
    reg sck_sync1, sck_sync2, sck_sync3, sck_sync4;
    reg ncs_sync1, ncs_sync2, ncs_sync3;
   
    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            sck_sync1 <= 0;
            sck_sync2 <= 0;
            sck_sync3 <= 0;
            sck_sync4 <= 0;
            ncs_sync1 <= 1;
            ncs_sync2 <= 1;
            ncs_sync3 <= 1;
        end else begin
            sck_sync1 <= spi_sck;
            sck_sync2 <= sck_sync1;
            sck_sync3 <= sck_sync2;
            sck_sync4 <= sck_sync3;
           
            ncs_sync1 <= spi_ncs;
            ncs_sync2 <= ncs_sync1;
            ncs_sync3 <= ncs_sync2;
        end
    end
   
    // Edge detection
    wire sck_rising = sck_sync3 && !sck_sync4;
    wire sck_falling = !sck_sync3 && sck_sync4;
    wire ncs_active = !ncs_sync3;
   
    // ========================================================================
    // SPI State Machine
    // ========================================================================
   
    localparam IDLE = 2'b00;
    localparam TRANSFER = 2'b01;
    localparam DONE = 2'b10;
   
    reg [1:0] state, next_state;
    reg [7:0] shift_reg;
    reg [2:0] bit_count;            // 0-7
   
    always @(posedge clk or negedge nreset) begin
        if (!nreset)
            state <= IDLE;
        else
            state <= next_state;
    end
   
    always @(*) begin
        case (state)
            IDLE:     next_state = (ncs_active && buffer_ready) ? TRANSFER : IDLE;
            TRANSFER: next_state = (!ncs_active) ? DONE : TRANSFER;
            DONE:     next_state = IDLE;
            default:  next_state = IDLE;
        endcase
    end
   
    // SPI transfer logic
    always @(posedge clk or negedge nreset) begin
        if (!nreset) begin
            spram_rd_addr <= 0;
            shift_reg <= 0;
            bit_count <= 0;
            spi_miso <= 0;
            frame_read_complete <= 0;
        end else begin
            frame_read_complete <= 0;  // Default: pulse
           
            case (state)
                IDLE: begin
                    if (next_state == TRANSFER) begin
                        // Start new transfer
                        spram_rd_addr <= 0;
                        bit_count <= 0;
                        shift_reg <= spram_rd_data;  // Load first byte
                    end
                end
               
                TRANSFER: begin
                    // Shift out on falling edge
                    if (sck_falling) begin
                        spi_miso <= shift_reg[7];
                        shift_reg <= {shift_reg[6:0], 1'b0};
                        bit_count <= bit_count + 1;
                       
                        // Byte complete
                        if (bit_count == 7) begin
                            bit_count <= 0;
                            spram_rd_addr <= spram_rd_addr + 1;
                           
                            // Check if all bytes sent (9600 bytes total)
                            if (spram_rd_addr >= 9599) begin
                                frame_read_complete <= 1;
                            end
                        end
                    end
                   
                    // Load next byte on rising edge after byte complete
                    if (bit_count == 0 && sck_rising) begin
                        shift_reg <= spram_rd_data;
                    end
                end
               
                DONE: begin
                    spi_miso <= 0;
                end
            endcase
        end
    end

endmodule