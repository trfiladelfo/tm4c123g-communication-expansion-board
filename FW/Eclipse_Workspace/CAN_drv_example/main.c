
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

uint32_t count=0;

void CANIntHandler ()
{
	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
	CANIntClear(CAN0_BASE,1);
	CANIntClear(CAN0_BASE,CAN_INT_INTID_STATUS);
	//SysCtlDelay(10000000);
	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	UARTprintf("Into CANIntHandler function!\n\r");
	UARTprintf("Count value = %d \n\r",count);
}


int main(void)
{
	// setup the system clock to run at 80 MHz from the external crystal:
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

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

	//initialize UART console for debugging purposes
	InitConsole();
	//initialize CAN controller
	app_can_init();

	//declaration of CAN msgs
	tCANMsgObject msg; // the CAN message object
	uint32_t msgData; // the message data is four bytes long which we can allocate as an int32
	uint8_t *msgDataPtr = (uint8_t *)&msgData; // make a pointer to msgData so we can access individual bytes
	// Set up of CAN msg object

	msg.ui32MsgID = 1;
	msg.ui32MsgIDMask = 0;
	msg.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
	msg.ui32MsgLen = sizeof(msgDataPtr);
	msg.pui8MsgData = msgDataPtr;

	//
    // Loop forever.
    //
    while(1)
    {

    	msgDataPtr[0]=++count;
    	msgDataPtr[1]=++count;
    	msgDataPtr[2]=++count;
    	msgDataPtr[3]=++count;

    	CANMessageSet(CAN0_BASE, 1, &msg, MSG_OBJ_TYPE_TX); // send as msg object 1
    	SysCtlDelay(50000000);
    	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_1);
    	SysCtlDelay(10000000);
    	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_2);
    	SysCtlDelay(10000000);
    	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_3);
    	SysCtlDelay(10000000);
    }

return 0;
}
