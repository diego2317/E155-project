#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H
#include <stdint.h>
#include "main.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

// Shared data
extern uint8_t image_buffer[IMAGE_SIZE_BYTES];
extern volatile uint32_t pixel_count;

void capture_frame(void);
void capture_frame_spi(void);

#endif