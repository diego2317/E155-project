// interrupts.h
// Header for interrupt functions
// Author: Diego Weiss
// Email: dweiss@hmc.edu
// 10/8/2025

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "STM32L432KC_GPIO.h"
#include <stm32l432xx.h>  // CMSIS device library include

#define FRAME_READY_PIN PA10


void configureInterrupts(void);

void EXTI9_5_IRQHandler(void);

void SysTick_Handler(void);

#endif