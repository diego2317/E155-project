/**
  ******************************************************************************
  * @file    stm32l4xx_hal_conf.h
  * @brief   HAL configuration file for Camera Line Follower Project
  ******************************************************************************
  */

#ifndef STM32L4xx_HAL_CONF_H
#define STM32L4xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* ########################## Atomic Operations ############################# */
#if !defined(ATOMIC_SET_BIT)
#define ATOMIC_SET_BIT(REG, BIT)     ((void)__LDREXW((volatile uint32_t *)&(REG)) , \
                                      __STREXW(((__LDREXW((volatile uint32_t *)&(REG))) | (BIT)), (volatile uint32_t *)&(REG)))
#endif

#if !defined(ATOMIC_CLEAR_BIT)
#define ATOMIC_CLEAR_BIT(REG, BIT)   ((void)__LDREXW((volatile uint32_t *)&(REG)) , \
                                      __STREXW(((__LDREXW((volatile uint32_t *)&(REG))) & ~(BIT)), (volatile uint32_t *)&(REG)))
#endif

#if !defined(ATOMIC_MODIFY_REG)
#define ATOMIC_MODIFY_REG(REG, CLEARMSK, SETMASK)  ((void)__LDREXW((volatile uint32_t *)&(REG)) , \
                                                    __STREXW(((__LDREXW((volatile uint32_t *)&(REG))) & ~(CLEARMSK)) | (SETMASK), (volatile uint32_t *)&(REG)))
#endif

/* ########################## Module Selection ############################## */
/**
  * @brief Only enable modules needed for camera project
  */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* ########################## Oscillator Values ############################# */
#if !defined  (HSE_VALUE)
  #define HSE_VALUE    8000000U /*!< Value of the External oscillator in Hz */
#endif

#if !defined  (HSE_STARTUP_TIMEOUT)
  #define HSE_STARTUP_TIMEOUT    100U   /*!< Time out for HSE start up, in ms */
#endif

#if !defined  (MSI_VALUE)
  #define MSI_VALUE    4000000U /*!< Value of the Internal oscillator in Hz*/
#endif

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    16000000U /*!< Value of the Internal oscillator in Hz*/
#endif

#if !defined  (HSI48_VALUE)
  #define HSI48_VALUE   48000000U
#endif

#if !defined  (LSI_VALUE)
  #define LSI_VALUE    32000U
#endif

#if !defined  (LSI_STARTUP_TIME)
  #define LSI_STARTUP_TIME   130U   /*!< Time in us */
#endif

#if !defined  (LSE_VALUE)
  #define LSE_VALUE    32768U /*!< Value of the External oscillator in Hz*/
#endif

#if !defined  (LSE_STARTUP_TIMEOUT)
  #define LSE_STARTUP_TIMEOUT    5000U  /*!< Time out for LSE start up, in ms */
#endif

#if !defined  (EXTERNAL_SAI1_CLOCK_VALUE)
  #define EXTERNAL_SAI1_CLOCK_VALUE    48000U
#endif

#if !defined  (EXTERNAL_SAI2_CLOCK_VALUE)
  #define EXTERNAL_SAI2_CLOCK_VALUE    48000U
#endif

/* ########################### System Configuration ######################### */
#define  VDD_VALUE                    3300U /*!< Value of VDD in mv */
#define  TICK_INT_PRIORITY            0x0FU /*!< tick interrupt priority */
#define  USE_RTOS                     0U
#define  PREFETCH_ENABLE              0U
#define  INSTRUCTION_CACHE_ENABLE     1U
#define  DATA_CACHE_ENABLE            1U

/* ########################## Assert Selection ############################## */
/* Uncomment the line below for debugging */
/* #define USE_FULL_ASSERT    1U */

/* ################## Register callback feature configuration ############### */
#define USE_HAL_I2C_REGISTER_CALLBACKS        0U
#define USE_HAL_SPI_REGISTER_CALLBACKS        0U
#define USE_HAL_TIM_REGISTER_CALLBACKS        0U
#define USE_HAL_UART_REGISTER_CALLBACKS       0U

/* ################## SPI peripheral configuration ########################## */
#define USE_SPI_CRC                   1U

/* Includes ------------------------------------------------------------------*/
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32l4xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32l4xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32l4xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32l4xx_hal_cortex.h"
#endif

#ifdef HAL_EXTI_MODULE_ENABLED
  #include "stm32l4xx_hal_exti.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32l4xx_hal_flash.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
  #include "stm32l4xx_hal_i2c.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32l4xx_hal_pwr.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
  #include "stm32l4xx_hal_spi.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32l4xx_hal_tim.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32l4xx_hal_uart.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT
/**
  * @brief  The assert_param macro is used for function's parameters check.
  */
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

#ifdef __cplusplus
}
#endif

#endif /* STM32L4xx_HAL_CONF_H */