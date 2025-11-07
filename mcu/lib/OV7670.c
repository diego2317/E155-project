/* ov7670.c */
#include "ov7670.h"

static HAL_StatusTypeDef write_tbl(ov7670_t *cam, const ov7670_regval_t *t) {
  for (size_t i=0; !(t[i].reg==0xFF && t[i].val==0xFF); ++i) {
    if (t[i].reg == OV7670_TABLE_DELAY) { HAL_Delay(t[i].val); continue; }
    HAL_StatusTypeDef st = SCCB_Write8(cam->bus, t[i].reg, t[i].val);
    if (st != HAL_OK) return st;
  }
  return HAL_OK;
}

HAL_StatusTypeDef OV7670_Init(ov7670_t *cam, sccb_t *bus) {
  cam->bus = bus;
  return write_tbl(cam, ov7670_qvga_yuv);
}

HAL_StatusTypeDef OV7670_ReadID(ov7670_t *cam, ov7670_id_t *id) {
  HAL_StatusTypeDef st = SCCB_Read8(cam->bus, OV7670_REG_PID, &id->pid);
  if (st != HAL_OK) return st;
  return SCCB_Read8(cam->bus, OV7670_REG_VER, &id->ver);
}

HAL_StatusTypeDef OV7670_WriteReg(ov7670_t *cam, uint8_t reg, uint8_t val) {
  return SCCB_Write8(cam->bus, reg, val);
}
HAL_StatusTypeDef OV7670_ReadReg(ov7670_t *cam, uint8_t reg, uint8_t *val) {
  return SCCB_Read8(cam->bus, reg, val);
}

HAL_StatusTypeDef OV7670_ApplyTable(ov7670_t *cam, const ov7670_regval_t *tbl) {
  return write_tbl(cam, tbl);
}
