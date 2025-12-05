#include "camera_vision.h"
#include <stdio.h>
#include "SEGGER_RTT.h"

// Get pixel value at (x, y) - returns 0 or 1
uint8_t get_pixel(uint16_t x, uint16_t y)
{
    if (x >= IMAGE_WIDTH || y >= IMAGE_HEIGHT) return 0;
    uint32_t pixel_index = y * IMAGE_WIDTH + x;
    uint32_t byte_idx = pixel_index >> 3;
    uint32_t bit_idx = 7 - (pixel_index & 0x07);
   
   return (image_buffer[byte_idx] >> bit_idx) & 0x01;
}

void visualize_image_compact(void)
{
    ////printf("=== COMPACT VIEW (Center Rows) ===\r\n\r\n");
    uint16_t start_row = 0;
    uint16_t end_row = IMAGE_HEIGHT;
    uint32_t black_pixels = 0;
    uint32_t white_pixels = 0;
    for (uint16_t y = start_row; y < end_row; y += 1) {
        ////printf("%3d: ", y);
        for (uint16_t x = 0; x < IMAGE_WIDTH; x += 1) {
            uint8_t pixel = get_pixel(x, y);
            black_pixels += pixel;
            if(pixel == 0) white_pixels += 1;
            ////printf("%c", pixel ? ' ' : '#');
        }
       
    }
    return white_pixels;
}


void image_to_file(void)
{
    // 1. CRITICAL: Enable Blocking Mode
    // If the 1KB buffer fills up, the STM32 will PAUSE here until the 
    // J-Link reads data. This prevents data loss.
    SEGGER_RTT_ConfigUpBuffer(0, "Terminal", NULL, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);

    // 2. Send PBM Header
    char header[64];
    //int len = snprintf(header, sizeof(header), "P1\n%d %d\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    //SEGGER_RTT_Write(0, header, len);

    // 3. Send Image Data (Row by Row)
    // Buffer size: 320 * 2 chars + newline + null terminator = 642
    char line_buffer[650]; 
    
    for (uint16_t y = 0; y < IMAGE_HEIGHT; y++) 
    {
        int pos = 0;
        for (uint16_t x = 0; x < IMAGE_WIDTH; x++) 
        {
            uint8_t pixel = get_pixel(x, y);
            
            // Unrolling this slightly manually to avoid s//printf overhead in the inner loop
            line_buffer[pos++] = pixel ? '1' : '0';
            line_buffer[pos++] = ' ';
        }
        line_buffer[pos++] = '\n'; // End of row
        
        // Write the full line to RTT
        SEGGER_RTT_Write(0, line_buffer, pos);
    }
    
    // Optional: Add a delimiter or extra newline
    SEGGER_RTT_Write(0, "\n", 1);
}

uint32_t count_white_pixels(void) {
    uint32_t white_pixels = IMAGE_HEIGHT * IMAGE_WIDTH;
    // go through every byte
    for (size_t i = 0; i < IMAGE_SIZE_BYTES; ++i) {
        // check every bit in each byte
        for (size_t j = 0; j < 8; ++j) {
            white_pixels -= ((image_buffer[i] >> j) & 0x01);
        }
    }
    return white_pixels;
}