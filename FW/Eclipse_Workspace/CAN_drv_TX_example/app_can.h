/*
 * app_can.h
 *
 *  Created on: Dec 31, 2014
 *      Author: Luca Buccolini
 *
 *  Useful document: 	TivaWare Peripheral Driver Library (SW-TM4C-DRL-UG-2.1.0.12573)
 *  					The paragraph's numbers in following comments are related to this document.
 *
 */



#ifndef __APP_CAN_H
#define __APP_CAN_H

#include <string.h>
#include <stdint.h>

void app_can_init();

void CANIntHandler(void);

char* app_can_DecodeControllerStsReg(uint32_t Encoded_ControllerStsReg, char* Decoded_ControllerStsReg);

#endif /* __APP_CAN_H */
