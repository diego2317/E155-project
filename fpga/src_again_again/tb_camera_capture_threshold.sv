// tb_camera_capture_threshold.sv
`timescale 1ns/1ps

module tb_camera_capture_threshold;
    // Testbench signals
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
    
    // Loop counters - must be declared at module level
    integer i, p;
    integer write_count;
    
    // Instantiate DUT
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
    
    // Clock generation: 2.5 MHz -> 400ns period -> 200ns half-period
    initial cam_pclk = 0;
    always #200 cam_pclk = ~cam_pclk;
    
    // Monitor writes
    always @(posedge cam_pclk) begin
        if (wr_en) begin
            $display("Time=%0t: WRITE addr=%0d, data=%b (expect: Y=%0d>128? ? %b), Y_value_d1=%0d", 
                     $time, wr_addr, wr_data, dut.data_d1, (dut.data_d1 > 128), dut.data_d1);
        end
    end
    
    // Main test stimulus
    initial begin
        $display("========================================");
        $display("TB: camera_capture_threshold starting");
        $display("========================================");
        
        // Initialize
        nreset = 0;
        cam_vsync = 1;  // Idle state (HIGH)
        cam_href = 0;
        cam_data = 0;
        threshold = 8'd128;
        write_count = 0;
        
        // Reset pulse
        #500;
        nreset = 1;
        $display("Time=%0t: Reset released", $time);
        
        // Wait a bit before starting frame
        repeat(5) @(posedge cam_pclk);
        
        //===========================================
        // START FRAME: VSYNC falling edge (HIGH->LOW)
        //===========================================
        $display("Time=%0t: Frame start - VSYNC falling", $time);
        @(negedge cam_pclk);  // Change on clock edge for cleaner timing
        cam_vsync = 0;
        
        // Wait a few cycles (simulate frame blanking)
        repeat(3) @(posedge cam_pclk);
        
        //===========================================
        // Generate 2 lines with 32 pixels each
        //===========================================
        for (i = 0; i < 2; i = i + 1) begin
            $display("Time=%0t: Line %0d start", $time, i);
            
            // Assert HREF (line active)
            @(negedge cam_pclk);
            cam_href = 1;
            
            // Generate 32 pixels (64 bytes: Y U Y V pattern)
            // Note: With 2-cycle pipeline, we may get 1-2 extra writes after HREF
            for (p = 0; p < 32; p = p + 1) begin
                // Wait for rising edge, then set Y byte
                @(negedge cam_pclk);
                cam_data = (p % 2) ? 8'd200 : 8'd50;  // Alternating bright/dark
                
                @(posedge cam_pclk);  // DUT samples here
                
                // Wait for next rising edge, then set U/V byte
                @(negedge cam_pclk);
                cam_data = 8'd128;  // Chrominance (ignored by DUT)
                
                @(posedge cam_pclk);  // DUT samples here (but skips)
            end
            
            // De-assert HREF (line end)
            @(negedge cam_pclk);
            cam_href = 0;
            cam_data = 8'hXX;  // Invalid data
            
            $display("Time=%0t: Line %0d end", $time, i);
            
            // Horizontal blanking period
            repeat(10) @(posedge cam_pclk);
        end
        
        //===========================================
        // END FRAME: VSYNC rising edge (LOW->HIGH)
        //===========================================
        repeat(5) @(posedge cam_pclk);
        @(negedge cam_pclk);
        cam_vsync = 1;
        $display("Time=%0t: Frame end - VSYNC rising", $time);
        
        // Wait for frame_done pulse
        repeat(10) @(posedge cam_pclk);
        
        //===========================================
        // Check results
        //===========================================
        $display("========================================");
        $display("TEST RESULTS:");
        $display("========================================");
        $display("Final wr_addr   = %0d (expected 64 for 2 lines × 32 pixels)", wr_addr);
        $display("frame_done      = %b", frame_done);
        $display("threshold       = %0d", threshold);
        $display("");
        
        // Expected results:
        // - 2 lines × 32 pixels = 64 Y values
        // - Even pixels (50 < 128) -> wr_data = 0
        // - Odd pixels (200 > 128) -> wr_data = 1
        // - Final wr_addr should be exactly 64
        
        if (wr_addr == 17'd64) begin
            $display("? PASS: Address counter correct (%0d pixels written)", wr_addr);
        end else begin
            $display("? FAIL: Address counter = %0d, expected 64", wr_addr);
        end
        
        $display("========================================");
        $display("TB: Simulation complete");
        $display("========================================");
        $finish;
    end
    
    // Watchdog timer
    initial begin
        #500000;  // 500us timeout
        $display("ERROR: Testbench timeout!");
        $finish;
    end

endmodule