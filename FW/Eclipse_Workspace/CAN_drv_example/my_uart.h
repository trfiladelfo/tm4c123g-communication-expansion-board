/*
 * my_uart.h
 *
 *  Created on: Jan 7, 2015
 *      Author: luca
 *      Comment: Only for debug purposes on TM4C123G_CommExpBoard!
 */

#ifndef __MY_UART_H
#define __MY_UART_H

#include <stdint.h>
//#include "uartstdio.h"

//initialize UART1 peripheral (PB0=U1RX, PB1=U1TX)
//On TM4C123G_CommExpBoard, pin-out is:
//U1TX = UART1, pin 3
//U1RX = UART1, pin 5
void InitConsole(void);

#endif /* __MY_UART_H */
