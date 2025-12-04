#ifndef CAMERA_VISION_H
#define CAMERA_VISION_H

#include "main.h"
#include "config.h"
#include "camera_capture.h" // Needs access to image_buffer

uint8_t get_pixel(uint16_t x, uint16_t y);
void visualize_image_compact(void);
void image_to_file(void);
uint32_t count_black_pixels(void);

#endif // CAMERA_VISION_H