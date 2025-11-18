#include "ov7670.h"
#include "i2c.h"
#include "uart.h"   // for printf via UART
#include <stdio.h>

// Basic QVGA YUV422 configuration
static const camera_reg ov7670_config[] = {
    {REG_COM7, COM7_RESET},  // Reset
    {0xFF, 100},              // Delay 100ms
    {0x11, 0x01},  // Clock prescaler
    {REG_COM7, COM7_YUV | COM7_QVGA}, // corrected QVGA YUV 30 fps format register config in the header file
    {0x0C, 0x04},  // COM3: Enable scaling 
    {0x3E, 0x19},  // COM14: PCLK scaling
    {0x32, 0x80},  // HREF
    {0x17, 0x16},  // HSTART
    {0x18, 0x04},  // HSTOP
    {0x19, 0x02},  // VSTART
    {0x1A, 0x7A},  // VSTOP
    {0x03, 0x0A},  // VREF
    {0x70, 0x3A},  // X scaling
    {0x71, 0x35},  // Y scaling
    {0x72, 0x11},  // Downsample by 2
    {0x73, 0xF1},  // Clock divide
    {0xA2, 0x02},  // PCLK delay
    {0x15, 0x00},  // COM10
    {0x3A, 0x04},  // TSLB
    {0x12, 0x00},  // COM7
    {0x8C, 0x00},  // RGB444
    {0x04, 0x00},  // COM1
    {0x40, 0xC0},  // COM15
    {0x14, 0x48},  // COM9
    {0x4F, 0x80},  // MTX1
    {0x50, 0x80},  // MTX2
    {0x51, 0x00},  // MTX3
    {0x52, 0x22},  // MTX4
    {0x53, 0x5E},  // MTX5
    {0x54, 0x80},  // MTX6
    {0x58, 0x9E},  // MTXS
    {0xFF, 0xFF}   // End marker
};

// Provided configuration setup from implementation guide
static const camera_reg ov7670_qvga_yuv[] = {
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

// Minimal camera communication test
void OV7670_MinimalTest(void) {
    uint8_t pid, ver, com7;
    HAL_StatusTypeDef status;
    
    printf("\n=== OV7670 Camera Test ===\n");
    printf("Testing I2C communication...\n\n");
    
    // Test 1: Read Product ID
    status = OV7670_ReadReg(REG_PID, &pid);
    if (status == HAL_OK) {
        printf("Product ID MSB: 0x%02hhX\n", pid);
        if (pid == 0x76) {
            printf("[OK]\n");
        } else {
            printf("[FAIL - Expected 0x76]\n");
        }
    } else {
        printf("FAILED to read Product ID!\n");
        printf("Check connections:\n");
        printf("  - PB6 -> SIOC (SCL)\n");
        printf("  - PB7 -> SIOD (SDA)\n");
        printf("  - 4.7k pull-ups on SDA/SCL\n");
        printf("  - Camera powered (3.3V)\n");
        return;
    }
    
    // Test 2: Read Version
    status = OV7670_ReadReg(REG_VER, &ver);
    if (status == HAL_OK) {
        printf("Product ID LSB: 0x%02X ", (unsigned int)ver);
        if (ver == 0x73) {
            printf("[OK]\n\n");
        } else {
            printf("[FAIL - Expected 0x73]\n\n");
        }
    } else {
        printf("FAILED to read version!\n\n");
        return;
    }
    
    // Test 3: Read/Write test
    printf("Testing register write...\n");
    OV7670_ReadReg(REG_COM7, &com7);
    printf("  COM7 before: 0x%02X\n", (unsigned int)com7);
    
    OV7670_WriteReg(REG_COM7, 0x10);  // Reset
    HAL_Delay(10);
    
    OV7670_ReadReg(REG_COM7, &com7);
    printf("  COM7 after reset: 0x%02X\n\n", (unsigned int)com7);
    
    printf("=== Test Complete ===\n");
    if (pid == 0x76 && ver == 0x73) {
        printf("SUCCESS! Camera communication working!\n");
        printf("Ready for full configuration.\n\n");
    }
}

// Return -1 if camera init failed,
int  OV7670_QVGA_YUV_Init(void) {
    int i = 0;

    // Write configuration registers
    while (ov7670_qvga_yuv[i].reg != 0xFF || ov7670_qvga_yuv[i].value != 0xFF) {
        if (ov7670_qvga_yuv[i].reg == 0xFF) {
            // Delay marker
            HAL_Delay(ov7670_qvga_yuv[i].value);
        } else {
            if (OV7670_WriteReg(ov7670_qvga_yuv[i].reg, 
                               ov7670_qvga_yuv[i].value) != HAL_OK) {
                return -1;
            }
        }
        i++;
    }

    // read back through registers and verify
    i = 0;
    while (ov7670_qvga_yuv[i].reg != 0xFF || ov7670_qvga_yuv[i].value != 0xFF) {
        if (ov7670_qvga_yuv[i].reg != 0xFF) {  // skip delay markers
            uint8_t read_val = 0;

            if (OV7670_ReadReg(ov7670_qvga_yuv[i].reg, &read_val) != HAL_OK) {
                return -2;  // read error
            }

            if (read_val != ov7670_qvga_yuv[i].value) {
                return -3;  // verify failed
            }
        }
        i++;
    }
    return 0; // success!
}

// Full camera initialization
int OV7670_Init(void) {
    int i = 0;
    
    //printf("\n=== Initializing OV7670 ===\n");
    
    // Write configuration registers
    while (ov7670_config[i].reg != 0xFF || ov7670_config[i].value != 0xFF) {
        if (ov7670_config[i].reg == 0xFF) {
            // Delay marker
            //printf("Delay %d ms...\n", ov7670_config[i].value);
            HAL_Delay(ov7670_config[i].value);
        } else {
            if (OV7670_WriteReg(ov7670_config[i].reg, 
                               ov7670_config[i].value) != HAL_OK) {
                //printf("Failed to write reg 0x%02X\n", ov7670_config[i].reg);
                return -1;
            }
        }
        i++;
    }
    
    //printf("Camera configured! (%d registers written)\n", i);
    
    // Verify configuration
    uint8_t com7;
    OV7670_ReadReg(REG_COM7, &com7);
    printf("COM7 should be 0X010000 \n");
    printf("COM7 = 0x%02hhX (QVGA YUV mode)\n\n", com7);
    
    return 0;
}