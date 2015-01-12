
#include <stdint.h>
#include <stdbool.h>

#include "my_uart.h"
#include "app_can.h"

#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"

#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/can.h"

#include "utils/uartstdio.h"


void CANIntHandler ()
{
	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
	SysCtlDelay(10000000);
	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	CANIntClear(CAN0_BASE,CAN_INT_INTID_STATUS);
}


int main(void)
{
	// setup the system clock to run at 80 MHz from the external crystal:
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	// enable peripherals to operate when CPU is in sleep:
	ROM_SysCtlPeripheralClockGating(true);

	// enable all of the GPIOs:
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOB);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOC);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOD);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);

	// setup pins connected to RGB LED:
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

	//setup CAN controller
	app_can_init();

    //
    // Loop forever.
    //
    while(1)
    {
    	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_1);
    	SysCtlDelay(10000000);
    	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_2);
    	SysCtlDelay(10000000);
    	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_3);
    	SysCtlDelay(10000000);
    }
}
