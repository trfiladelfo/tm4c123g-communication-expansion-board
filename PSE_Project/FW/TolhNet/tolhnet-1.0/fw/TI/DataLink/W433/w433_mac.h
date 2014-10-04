// w433_mac.h: wireless data-link layer for the HopeRF RF23B module
// Copyright (C) 2014 Giorgio Biagetti.
// Based on previous work by Francesco Ricciardi, (C) 2013 CEDAR Solutions.

#ifndef __W433_MAC_H
#define __W433_MAC_H

#include <stdint.h>
#include <stdbool.h>

int w433_mac_init (void);
int w433_process_event (void);

extern volatile uint32_t w433_event;

#endif
