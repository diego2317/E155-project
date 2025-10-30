#ifndef OV7670_INIT_H
#define OV7670_INIT_H

#include <stdint.h>
#include "ov7670_regs.h"

/* simple reg-value pair */
typedef struct { uint8_t reg; uint8_t val; } ov7670_regval_t;

/* YUV422 QVGA, YUYV byte order, full range.
 * - QVGA select via COM7[4]=1, YUV mode with COM7[2]=0 and COM7[0]=0. 0x10 total.
 * - YUYV order requires TSLB[3]=0 and COM13[0]=0 (default 0x88 has COM13[0]=0).
 * - Full 0–255 range via COM15[7:6]=11.
 * - Pixel clock prescale example: CLKRC=0x01 → Fint=Fxclk/(1+1). Adjust as needed.
 * - PLL x4 via DBLV=0x0A keeps internal regulator enabled.
 */
static const ov7670_regval_t OV7670_INIT_YUV422_QVGA[] = {
    { OV7670_REG_COM7,  OV7670_COM7_RESET },   /* soft reset; delay ~10ms in caller */
    { OV7670_REG_CLKRC, 0x01 },                /* prescale = 2 → Fint = Fxclk/2 */
    { OV7670_REG_DBLV,  0x0A },                /* PLL x4, regulator enabled */
    { OV7670_REG_COM7,  OV7670_COM7_FMT_QVGA },/* QVGA + YUV (RGB=0, RAW=0) */
    { OV7670_REG_TSLB,  0x00 },                /* YUYV with COM13[0]=0 */
    { OV7670_REG_COM13, 0x88 },                /* gamma en + UV auto; UV swap=0 */
    { OV7670_REG_COM15, 0xC0 },                /* output range [00..FF] */
    /* Optional: keep PCLK free-running during H-blank or not */
    /* { OV7670_REG_COM10, OV7670_COM10_PCLK_HB }, */
    { 0xFF, 0xFF }                             /* end marker */
};

/* RGB565 QVGA, full range.
 * - Set RGB via COM7[2]=1 with RAW=0, plus QVGA select bit. COM7 = 0x14.
 * - RGB565 requires COM15[5:4]=01 and RGB444[1]=0. Also set full range.
 * - Same clocking as above: adjust CLKRC/DBLV to your XCLK.
 */
static const ov7670_regval_t OV7670_INIT_RGB565_QVGA[] = {
    { OV7670_REG_COM7,   OV7670_COM7_RESET },  /* soft reset; delay in caller */
    { OV7670_REG_CLKRC,  0x01 },               /* prescale = 2 */
    { OV7670_REG_DBLV,   0x0A },               /* PLL x4 */
    { OV7670_REG_COM7,   (OV7670_COM7_FMT_QVGA | (1u<<2)) }, /* QVGA + RGB */
    { OV7670_REG_RGB444, 0x00 },               /* ensure RGB444 disabled so COM15 works */
    { OV7670_REG_COM15,  0xD0 },               /* full range + RGB565 (11xx0000b with 01 in [5:4]) */
    { 0xFF, 0xFF }                              /* end marker */
};

#endif /* OV7670_INIT_H */
