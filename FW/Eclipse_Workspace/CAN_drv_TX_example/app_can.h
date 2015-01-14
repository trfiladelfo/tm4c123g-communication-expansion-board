/*
 * app_can.h
 *
 *  Created on: Dec 31, 2014
 *      Author: Luca Buccolini
 */



#ifndef __APP_CAN_H
#define __APP_CAN_H

#include <string.h>
#include <stdint.h>

void app_can_init();

void CANIntHandler(void);

char* app_can_DecodeControllerStsReg(uint32_t Encoded_ControllerStsReg, char* Decoded_ControllerStsReg);

//#include <stdint.h>
//
//extern const char serial_eol[];
//
//void serial_wait (void);
//void serial_send (const char *text);
//void serial_send_h (const uint8_t *data, uint8_t len);
//void serial_send_16 (const char *prefix, uint16_t val, const char *suffix);
//void serial_send_32 (const char *prefix, uint32_t val, const char *suffix);
//void serial_send_48 (const char *prefix, uint64_t val, const char *suffix);



#endif /* __APP_CAN_H */



