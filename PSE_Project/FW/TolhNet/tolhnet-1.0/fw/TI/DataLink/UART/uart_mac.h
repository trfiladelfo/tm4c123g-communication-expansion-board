// uart_mac.h: UART data-link layer
// Copyright (C) 2014 Giorgio Biagetti.
// Based on previous work by Francesco Ricciardi, (C) 2013 CEDAR Solutions.

#ifndef __UART_MAC_H
#define __UART_MAC_H

#include <stdint.h>
#include <stdbool.h>

#define UART_PACKET_MAX_LENGTH  250

int uart_mac_init (void);
int uart_receive_packet (int source, uint8_t *uart_rx_buffer, uint8_t rx_len);
int uart_enable_receive (bool en);
int uart_enable_transmit (bool en);

#endif
