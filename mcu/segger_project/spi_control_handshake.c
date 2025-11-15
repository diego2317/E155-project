#include "spi_control_handshake.h"
#include "spi.h"

static bool is_valid_ctx(const SpiControlHandshake *ctx) {
    return (ctx != NULL) && (ctx->buffer != NULL) && (ctx->length > 0U);
}

void SpiControlHandshake_Init(SpiControlHandshake *ctx, uint8_t *buffer, uint16_t length)
{
    if (ctx == NULL) {
        return;
    }

    ctx->buffer = buffer;
    ctx->length = length;
    ctx->phase = SPI_CTRL_HANDSHAKE_IDLE;
    ctx->midpoint_pixel = 0U;
    ctx->midpoint_valid = false;
}

HAL_StatusTypeDef SpiControlHandshake_BeginCapture(SpiControlHandshake *ctx)
{
    if (!is_valid_ctx(ctx)) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status = SPI1_Receive_DMA(ctx->buffer, ctx->length);
    if (status == HAL_OK) {
        ctx->phase = SPI_CTRL_HANDSHAKE_RECEIVING;
    } else {
        ctx->phase = SPI_CTRL_HANDSHAKE_ERROR;
    }
    return status;
}

void SpiControlHandshake_Service(SpiControlHandshake *ctx)
{
    if (!is_valid_ctx(ctx)) {
        return;
    }

    if (spi_rx_half_complete) {
        spi_rx_half_complete = false;
        ctx->midpoint_pixel = ctx->buffer[ctx->length / 2U];
        ctx->midpoint_valid = true;
    }

    if (spi_rx_full_complete) {
        spi_rx_full_complete = false;
        ctx->phase = SPI_CTRL_HANDSHAKE_READY;
    }

    if (spi_rx_error) {
        spi_rx_error = false;
        ctx->phase = SPI_CTRL_HANDSHAKE_ERROR;
        SPI1_Stop_DMA();
    }
}

bool SpiControlHandshake_FrameReady(const SpiControlHandshake *ctx)
{
    return (ctx != NULL) && (ctx->phase == SPI_CTRL_HANDSHAKE_READY);
}

void SpiControlHandshake_ReleaseFrame(SpiControlHandshake *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->phase == SPI_CTRL_HANDSHAKE_READY) {
        ctx->phase = SPI_CTRL_HANDSHAKE_IDLE;
    }
}

bool SpiControlHandshake_InError(const SpiControlHandshake *ctx)
{
    return (ctx != NULL) && (ctx->phase == SPI_CTRL_HANDSHAKE_ERROR);
}

bool SpiControlHandshake_TakeMidpoint(SpiControlHandshake *ctx, uint8_t *pixel)
{
    if ((ctx == NULL) || (pixel == NULL) || !ctx->midpoint_valid) {
        return false;
    }

    ctx->midpoint_valid = false;
    *pixel = ctx->midpoint_pixel;
    return true;
}
