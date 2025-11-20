#include "camera_capture.h"

// Definition of shared buffers
uint8_t image_buffer[IMAGE_SIZE_BYTES];
volatile uint32_t pixel_count = 0;

void capture_frame(void)
{
    uint32_t timeout;
    uint32_t gpio_state;
    uint32_t prev_gpio;
    
    // Optimization: Local variables for buffering
    uint8_t *p_buffer = image_buffer; 
    uint8_t current_byte = 0;
    int8_t bit_index = 7; // MSB first (7 down to 0)
    
    // Clear buffer and counter
    // Note: Using memset here is fine as it happens before the time-critical loop
    memset(image_buffer, 0, IMAGE_SIZE_BYTES);
    pixel_count = 0;
    
    // 1. Wait for frame to start (FRAME_ACTIVE goes HIGH)
    timeout = 10000000;
    while (!(GPIOA->IDR & FRAME_ACTIVE_PIN)) {
        if (--timeout == 0) {
            printf("ERROR: Timeout waiting for frame start\r\n");
            return;
        }
    }
   
    // Read initial GPIO state
    prev_gpio = GPIOA->IDR;

    
    // 2. Fast Capture Loop
    // We use a pointer to IDR to ensure the compiler optimizes the read address
    volatile uint32_t *IDR_Reg = &(GPIOA->IDR);

    while (*IDR_Reg & FRAME_ACTIVE_PIN)
    {
        gpio_state = *IDR_Reg;
        
        // Detect PCLK Rising Edge (Current High && Previous Low)
        // We use bitwise logic strictly on registers here
        if ((gpio_state & PCLK_PIN) && !(prev_gpio & PCLK_PIN))
        {
            // Check if data is valid (HREF/Write Enable)
            if (gpio_state & DATA_VALID_PIN)
            {
                // If pixel data is High, set the specific bit in our local register
                if (gpio_state & PIXEL_DATA_PIN) {
                    current_byte |= (1 << bit_index);
                }

                // Move to next bit
                bit_index--;

                // If we have collected 8 bits, flush to RAM
                if (bit_index < 0) {
                    *p_buffer = current_byte; // Write byte to RAM
                    p_buffer++;               // Increment pointer
                    
                    // Reset for next byte
                    current_byte = 0;
                    bit_index = 7;
                    
                    // Safety check to prevent buffer overflow
                    if (p_buffer >= (image_buffer + IMAGE_SIZE_BYTES)) break;
                }
                
                pixel_count++;
            }
        }
        prev_gpio = gpio_state;
    }
}