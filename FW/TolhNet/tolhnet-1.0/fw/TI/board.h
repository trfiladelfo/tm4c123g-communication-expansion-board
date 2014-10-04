// board.h: useful definitions for the TI Tiva™ C Series TM4C123G LaunchPad Evaluation Board (EK-TM4C123GXL)
// featuring an ARM® Cortex™-M4F TM4C123GH6PMI µC (80 MHz, 256 KiB Flash, 32 KiB SRAM, ROM, 2 KiB EEPROM)
// Copyright (C) 2014 Giorgio Biagetti.

#ifndef __BOARD_H
#define __BOARD_H

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>

#define BITBAND(x, b) (((uint32_t) (x) & 0xF0000000) | 0x02000000 | (((uint32_t) (x) & 0x000FFFFF) << 5) | ((b) << 2))

#define BOARD_LED_RED   HWREGBITW(GPIO_PORTF_BASE + (GPIO_PIN_1 << 2), 1)
#define BOARD_LED_BLUE  HWREGBITW(GPIO_PORTF_BASE + (GPIO_PIN_2 << 2), 2)
#define BOARD_LED_GREEN HWREGBITW(GPIO_PORTF_BASE + (GPIO_PIN_3 << 2), 3)

void delay_cs (uint32_t cs);

#endif
