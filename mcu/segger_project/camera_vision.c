#include "camera_vision.h"
#include <stdio.h>

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
    int32_t sum_x = 0;
    int32_t pixel_weight = 0;
   
    for (uint16_t x = 0; x < IMAGE_WIDTH; x++) {
        if (get_pixel(x, row)) {
            sum_x += x;
            pixel_weight++;
        }
    }
   
    if (pixel_weight > 5) { 
        return (int16_t)(sum_x / pixel_weight);
    }
    return -1;
}

void visualize_image_compact(void)
{
    printf("=== COMPACT VIEW (Center Rows) ===\r\n\r\n");
    uint16_t start_row = 100;
    uint16_t end_row = 200;
   
    for (uint16_t y = start_row; y < end_row; y += 3) {
        printf("%3d: ", y);
        for (uint16_t x = 0; x < IMAGE_WIDTH; x += 2) {
            uint8_t pixel = get_pixel(x, y);
            printf("%c", pixel ? ' ' : '#');
        }
        printf("\r\n");
    }
    printf("\r\n");
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
        } else {
            printf("NO LINE DETECTED\r\n");
        }
    }
    printf("\r\n");
}