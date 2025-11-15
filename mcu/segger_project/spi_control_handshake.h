#ifndef SPI_CONTROL_HANDSHAKE_H
#define SPI_CONTROL_HANDSHAKE_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32l4xx_hal.h"

typedef enum {
    SPI_CTRL_HANDSHAKE_IDLE = 0,
    SPI_CTRL_HANDSHAKE_RECEIVING,
    SPI_CTRL_HANDSHAKE_READY,
    SPI_CTRL_HANDSHAKE_ERROR
} SpiControlHandshakePhase;

typedef struct {
    uint8_t *buffer;
    uint16_t length;
    volatile SpiControlHandshakePhase phase;
    uint8_t midpoint_pixel;
    bool midpoint_valid;
} SpiControlHandshake;

void SpiControlHandshake_Init(SpiControlHandshake *ctx, uint8_t *buffer, uint16_t length);
HAL_StatusTypeDef SpiControlHandshake_BeginCapture(SpiControlHandshake *ctx);
void SpiControlHandshake_Service(SpiControlHandshake *ctx);
bool SpiControlHandshake_FrameReady(const SpiControlHandshake *ctx);
void SpiControlHandshake_ReleaseFrame(SpiControlHandshake *ctx);
bool SpiControlHandshake_InError(const SpiControlHandshake *ctx);
bool SpiControlHandshake_TakeMidpoint(SpiControlHandshake *ctx, uint8_t *pixel);

#endif // SPI_CONTROL_HANDSHAKE_H
