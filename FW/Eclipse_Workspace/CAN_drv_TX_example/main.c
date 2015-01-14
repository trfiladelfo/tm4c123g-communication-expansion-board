
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


#define TURNON_RED_LED()		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_1 )
#define TURNON_BLUE_LED()		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_2 )
#define TURNON_GREEN_LED()		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_3 )

#define TURNON_YELLOW_LED()		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 			\
																						GPIO_PIN_1 | GPIO_PIN_3 )

#define TURNON_MAGENTA_LED()		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 		\
																						GPIO_PIN_1 | GPIO_PIN_2 )

#define TURNON_CYAN_LED()		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 			\
																						GPIO_PIN_2 | GPIO_PIN_3 )

#define TURNON_WHITE_LED()		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,				\
														  	  	 	 	   GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 )
#define TURNOFF_ALL_LED()		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,				\
																											  0 )


uint32_t count=0;
uint8_t err_flag=0;

//declaration of CAN msgs
tCANMsgObject msg; // the CAN message object
uint32_t msgData; // the message data is four bytes long which we can allocate as an int32
uint8_t *msgDataPtr = (uint8_t *)&msgData; // make a pointer to msgData so we can access individual bytes

void CANIntHandler ()
{
	TURNOFF_ALL_LED();
	uint32_t CAN_status = CANIntStatus(CAN0_BASE,CAN_INT_STS_CAUSE);
	if(CAN_status == CAN_INT_INTID_STATUS)							// controller status interrupt
	{
		CAN_status = CANStatusGet(CAN0_BASE,CAN_STS_CONTROL);		// read back error bits and print it
		//UARTprintf("ERROR --> CAN controller status: 0x\%02X\n\r",CAN_status);
		char Decoded_ControllerStsReg[30];
		UARTprintf("\%s\n\r", app_can_DecodeControllerStsReg(CAN_status,Decoded_ControllerStsReg));
		err_flag = 1;
		TURNON_RED_LED();											//turn on RED led to indicate ERROR!
	}
	else if(CAN_status == 1)										// message object 1
	{
		CANIntClear(CAN0_BASE,1);									// clear interrupt
		err_flag = 0;												// clear any error flags
		TURNON_GREEN_LED();											// turn on GREEN led to indicate successful
																	// msg transmission
		UARTprintf("CAN msg tx: \%02X-\%02X-\%02X-\%02X\n\r",msgDataPtr[0],msgDataPtr[1],msgDataPtr[2],msgDataPtr[3]);
	}
	else															// should never happen
	{
		UARTprintf("Unexpected CAN bus interrupt\n\r");
		TURNON_RED_LED();											//turn on RED led to indicate ERROR!
	}

	SysCtlDelay(20000000);
	TURNOFF_ALL_LED();
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

    	TURNON_CYAN_LED();
    	SysCtlDelay(50000000);
    	TURNOFF_ALL_LED();
    	SysCtlDelay(20000000);
    }

return 0;
}
