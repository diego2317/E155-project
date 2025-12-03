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
    {0x11, 0x00, 20, "CLKRC: /1"}, // Prescaler

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


