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

// Find center of line in given row (Centroid calculation)
int16_t find_line_center(uint16_t row)
{
    if (row >= IMAGE_HEIGHT) return -1;
    int32_t white_pixels_x = 0;
    int32_t pixel_weight = 0;
   
    for (uint16_t x = 0; x < IMAGE_WIDTH; x++) {
        if (get_pixel(x, row)) {
            white_pixels_x += x;
            pixel_weight++;
        }
    }
   
    if (pixel_weight > 5) { 
        return (int16_t)(white_pixels_x / pixel_weight);
    }
    return -1;
}

int32_t visualize_image_compact(void)
{
    //printf("=== COMPACT VIEW (Center Rows) ===\r\n\r\n");
    uint16_t start_row = 0;
    uint16_t end_row = IMAGE_HEIGHT;
    uint32_t white_pixels = 0;
    uint32_t black_pixels = 0;
    for (uint16_t y = start_row; y < end_row; y += 1) {
        //printf("%3d: ", y);
        for (uint16_t x = 0; x < IMAGE_WIDTH; x += 1) {
            uint8_t pixel = get_pixel(x, y);
            white_pixels += pixel;
            if(pixel == 0) black_pixels += 1;
            //printf("%c", pixel ? ' ' : '#');
        }
       
        //printf("\r\n");
    }
    if (black_pixels >= 70000) {
      //printf("BLACK = %d\n", black_pixels);
    }
    if (white_pixels >= 40000 && white_pixels <= 54000) {
      //printf("WHITE = %d\n", white_pixels);
    }
    
    return black_pixels;
    //printf("\r\n");
}

void visualize_image_line_stats(void)
{
    printf("=== LINE DETECTION STATS ===\r\n\r\n");
    uint16_t test_rows[] = {150, 200, 230};
    for (int i = 0; i < 3; i++) {
        uint16_t row = test_rows[i];
        int16_t center = find_line_center(row);
       
        printf("Row %3d: ", row);
        if (center >= 0) {
            int16_t error = center - (IMAGE_WIDTH / 2);
            printf("Line Center X=%3d | Error=%4d\r\n", center, error);
            printf("         [---L---(C)-R---]\r\n");
            if (error < 0) {
              printf("Turn RIGHT");
            } else if (error > 0) {
              printf("Turn LEFT");
            }
        } else {
            printf("NO LINE DETECTED\r\n");
        }
    }
    printf("\r\n");
}
void image_to_file(void)
{
    // 1. CRITICAL: Enable Blocking Mode
    // If the 1KB buffer fills up, the STM32 will PAUSE here until the 
    // J-Link reads data. This prevents data loss.
    SEGGER_RTT_ConfigUpBuffer(0, "Terminal", NULL, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);

    // 2. Send PBM Header
    char header[64];
    int len = snprintf(header, sizeof(header), "P1\n%d %d\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    SEGGER_RTT_Write(0, header, len);

    // 3. Send Image Data (Row by Row)
    // Buffer size: 320 * 2 chars + newline + null terminator = 642
    char line_buffer[650]; 
    
    for (uint16_t y = 0; y < IMAGE_HEIGHT; y++) 
    {
        int pos = 0;
        for (uint16_t x = 0; x < IMAGE_WIDTH; x++) 
        {
            uint8_t pixel = get_pixel(x, y);
            
            // Unrolling this slightly manually to avoid sprintf overhead in the inner loop
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

// Scans the entire image and prints navigation direction
void determine_direction(void) {
    int32_t total_error = 0;
    int32_t valid_rows = 0;
    int16_t center_point = IMAGE_WIDTH / 2;
    
    // Threshold for going forward (deadband)
    // If the line is within +/- 15 pixels of center, we go straight
    const int16_t FORWARD_THRESHOLD = 15;

    // Scan every row in the image
    for (uint16_t y = 0; y < IMAGE_HEIGHT; y++) {
        int16_t line_pos = find_line_center(y);
        
        if (line_pos != -1) {
            // Calculate error for this row
            int16_t error = line_pos - center_point;
            total_error += error;
            valid_rows++;
        }
    }

    // Check if we found a line in enough rows to make a reliable decision
    // Requiring at least 10% of rows to have a line prevents noise triggers
    if (valid_rows < (IMAGE_HEIGHT / 10)) {
        printf("STOP - No Line Detected\r\n");
        return;
    }

    // Calculate the average error across the entire image
    int32_t average_error = total_error / valid_rows;

    printf("Avg Error: %d | ", average_error);

    // Decision Logic
    if (average_error < -FORWARD_THRESHOLD) {
        // Line is to the left (negative error)
        // Using your existing logic: Error < 0 -> Turn RIGHT
        printf("Turn RIGHT\r\n");
    } 
    else if (average_error > FORWARD_THRESHOLD) {
        // Line is to the right (positive error)
        // Using your existing logic: Error > 0 -> Turn LEFT
        printf("Turn LEFT\r\n");
    } 
    else {
        // Error is within the threshold
        printf("Go FORWARD\r\n");
    }
}