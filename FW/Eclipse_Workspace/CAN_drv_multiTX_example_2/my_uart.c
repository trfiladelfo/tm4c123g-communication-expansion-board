/*
 * my_uart.c
 *
 *  Created on: Jan 7, 2015
 *      Author: luca
 *      Comment: Only for debug purposes on TM4C123G_CommExpBoard!
 */

#include "my_uart.h"

#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"

#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

#include "utils/uartstdio.h"

//*****************************************************************************
//
// This function sets up UART1 to be used for a console to display information
// as the example is running.
//
//*****************************************************************************
void
InitConsole(void)
{
    //
    // Enable GPIO port B which is used for UART1 pins.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Configure the pin muxing for UART1 functions on port B0 and B1.
    // This step is not necessary if your part does not support pin muxing.
    //
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);

    //
    // Enable UART1 so that we can configure the clock.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    //
    // Use the external 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART1_BASE, UART_CLOCK_SYSTEM);

    //
    // Select the alternate (UART) function for these pins.
    //
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART1 for console I/O @ 921600 bps.
    //
    UARTStdioConfig(1, 921600, 80000000);
}


