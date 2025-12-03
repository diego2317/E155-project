#include "ov7670.h"
#include "i2c.h"
#include "uart.h"   // for //printf via UART
#include "spi.h"
#include <stdio.h>

/* Frame buffer allocated in main.c */
extern uint8_t frame_buffer[SPI_RX_BUFFER_BYTES];

static const camera_reg ov7670_qvga_yuv[] = {
    // --- RESET & CLOCK ---
    {0x12, 0x80, 500, "Reset"},
    {0x11, 0x01, 20, "CLKRC: /1"}, // Prescaler

    // --- FORMATTING & SCALING ---
    {0x12, 0x10, 100, "COM7: QVGA + YUV"}, // Set QVGA (Bit 4) and YUV (Bit 0=0)
    {0x0C, 0x04, 20, "COM3: Scaling"},
    {0x3E, 0x19, 20, "COM14: PCLK scaling"},
    {0x32, 0x80, 10, "HREF"},
    {0x17, 0x16, 10, "HSTART"},
    {0x18, 0x04, 10, "HSTOP"},
    {0x19, 0x02, 10, "VSTART"},
    {0x1A, 0x7A, 10, "VSTOP"},
    {0x03, 0x0A, 10, "VREF"},
    {0x70, 0x3A, 20, "X_SCALING"},
    {0x71, 0x35, 20, "Y_SCALING"},
    {0x72, 0x11, 20, "DCW_SCALING"},
    {0x73, 0xF1, 20, "PCLK_DIV"},
    {0xA2, 0x02, 10, "PCLK_DELAY"},
    {0x15, 0x00, 10, "COM10"},
    {0x3A, 0x00, 10, "TSLB"},
    {0x3D, 0x99, 10, "COM13"}, // Gamma enabled, U/V saturation
    {0x8C, 0x00, 10, "RGB444 disable"},
    {0x40, 0xC0, 10, "COM15: [7:6]=11 (Full range 00-FF)"}, 
    {0x14, 0x49, 10, "COM9: AGC Ceiling"},

    // --- CRITICAL: DISABLE AUTO-MODES FIRST ---
    // Disable AGC (Bit 2), AWB (Bit 1), AEC (Bit 0)
    {0x13, 0x00, 20, "COM8: Everything OFF (Manual Mode)"}, 

    // --- CONTRAST & EDGE ENHANCEMENT (For Binary Thresholding) ---
    // High contrast stretches the histogram.
    {0x56, 0x60, 10, "CONTR: High Contrast"}, 
    // Edge enhancement makes the transition from black to white sharper.
    // Factor range 0x00-0x1F. 0x04 is a moderate boost.
    {0x3F, 0x04, 10, "EDGE: Edge Enhancement Factor"},

    // --- MANUAL EXPOSURE BLOCK ---
    // Adjust AECH (0x10) to shift the entire brightness up/down.
    {0x04, 0x00, 10, "COM1: AEC Low bits"},       
    {0x10, 0x40, 10, "AECH: Exposure Value"},     
    {0x07, 0x00, 10, "AECHH: Exposure High bits"},

    // --- MANUAL GAIN BLOCK ---
    {0x00, 0x08, 10, "GAIN: Fixed Gain"},         

    // --- MANUAL COLOR GAINS (Critical for Y Calculation) ---
    // Y = 0.59G + 0.30R + 0.11B. 
    // We set these high to ensure 'White' light generates a high Y value.
    {0x01, 0x80, 10, "BLUE: Fixed Blue Gain"},    
    {0x02, 0x80, 10, "RED: Fixed Red Gain"},      
    {0x6A, 0x80, 10, "GGAIN: Fixed Green Gain"},  
    
    // --- END ---
    {0xFF, 0xFF, 0, "End"}
};
// Provided configuration setup from implementation guide
static const camera_reg ov7670_qvga_yuv_2[] = {
    {REG_COM7, COM7_RESET},  // Reset
    {0xFF, 100},              // Delay 100ms
    {0x11, 0x01}, // CLKRC
    {REG_COM7, 0x00}, // COM7
    {0x0C, 0x04}, // COM3
    {0x3E, 0x19}, // COM14
    {0x70, 0x3A}, // SCALING_XSC
    {0x71, 0x35}, // SCALING_YSC
    {0x72, 0x11}, // SCALING_DCWCTR
    {0x73, 0xF1}, // SCALING_PCLK_DIV
    {0xA2, 0x02}, // SCALING_PCLK_DELAY
    {0xFF, 0xFF} // End marker
};


// I2C handle comes from bsp_i2c.c
extern I2C_HandleTypeDef hi2c1;


// Read register from OV7670
HAL_StatusTypeDef OV7670_ReadReg(uint8_t reg, uint8_t *value) {
    HAL_StatusTypeDef status;
    
    status = HAL_I2C_Master_Transmit(&hi2c1, OV7670_WRITE_ADDR, &reg, 1, 1000);
    if (status != HAL_OK) return status;
    
    status = HAL_I2C_Master_Receive(&hi2c1, OV7670_READ_ADDR, value, 1, 1000);
    return status;
}

// Write register to OV7670
HAL_StatusTypeDef OV7670_WriteReg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    HAL_StatusTypeDef status;
    
    status = HAL_I2C_Master_Transmit(&hi2c1, OV7670_WRITE_ADDR, data, 2, 1000);
    HAL_Delay(1);
    
    return status;
}

// Initialize QVGA
int OV7670_Init_QVGA(void) {
    //printf("\n----------------------\n");
    //printf("  Configuring OV7670: QVGA YUV422\n");
    //printf("  Resolution: 320x240\n");
    //printf("----------------------\n\n");
    
    int i = 0, success = 0, total = 0;
    
    while (ov7670_qvga_yuv[i].reg != 0xFF) {
        //printf("[%2d] 0x%02X=0x%02X  %-30s ", 
        //       i+1, ov7670_qvga_yuv[i].reg, 
          //     ov7670_qvga_yuv[i].value,
          //     ov7670_qvga_yuv[i].description);
        
        HAL_StatusTypeDef status = OV7670_WriteReg(ov7670_qvga_yuv[i].reg, 
                                                    ov7670_qvga_yuv[i].value);
        
        if (status == HAL_OK) {
            // Verify (except reset)
            if (ov7670_qvga_yuv[i].reg != 0x12 || ov7670_qvga_yuv[i].value != 0x80) {
                uint8_t readback;
                HAL_Delay(5);
                
                if (OV7670_ReadReg(ov7670_qvga_yuv[i].reg, &readback) == HAL_OK) {
                    if (readback == ov7670_qvga_yuv[i].value) {
                        //printf("!!\n");
                    } else {
                        //printf("(read 0x%02X)\n", readback);
                    }
                } else {
                    //printf("!!\n");
                }
            } else {
                //printf("!! (reset)\n");
            }
            success++;
        } else {
            //printf("X FAIL\n");
        }
        
        if (ov7670_qvga_yuv[i].delay_ms > 0) {
            HAL_Delay(ov7670_qvga_yuv[i].delay_ms);
        }
        
        total++;
        i++;
    }
    
    //printf("\n----------------------\n");
    //printf("Summary: %d/%d registers written\n", success, total);
    //printf("----------------------\n\n");
    
    // Verify critical registers
    uint8_t com7, com3, com14, clkrc;
    
    OV7670_ReadReg(0x12, &com7);
    OV7670_ReadReg(0x0C, &com3);
    OV7670_ReadReg(0x3E, &com14);
    OV7670_ReadReg(0x11, &clkrc);
    
    //printf("Critical Registers:\n");
    //printf("----------------------\n");
    //printf("COM7  (0x12) = 0x%02X  %s\n", com7, 
    //       (com7 == 0x10) ? "!!!!!! QVGA MODE!" : "X NOT 0x10");
    //printf("COM3  (0x0C) = 0x%02X  %s\n", com3,
    //       (com3 == 0x04) ? "!! Scaling ON" : "");
    //printf("COM14 (0x3E) = 0x%02X  %s\n", com14,
    //       (com14 == 0x19) ? "!! PCLK scaling" : "");
    //printf("CLKRC (0x11) = 0x%02X  %s\n", clkrc,
    //       (clkrc == 0x01) ? "!! Clock /2" : "");
    //printf("----------------------\n\n");
    
    if (com7 == 0x10) {
        //printf("!!!!!! CONFIGURATION SUCCESS! !!!!!!\n\n");
        return 0;
    } else {
        //printf("X Configuration failed (COM7 wrong)\n\n");
        return -1;
    }
}

// // Show current config
// void Camera_ShowCurrentConfig(void) {
//     //printf("=== Current Configuration ===\n");
    
//     uint8_t val;
//     const uint8_t regs[] = {0x11, 0x12, 0x13, 0x0C, 0x3E, 0x15, 0x40};
//     const char* names[] = {"CLKRC", "COM7", "COM8", "COM3", "COM14", "COM10", "COM15"};
    
//     for (int i = 0; i < 7; i++) {
//         if (OV7670_ReadReg(regs[i], &val) == HAL_OK) {
//             //printf("  %-8s (0x%02X) = 0x%02X\n", names[i], regs[i], val);
//         }
//     }
//     //printf("\n");
// }

// // Configure pins
// void Camera_ControlPins_Init(void) {
//     GPIO_InitTypeDef gpio = {0};
//     __HAL_RCC_GPIOA_CLK_ENABLE();
    
//     gpio.Pin = CAM_VSYNC_PIN | CAM_HREF_PIN | CAM_PCLK_PIN;
//     gpio.Mode = GPIO_MODE_INPUT;
//     gpio.Pull = GPIO_NOPULL;
//     gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//     HAL_GPIO_Init(GPIOA, &gpio);
// }

// // Test signals
// void Camera_SignalTest(void) {
//     //printf("----------------------\n");
//     //printf("  Video Signal Test\n");
//     //printf("----------------------\n\n");
    
//     Camera_ControlPins_Init();
    
//     //printf("Sampling for 300ms...\n");
    
//     int vsync_count = 0, href_count = 0, pclk_count = 0;
//     int prev_vsync = 0, prev_href = 0, prev_pclk = 0;
    
//     uint32_t start = HAL_GetTick();
//     int samples = 0;
    
//     while (HAL_GetTick() - start < 300) {
//         int vsync = HAL_GPIO_ReadPin(CAM_VSYNC_PORT, CAM_VSYNC_PIN);
//         int href = HAL_GPIO_ReadPin(CAM_HREF_PORT, CAM_HREF_PIN);
//         int pclk = HAL_GPIO_ReadPin(CAM_PCLK_PORT, CAM_PCLK_PIN);
        
//         if (vsync != prev_vsync) vsync_count++;
//         if (href != prev_href) href_count++;
//         if (pclk != prev_pclk) pclk_count++;
        
//         prev_vsync = vsync;
//         prev_href = href;
//         prev_pclk = pclk;
//         samples++;
//     }
    
//     //printf("\nResults (%d samples):\n", samples);
//     //printf("----------------------\n");
    
//     //printf("PCLK:  %7d transitions ", pclk_count);
//     if (pclk_count > 5000) {
//         //printf("!!!!!! ACTIVE!\n");
//     } else if (pclk_count > 500) {
//         //printf("⚠ SLOW\n");
//     } else {
//         //printf("X NONE\n");
//     }
    
//     //printf("HREF:  %7d transitions ", href_count);
//     if (href_count > 200) {
//         //printf("!!!!!! ACTIVE!\n");
//     } else if (href_count > 20) {
//         //printf("SLOW\n");
//     } else {
//         //printf("X NONE\n");
//     }
    
//     //printf("VSYNC: %7d transitions ", vsync_count);
//     if (vsync_count >= 6) {
//         //printf("!!!!!! ACTIVE!\n");
//     } else if (vsync_count > 0) {
//         //printf("SLOW\n");
//     } else {
//         //printf("X NONE\n");
//     }
    
//     //printf("----------------------\n");
    
//     if (pclk_count > 5000 && href_count > 200) {
//         //printf("\n!!!!!! VIDEO OUTPUT DETECTED! !!!!!!\n");
//         //printf("Camera is working - ready for FPGA!\n\n");
//     } else if (pclk_count == 0 && href_count == 0) {
//         //printf("\nX NO VIDEO OUTPUT\n\n");
//         //printf("Hardware checklist:\n");
//         //printf("  [ ] PWDN pin grounded or floating?\n");
//         //printf("  [ ] XCLK actually reaching camera sensor?\n");
//         //printf("  [ ] Power stable (measure with meter)?\n");
//         //printf("  [ ] All GND connections solid?\n\n");
//     } else {
//         //printf("\n⚠ PARTIAL OUTPUT\n");
//         //printf("Camera responding but output is weak\n\n");
//     }
    
//     //printf("----------------------\n\n");
// }

// // Measure frame rate based on full frames received over SPI (DMA)
// void Camera_MeasureFrameRate(void)
// {
//     //printf("Measuring frame rate from SPI stream (3 seconds).\n");

//     const uint32_t window_ms = 3000U;
//     uint32_t start_ms = HAL_GetTick();
//     int frames = 0;

//     while ((HAL_GetTick() - start_ms) < window_ms) {
//         /* Start DMA RX for one full 1-bit QVGA frame (9600 bytes) */
//         if (SPI1_Receive_DMA(frame_buffer, SPI_RX_BUFFER_BYTES) != HAL_OK) {
//             //printf("X SPI DMA start failed\n");
//             break;
//         }

//         bool timeout = false;

//         /* Wait for this frame's DMA transfer to complete or time window to expire */
//         while (!spi_rx_full_complete && !spi_rx_error) {
//             if ((HAL_GetTick() - start_ms) >= window_ms) {
//                 timeout = true;
//                 SPI1_Stop_DMA();
//                 break;
//             }
//         }

//         if (timeout) {
//             /* Measurement window ended mid-frame; do not count this one */
//             break;
//         }

//         if (spi_rx_error) {
//             //printf("X SPI RX error during frame-rate measurement\n");
//             break;
//         }

//         /* A full SPI frame has been received */
//         frames++;
//     }

//     uint32_t elapsed_ms = HAL_GetTick() - start_ms;
//     if (elapsed_ms == 0U) {
//         elapsed_ms = 1U;  // avoid divide-by-zero
//     }

//     float fps = (frames * 1000.0f) / (float)elapsed_ms;

//     //printf("Result: %d frames in %lu ms = %.1f fps\n",
//            frames, (unsigned long)elapsed_ms, fps);

//     if (frames == 0) {
//         //printf("X No frames received over SPI\n\n");
//     } else if (fps >= 45.0f && fps <= 90.0f) {
//         //printf("!! Frame rate is good!\n\n");
//     } else {
//         //printf("⚠ Unusual frame rate\n\n");
//     }
// }


// // Measure frame rate
// void Camera_MeasureFrameRate_VSYNC(void) {
//     //printf("Measuring frame rate (3 seconds)...\n");
    
//     int frames = 0;
//     int prev = HAL_GPIO_ReadPin(CAM_VSYNC_PORT, CAM_VSYNC_PIN);
//     uint32_t start = HAL_GetTick();
    
//     while (HAL_GetTick() - start < 3000) {
//         int curr = HAL_GPIO_ReadPin(CAM_VSYNC_PORT, CAM_VSYNC_PIN);
//         if (curr && !prev) frames++;
//         prev = curr;
//     }
    
//     //printf("Result: %d frames in 3s = %.1f fps\n", frames, frames/3.0f);
    
//     if (frames >= 45 && frames <= 90) {
//         //printf("!! Frame rate is good!\n\n");
//     } else if (frames > 0) {
//         //printf("⚠ Unusual frame rate\n\n");
//     } else {
//         //printf("X No frames\n\n");
//     }
// }

// // Verify YUV output format
// void Camera_VerifyFormat(void) {
//     //printf("----------------------\n");
//     //printf("  Format Verification\n");
//     //printf("----------------------\n\n");
    
//     uint8_t com7, com15, com13, tslb;
    
//     OV7670_ReadReg(0x12, &com7);
//     OV7670_ReadReg(0x40, &com15);
//     OV7670_ReadReg(0x3D, &com13);
//     OV7670_ReadReg(0x3A, &tslb);
    
//     //printf("Format Control Registers:\n");
//     //printf("----------------------\n");
//     //printf("COM7  (0x12) = 0x%02X\n", com7);
//     //printf("  Bit 2 (RGB) = %d  → %s\n", 
//            (com7 >> 2) & 1,
//            ((com7 >> 2) & 1) ? "RGB" : "YUV");
//     //printf("  Bit 4 (QVGA)= %d  → %s\n",
//            (com7 >> 4) & 1,
//            ((com7 >> 4) & 1) ? "QVGA (320x240)" : "VGA (640x480)");
    
//     //printf("\nCOM15 (0x40) = 0x%02X\n", com15);
//     //printf("  Bits 7-6 = %d  → ", (com15 >> 6) & 3);
//     switch ((com15 >> 6) & 3) {
//         case 0: //printf("Output range varies\n"); break;
//         case 1: //printf("Reserved\n"); break;
//         case 2: //printf("Output range [16-235]\n"); break;
//         case 3: //printf("Output range [0-255] (Full)\n"); break;
//     }
//     //printf("  Bit 4 (RGB565) = %d\n", (com15 >> 4) & 1);
    
//     //printf("\nTSLB  (0x3A) = 0x%02X\n", tslb);
//     //printf("  Bit 3 (UV order) = %d  → %s\n",
//            (tslb >> 3) & 1,
//            ((tslb >> 3) & 1) ? "UYVY":  "YUYV");
    
//     //printf("----------------------\n\n");
    
//     // Determine format
//     int is_rgb = (com7 >> 2) & 1;
//     int is_rgb565 = (com15 >> 4) & 1;
//     int yuv_order = (tslb >> 3) & 1;
    
//     //printf("DETECTED FORMAT:\n");
//     if (is_rgb) {
//         if (is_rgb565) {
//             //printf("  → RGB565 (16-bit)\n");
//         } else {
//             //printf("  → RGB444 or RGB555\n");
//         }
//     } else {
//         //printf("  → YUV422 (%s byte order)\n", 
//                yuv_order ? "YUYV" : "UYVY");
//         //printf("\n");
//         //printf("YUV422 Format Details:\n");
//         if (!yuv_order) {
//             //printf("  Pixel order: Y0 U0 Y1 V0 (YUYV)\n");
//             //printf("  For 2 pixels: [Y0][U][Y1][V]\n");
//         } else {
//             //printf("  Pixel order: U0 Y0 V0 Y1 (UYVY)\n");
//             //printf("  For 2 pixels: [U][Y0][V][Y1]\n");
//         }
//         //printf("\n");
//         //printf("Expected data stream:\n");
//         //printf("  - 2 bytes per pixel (4:2:2 subsampling)\n");
//         //printf("  - 320x240 = 76,800 pixels\n");
//         //printf("  - 153,600 bytes per frame\n");
//     }
    
//     //printf("\n----------------------\n\n");
// }

