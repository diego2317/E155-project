/* ov7670.h */
#pragma once
#include "sccb.h"
#include "ov7670_regs.h"

typedef struct {
  sccb_t *bus;
} ov7670_t;

typedef struct { uint8_t pid, ver; } ov7670_id_t;

HAL_StatusTypeDef OV7670_Init(ov7670_t *cam, sccb_t *bus);
HAL_StatusTypeDef OV7670_ReadID(ov7670_t *cam, ov7670_id_t *id);
HAL_StatusTypeDef OV7670_WriteReg(ov7670_t *cam, uint8_t reg, uint8_t val);
HAL_StatusTypeDef OV7670_ReadReg (ov7670_t *cam, uint8_t reg, uint8_t *val);
HAL_StatusTypeDef OV7670_ApplyTable(ov7670_t *cam, const ov7670_regval_t *tbl);
