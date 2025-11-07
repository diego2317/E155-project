#include "stm32l4xx_hal.h"
#include "board_init.h"
#include "uart_io.h"
#include "xclk.h"
#include "sccb.h"
#include "ov7670.h"

int main(void) {
  HAL_Init();
  Board_SystemClock_Config();
  UART2_Init();
  Board_InitLED();
  XCLK_Start(10000000u, 0.5f);

  sccb_t bus = {0};
  SCCB_Init_I2C1(&bus, 0x10909CEC);   // 100 kHz @ 80 MHz
  bus.dev7 = OV7670_I2C_ADDR_7B;

  ov7670_t cam = {0};
  ov7670_id_t id = {0};

  printf("\nOV7670 bring-up\n");
  if (OV7670_ReadID(&cam, &id) == HAL_OK) {
    printf("PID=0x%02X VER=0x%02X\n", id.pid, id.ver);
  } else {
    printf("ReadID failed\n");
  }

  if (OV7670_Init(&cam, &bus) == HAL_OK) {
    printf("Camera configured QVGA YUV422\n");
  } else {
    printf("Config failed\n");
  }

  while (1) {
    Board_LED_Toggle();
    HAL_Delay(500);
  }
}
