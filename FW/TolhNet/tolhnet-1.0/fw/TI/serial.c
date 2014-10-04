// serial.c
// Copyright (C) 2014 Giorgio Biagetti.

#include "serial.h"
#include "Drivers/uartdma.h"

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_uart.h>
#include "board.h"

const char serial_eol[] = "\r\n";

static const char hexdigits[] = "0123456789ABCDEF";
static char serial_buffer[256];
static uint8_t tail;

static void serial_flush (void)
{
	while (uart_send_frame(0, serial_buffer, tail) < 0) {}
	tail = 0;
}

void serial_send (const char *text)
{
	bool flush = text == serial_eol;
	if (text) while (*text) serial_buffer[tail++] = *text++;
	if (flush) serial_flush();
}

void serial_send_h (const uint8_t *data, uint8_t len)
{
	if (data) while (len--) {
		uint8_t val = *data++;
		serial_buffer[tail++] = hexdigits[(val >> 4) & 0x0F];
		serial_buffer[tail++] = hexdigits[(val >> 0) & 0x0F];
	}
}

void serial_send_16 (const char *prefix, uint16_t val, const char *suffix)
{
	bool flush = suffix == serial_eol;
	if (prefix) while (*prefix) serial_buffer[tail++] = *prefix++;
	serial_buffer[tail++] = hexdigits[(val >> 12) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  8) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  4) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  0) & 0x0F];
	if (suffix) while (*suffix) serial_buffer[tail++] = *suffix++;
	if (flush) serial_flush();
}

void serial_send_32 (const char *prefix, uint32_t val, const char *suffix)
{
	bool flush = suffix == serial_eol;
	if (prefix) while (*prefix) serial_buffer[tail++] = *prefix++;
	serial_buffer[tail++] = hexdigits[(val >> 28) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 24) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 20) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 16) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 12) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  8) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  4) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  0) & 0x0F];
	if (suffix) while (*suffix) serial_buffer[tail++] = *suffix++;
	if (flush) serial_flush();
}

void serial_send_48 (const char *prefix, uint64_t val, const char *suffix)
{
	bool flush = suffix == serial_eol;
	if (prefix) while (*prefix) serial_buffer[tail++] = *prefix++;
	serial_buffer[tail++] = hexdigits[(val >> 44) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 40) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 36) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 32) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 28) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 24) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 20) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 16) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >> 12) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  8) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  4) & 0x0F];
	serial_buffer[tail++] = hexdigits[(val >>  0) & 0x0F];
	if (suffix) while (*suffix) serial_buffer[tail++] = *suffix++;
	if (flush) serial_flush();
}

void serial_wait (void)
{
	while (HWREG(BITBAND(UART0_BASE + UART_O_FR, 3)));
}
