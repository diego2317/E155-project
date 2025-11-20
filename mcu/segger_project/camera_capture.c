#include "camera_capture.h"

// Definition of shared buffers
uint8_t image_buffer[IMAGE_SIZE_BYTES];
volatile uint32_t pixel_count = 0;

void capture_frame(void)
{
    uint32_t timeout;
    uint32_t gpio_state, prev_gpio;
   
    // Clear buffer and counter
    memset(image_buffer, 0, IMAGE_SIZE_BYTES);
    pixel_count = 0;
    
    // Wait for frame to start (FRAME_ACTIVE goes HIGH)
    timeout = 10000000;
    while (!(GPIOA->IDR & FRAME_ACTIVE_PIN)) {
        if (--timeout == 0) {
            printf("ERROR: Timeout waiting for frame start\r\n");
            return;
        }
    }
   
    // Read initial GPIO state
    prev_gpio = GPIOA->IDR;
    
    // Fast capture loop - read entire GPIO port at once
    while (GPIOA->IDR & FRAME_ACTIVE_PIN)
    {
        gpio_state = GPIOA->IDR;
        // Detect PCLK rising edge
        if ((gpio_state & PCLK_PIN) && !(prev_gpio & PCLK_PIN))
        {
            // Check if data is valid
            if (gpio_state & DATA_VALID_PIN)
            {
                // Calculate position in the buffe
                uint32_t byte_idx = pixel_count >> 3;
                uint32_t bit_idx = 7 - (pixel_count & 0x07); // MSB-first packing
               
                // Read the single bit and pack it into the byte
                if (gpio_state & PIXEL_DATA_PIN) {
                    image_buffer[byte_idx] |= (1 << bit_idx);
                }
               
                pixel_count++;
                if (pixel_count >= IMAGE_SIZE_PIXELS) break;
            }
        }
        prev_gpio = gpio_state;
    }
}