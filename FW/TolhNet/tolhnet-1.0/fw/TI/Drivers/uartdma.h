// uartdma.h: UART low-level interface
// Copyright (C) 2014 Giorgio Biagetti.

#ifndef UARTDMA_H
#define UARTDMA_H

#include <stdint.h>
#include <stdbool.h>

int uart_init (unsigned port, uint32_t speed, uint32_t timeout);
int uart_send_frame (unsigned port, void const *data, uint8_t len);
int uart_start_timeout (unsigned port, uint32_t timeout);

extern volatile uint8_t uart_data_length[4];
extern volatile bool uart_data_available[4];
extern uint8_t uart_rx_buffer[4][256];

#endif
