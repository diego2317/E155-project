// ----------------------------------------------------------------------------
// Async FIFO Frame Buffer (Double Buffered SPRAM) with Serial Readout
// Target: Lattice iCE40 UP5K (Uses SB_SPRAM256KA)
// ----------------------------------------------------------------------------

module frame_buffer_spram (
    // Write Interface (Camera Domain ~2.5 MHz)
    input  wire        w_clk,          // cam_pclk
    input  wire        w_rst_n,        // nreset
    input  wire        w_en,           // wr_en from threshold module
    input  wire [16:0] w_addr_pixel,   // wr_addr from threshold module (0-76799)
    input  wire        w_data_bit,     // wr_data from threshold module (1-bit)
    input  wire        w_frame_done,   // frame_done pulse from threshold module

    // Read Interface (System Domain 48 MHz + MCU Interface)
    input  wire        r_clk,          // 48 MHz system clock
    input  wire        i_mcu_sck,      // Clock line from MCU (Continuous or Gated)
    output reg         o_mcu_mosi,     // Data line to MCU (Serial Data)
    
    // Status
    output reg         frame_ready     // High while valid frame data is available (Logic High = Valid)
);

    // ------------------------------------------------------------------------
    // 1. Pixel Packing (Camera Domain)
    // ------------------------------------------------------------------------
    reg [15:0] shifter;
    reg        word_write_req;
    reg [15:0] word_data_latch;
    reg [12:0] w_addr_word_latch;

    always @(posedge w_clk or negedge w_rst_n) begin
        if (!w_rst_n) begin
            shifter <= 16'd0;
            word_write_req <= 1'b0;
            word_data_latch <= 16'd0;
            w_addr_word_latch <= 13'd0;
        end else begin
            word_write_req <= 1'b0; 

            if (w_en) begin
                shifter[w_addr_pixel[3:0]] <= w_data_bit;

                if (w_addr_pixel[3:0] == 4'b1111) begin
                    word_write_req <= 1'b1;
                    word_data_latch <= {w_data_bit, shifter[14:0]};
                    w_addr_word_latch <= w_addr_pixel[16:4];
                end
            end
        end
    end

    // ------------------------------------------------------------------------
    // 2. Bank Management (Double Buffering)
    // ------------------------------------------------------------------------
    reg w_bank_sel; 
    
    always @(posedge w_clk or negedge w_rst_n) begin
        if (!w_rst_n) begin
            w_bank_sel <= 1'b0;
        end else if (w_frame_done) begin
            w_bank_sel <= ~w_bank_sel; 
        end
    end

    // ------------------------------------------------------------------------
    // 3. CDC: Write Request -> System Clock
    // ------------------------------------------------------------------------
    reg w_req_toggle;
    always @(posedge w_clk or negedge w_rst_n) begin
        if (!w_rst_n) w_req_toggle <= 1'b0;
        else if (word_write_req) w_req_toggle <= ~w_req_toggle;
    end

    reg [2:0] w_req_sync;
    always @(posedge r_clk) begin
        w_req_sync <= {w_req_sync[1:0], w_req_toggle};
    end

    wire sys_write_trigger = (w_req_sync[2] != w_req_sync[1]);

    reg [12:0] sys_w_addr;
    reg [15:0] sys_w_data;
    reg        sys_w_bank;
    reg        sys_write_en;

    always @(posedge r_clk) begin
        if (sys_write_trigger) begin
            sys_write_en <= 1'b1;
            sys_w_addr   <= w_addr_word_latch; 
            sys_w_data   <= word_data_latch;   
            sys_w_bank   <= w_bank_sel;        
        end else begin
            sys_write_en <= 1'b0;
        end
    end

    // ------------------------------------------------------------------------
    // 4. Read Bank Logic & Serializer (System Domain)
    // ------------------------------------------------------------------------
    
    // -- Bank Synchronization --
    reg [2:0] bank_sync;
    reg       r_bank_sel;
    reg       prev_bank_sync;

    always @(posedge r_clk) begin
        bank_sync <= {bank_sync[1:0], w_bank_sel};
        r_bank_sel <= ~bank_sync[2]; 
        prev_bank_sync <= bank_sync[2];
    end

    // Trigger on Bank Swap (Frame Done)
    wire frame_swap_event = (bank_sync[2] != prev_bank_sync);

    // -- MCU Clock Sampling (CDC) --
    reg [2:0] mcu_sck_sync;
    always @(posedge r_clk) begin
        mcu_sck_sync <= {mcu_sck_sync[1:0], i_mcu_sck};
    end

    wire mcu_sck_falling = (mcu_sck_sync[2:1] == 2'b10); 

    // -- Read Pointers & Data Path --
    reg [12:0] r_addr_word;     // Current word address (0-4799)
    reg [3:0]  bit_idx;         // Current bit index (0-15)
    reg [15:0] output_shift_reg;
    wire [15:0] spram_data_out; 
    reg load_new_word; 
    
    // Flag to indicate we reached the end of the frame
    reg frame_complete;

    always @(posedge r_clk) begin
        if (!w_rst_n) begin
            frame_ready <= 1'b0;
            r_addr_word <= 13'd0;
            bit_idx <= 4'd0;
            o_mcu_mosi <= 1'b0;
            frame_complete <= 1'b0;
        end else begin
            // PRIORITY 1: New Frame Arrived
            // Reset everything immediately, regardless of MCU clock state
            if (frame_swap_event) begin
                r_addr_word <= 13'd0;
                bit_idx <= 4'd0;
                load_new_word <= 1'b1; // Fetch first word
                o_mcu_mosi <= 1'b0;
                
                // Assert Ready Flag
                frame_ready <= 1'b1; 
                frame_complete <= 1'b0;
            end 
            
            // PRIORITY 2: Load Data from SPRAM
            else if (load_new_word) begin
                output_shift_reg <= spram_data_out;
                load_new_word <= 1'b0;
                o_mcu_mosi <= spram_data_out[0]; // Output bit 0 immediately
            end
            
            // PRIORITY 3: Shift Data on MCU Clock
            else if (mcu_sck_falling && !frame_complete) begin
                
                // --- Shifting Logic ---
                if (bit_idx == 4'd15) begin
                    bit_idx <= 4'd0;
                    
                    if (r_addr_word < 13'd4799) begin
                        r_addr_word <= r_addr_word + 1'b1;
                        load_new_word <= 1'b1; // Trigger fetch for next word
                    end else begin
                        // End of Frame Reached
                        frame_complete <= 1'b1;
                        o_mcu_mosi <= 1'b0; // Output 0s as padding
                        frame_ready <= 1'b0; // Drop flag when data is exhausted
                    end
                end else begin
                    bit_idx <= bit_idx + 1'b1;
                    o_mcu_mosi <= output_shift_reg[bit_idx + 1]; 
                end
            end
        end
    end

    // ------------------------------------------------------------------------
    // 5. SPRAM Instantiation & Arbitration
    // ------------------------------------------------------------------------
    wire [13:0] spram_addr;
    wire [15:0] spram_data_in;
    wire        spram_wren;

    assign spram_wren = sys_write_en;
    assign spram_data_in = sys_w_data;
    assign spram_addr = sys_write_en ? {sys_w_bank, sys_w_addr} : {r_bank_sel, r_addr_word};
    
    SP256K spram_inst (
        .AD(spram_addr),      
        .DI(spram_data_in),    
        .MASKWE(4'b1111),        
        .WE(spram_wren),         
        .CS(1'b1),         
        .CK(r_clk),             
        .STDBY(1'b0),
        .SLEEP(1'b0),
        .PWROFF_N(1'b1),           
        .DO(spram_data_out)
    );

endmodule