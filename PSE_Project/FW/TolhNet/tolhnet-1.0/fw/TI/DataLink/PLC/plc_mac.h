// plc_mac.h: PLC data-link layer for ST7580
// Copyright (C) 2014 Giorgio Biagetti.
// Based on previous work by Francesco Ricciardi, (C) 2013 CEDAR Solutions.

#ifndef __PLC_MAC_H
#define __PLC_MAC_H

#include <stdint.h>
#include <stdbool.h>

/* GENERAL DEFINITIONS */
#define PLC_PACKET_MAX_LENGTH  250

/* FUNCTION DECLARATION */

int plc_mac_init (void);
int plc_receive_packet (int source, uint8_t *uart_rx_buffer, uint8_t rx_len);
int plc_enable_receive (bool en);
int plc_enable_transmit (bool en);

#endif
