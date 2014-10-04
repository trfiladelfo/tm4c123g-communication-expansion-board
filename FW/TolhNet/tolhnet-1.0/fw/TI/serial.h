// serial.h
// Copyright (C) 2014 Giorgio Biagetti.

#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>

extern const char serial_eol[];

void serial_wait (void);
void serial_send (const char *text);
void serial_send_h (const uint8_t *data, uint8_t len);
void serial_send_16 (const char *prefix, uint16_t val, const char *suffix);
void serial_send_32 (const char *prefix, uint32_t val, const char *suffix);
void serial_send_48 (const char *prefix, uint64_t val, const char *suffix);

#endif
