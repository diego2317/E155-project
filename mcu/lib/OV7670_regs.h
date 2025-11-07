/* ov7670_regs.h */
#pragma once
#include <stdint.h>

/* 7-bit SCCB address per datasheet: write=0x42, read=0x43 -> dev7=0x21 */
#define OV7670_I2C_ADDR_7B 0x21

enum { OV7670_REG_PID = 0x0A, OV7670_REG_VER = 0x0B, OV7670_REG_COM7 = 0x12 };
#define OV7670_COM7_RESET 0x80
#define OV7670_COM7_QVGA  0x40
#define OV7670_COM7_YUV   0x00

typedef struct { uint8_t reg; uint8_t val; } ov7670_regval_t;

/* special row that encodes a delay */
#define OV7670_TABLE_DELAY 0xFF

extern const ov7670_regval_t ov7670_qvga_yuv[];
