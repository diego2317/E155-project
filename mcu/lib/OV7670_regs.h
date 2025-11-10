// Author: Diego Weiss
// Registers for OV7670

#ifndef OV7670_REGS_H
#define OV7670_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------- SCCB/I2C slave addresses -------- */
#define OV7670_ADDR_WRITE 0x42u  /* write */
#define OV7670_ADDR_READ  0x43u  /* read  */

/* -------- Identification -------- */
#define OV7670_REG_PID    0x0Au   /* expected 0x76 */
#define OV7670_REG_VER    0x0Bu   /* expected 0x73 */
#define OV7670_REG_MIDH   0x1Cu   /* 0x7F */
#define OV7670_REG_MIDL   0x1Du   /* 0xA2 */

/* -------- Core analog gain / color gains -------- */
#define OV7670_REG_GAIN   0x00u
#define OV7670_REG_BLUE   0x01u
#define OV7670_REG_RED    0x02u
#define OV7670_REG_VREF   0x03u  /* [7:6]=AGC[9:8], [3:2] Vend LSB, [1:0] Vstart LSB */

/* -------- Exposure -------- */
#define OV7670_REG_COM1   0x04u  /* [1:0]=AEC[1:0] LSB */
#define OV7670_REG_AECHH  0x07u  /* AEC[15:10] */
#define OV7670_REG_AECH   0x10u  /* AEC[9:2] */

/* -------- Common control blocks -------- */
#define OV7670_REG_COM2   0x09u  /* [1:0] output drive */
#define OV7670_REG_COM3   0x0Cu  /* [3]=scale en, [2]=DCW en, [6]=swap MSB/LSB */
#define OV7670_REG_COM4   0x0Du  /* [5:4] AEC window match COM17[7:6] */
#define OV7670_REG_COM5   0x0Eu
#define OV7670_REG_COM6   0x0Fu  /* [1]=reset timing on format change */

/* -------- Internal clock -------- */
#define OV7670_REG_CLKRC  0x11u  /* [6]=ext clk direct; [5:0]=pre-scaler */

/* -------- Output format selection -------- */
#define OV7670_REG_COM7   0x12u
/* COM7 bits */
#define OV7670_COM7_RESET        (1u<<7)
#define OV7670_COM7_FMT_CIF      (1u<<5)
#define OV7670_COM7_FMT_QVGA     (1u<<4)
#define OV7670_COM7_FMT_QCIF     (1u<<3)
#define OV7670_COM7_RGB          (1u<<2)  /* with COM7[0]=0 */
#define OV7670_COM7_COLOR_BAR    (1u<<1)
#define OV7670_COM7_RAW          (1u<<0)  /* Bayer RAW; with COM7[2]=0 */

/* -------- AGC/AWB/AEC and ceilings -------- */
#define OV7670_REG_COM8   0x13u  /* [7]=fast AEC/AGC, [5]=banding, [2]=AGC, [1]=AWB, [0]=AEC */
#define OV7670_REG_COM9   0x14u  /* [6:4]=AGC max gain ceiling */
#define   OV7670_COM9_AGC_2X     (0u<<4)
#define   OV7670_COM9_AGC_4X     (1u<<4)
#define   OV7670_COM9_AGC_8X     (2u<<4)
#define   OV7670_COM9_AGC_16X    (3u<<4)
#define   OV7670_COM9_AGC_32X    (4u<<4)
#define   OV7670_COM9_AGC_64X    (5u<<4)
#define   OV7670_COM9_AGC_128X   (6u<<4)

#define OV7670_REG_COM10  0x15u  /* sync/clock polarities */
/* COM10 bits */
#define OV7670_COM10_PCLK_HB     (1u<<5)  /* PCLK held during H-blank */
#define OV7670_COM10_PCLK_REV    (1u<<4)
#define OV7670_COM10_HREF_REV    (1u<<3)
#define OV7670_COM10_VS_EDGE     (1u<<2)  /* 1=rising */
#define OV7670_COM10_VS_NEG      (1u<<1)
#define OV7670_COM10_HS_NEG      (1u<<0)

/* -------- Windowing -------- */
#define OV7670_REG_HSTART 0x17u
#define OV7670_REG_HSTOP  0x18u
#define OV7670_REG_VSTRT  0x19u
#define OV7670_REG_VSTOP  0x1Au
#define OV7670_REG_HREF   0x32u  /* [5:3] HEND LSB, [2:0] HSTART LSB; [7:6] href edge offset */
#define OV7670_REG_VREF2  0x03u  /* low 2 bits via VREF[3:0] as above */

/* -------- Mirror/Flip -------- */
#define OV7670_REG_MVFP   0x1Eu
#define   OV7670_MVFP_MIRROR     (1u<<5)
#define   OV7670_MVFP_VFLIP      (1u<<4)

/* -------- Test and ordering of YUV bytes -------- */
#define OV7670_REG_TSLB   0x3Au
#define   OV7670_TSLB_NEGATIVE   (1u<<5)
#define   OV7670_TSLB_UV_FIXED   (1u<<4)  /* use MANU/MANV */
#define   OV7670_TSLB_YUYV       (0u<<3)
#define   OV7670_TSLB_YVYU       (1u<<3)  /* with COM13[0] controls swap */
#define   OV7670_TSLB_AUTO_WIN   (1u<<0)

/* -------- More common controls -------- */
#define OV7670_REG_COM11  0x3Bu  /* night mode, banding select */
#define OV7670_REG_COM12  0x3Cu  /* [7]=HREF while VSYNC low option */
#define OV7670_REG_COM13  0x3Du  /* [7]=gamma en, [6]=UV auto adj, [0]=UV swap */
#define OV7670_REG_COM14  0x3Eu  /* scaling/DCW PCLK and divider control */
#define   OV7670_COM14_PCLK_DIV1   0u
#define   OV7670_COM14_PCLK_DIV2   1u
#define   OV7670_COM14_PCLK_DIV4   2u
#define   OV7670_COM14_PCLK_DIV8   3u
#define   OV7670_COM14_PCLK_DIV16  4u
#define   OV7670_COM14_MANUAL_SCAL (1u<<3)
#define   OV7670_COM14_DCWS_PCLK   (1u<<4) /* enable alt PCLK for DCW/scale */

#define OV7670_REG_EDGE   0x3Fu  /* edge enhancement factor */

/* -------- Output range and RGB packing -------- */
#define OV7670_REG_COM15  0x40u
/* COM15 output range */
#define   OV7670_COM15_RANGE_10_F0 (0u<<6)
#define   OV7670_COM15_RANGE_01_FE (2u<<6)
#define   OV7670_COM15_RANGE_00_FF (3u<<6)
/* COM15 RGB555/565 selection (when RGB444[1]==0 and RGB mode) */
#define   OV7670_COM15_RGB565      (1u<<4)
#define   OV7670_COM15_RGB555      (3u<<4)

#define OV7670_REG_RGB444 0x8Cu
#define   OV7670_RGB444_ENABLE     (1u<<1)
#define   OV7670_RGB444_XR_GB      (0u<<0)
#define   OV7670_RGB444_RG_BX      (1u<<0)

/* -------- Auto features, matrix, brightness/contrast -------- */
#define OV7670_REG_COM16  0x41u  /* [3]=AWB gain, [5]=auto edge, [4]=auto denoise */
#define OV7670_REG_COM17  0x42u  /* [3]=DSP color bar, [7:6] AEC window same as COM4 */
#define OV7670_REG_BRIGHT 0x55u
#define OV7670_REG_CONTR  0x56u
#define OV7670_REG_CONTR_CENTER 0x57u
#define OV7670_REG_MTX1   0x4Fu
#define OV7670_REG_MTX2   0x50u
#define OV7670_REG_MTX3   0x51u
#define OV7670_REG_MTX4   0x52u
#define OV7670_REG_MTX5   0x53u
#define OV7670_REG_MTX6   0x54u
#define OV7670_REG_MTXS   0x58u

/* -------- Fixed/Manual gains and PLL -------- */
#define OV7670_REG_GFIX   0x69u  /* per-channel fixed gains */
#define OV7670_REG_GGAIN  0x6Au  /* G channel AWB gain */
#define OV7670_REG_DBLV   0x6Bu  /* PLL and regulator control */
#define   OV7670_DBLV_PLL_BYPASS  (0u<<6)
#define   OV7670_DBLV_PLL_X4      (1u<<6)
#define   OV7670_DBLV_PLL_X6      (2u<<6)
#define   OV7670_DBLV_PLL_X8      (3u<<6)
#define   OV7670_DBLV_REG_BYPASS  (1u<<4)  /* 1=bypass internal 1.8V reg */

/* -------- Scaler / DCW -------- */
#define OV7670_REG_SCALING_XSC     0x70u  /* [7]=test pattern bit0; [6:0]=H scale */
#define OV7670_REG_SCALING_YSC     0x71u  /* [7]=test pattern bit1; [6:0]=V scale */
#define OV7670_REG_SCALING_DCWCTR  0x72u
#define   OV7670_DCW_V_AVG         (1u<<7)
#define   OV7670_DCW_V_ROUND       (1u<<6)
#define   OV7670_DCW_V_DIV2        (1u<<4)
#define   OV7670_DCW_V_DIV4        (2u<<4)
#define   OV7670_DCW_V_DIV8        (3u<<4)
#define   OV7670_DCW_H_AVG         (1u<<3)
#define   OV7670_DCW_H_ROUND       (1u<<2)
#define   OV7670_DCW_H_DIV2        (1u<<0)
#define   OV7670_DCW_H_DIV4        (2u<<0)
#define   OV7670_DCW_H_DIV8        (3u<<0)

#define OV7670_REG_SCALING_PCLK_DIV 0x73u
#define   OV7670_SCAL_PCLK_BYPASS  (1u<<3)
#define   OV7670_SCAL_PCLK_DIV1    0u
#define   OV7670_SCAL_PCLK_DIV2    1u
#define   OV7670_SCAL_PCLK_DIV4    2u
#define   OV7670_SCAL_PCLK_DIV8    3u
#define   OV7670_SCAL_PCLK_DIV16   4u

#define OV7670_REG_SCALING_PCLK_DELAY 0xA2u  /* [6:0] delay */

/* -------- Test patterns via scaler regs -------- */
#define OV7670_TESTPAT_NONE        0u
#define OV7670_TESTPAT_SHIFT1      1u
#define OV7670_TESTPAT_COLOR_BAR   2u
#define OV7670_TESTPAT_FADE2GRAY   3u
/* Use SCALING_XSC[7] and SCALING_YSC[7] as bit0/bit1 */

/* -------- YUV byte order helper with COM13/TSLB -------- */
#define OV7670_YUV_YUYV  0u /* TSLB[3]=0, COM13[0]=0 */
#define OV7670_YUV_YVYU  1u /* TSLB[3]=1, COM13[0]=0 */
#define OV7670_YUV_UYVY  2u /* TSLB[3]=0, COM13[0]=1 */
#define OV7670_YUV_VYUY  3u /* TSLB[3]=1, COM13[0]=1 */

/* -------- Window pixel delay relative to HREF -------- */
#define OV7670_REG_PSHFT  0x1Bu  /* pixel delay */
#define OV7670_REG_PS_DLY_MAX 255u

/* -------- H/V sync timing tweak -------- */
#define OV7670_REG_HSYST  0x30u  /* HSYNC rise delay LSB */
#define OV7670_REG_HSYEN  0x31u  /* HSYNC fall  delay LSB */

/* -------- Gamma curve -------- */
#define OV7670_REG_SLOP   0x7Au
#define OV7670_REG_GAM1   0x7Bu
#define OV7670_REG_GAM2   0x7Cu
#define OV7670_REG_GAM3   0x7Du
#define OV7670_REG_GAM4   0x7Eu
#define OV7670_REG_GAM5   0x7Fu
#define OV7670_REG_GAM6   0x80u
#define OV7670_REG_GAM7   0x81u
#define OV7670_REG_GAM8   0x82u
#define OV7670_REG_GAM9   0x83u
#define OV7670_REG_GAM10  0x84u
#define OV7670_REG_GAM11  0x85u
#define OV7670_REG_GAM12  0x86u
#define OV7670_REG_GAM13  0x87u
#define OV7670_REG_GAM14  0x88u
#define OV7670_REG_GAM15  0x89u

/* -------- Banding filter -------- */
#define OV7670_REG_BD50ST 0x9Du
#define OV7670_REG_BD60ST 0x9Eu
#define OV7670_REG_BD50MAX 0xA5u
#define OV7670_REG_BD60MAX 0xABu

/* -------- Manual UV for test -------- */
#define OV7670_REG_MANU   0x67u
#define OV7670_REG_MANV   0x68u

/* -------- Saturation control -------- */
#define OV7670_REG_SATCTR 0xC9u

/* -------- Defaults helpers -------- */
#define OV7670_DEFAULT_PID 0x76u
#define OV7670_DEFAULT_VER 0x73u

#ifdef __cplusplus
}
#endif

#endif /* OV7670_REGS_H */
