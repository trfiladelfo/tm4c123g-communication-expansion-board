
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "my_uart.h"
#include "app_can.h"

#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"


#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/can.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "drivers/uartstdio.h"

#define SYSTICK_PERIOD 		50000000	// the number of clock ticks in each period of the SysTick counter
#define SYSCLOCK			80000000	// 80 MHz

volatile uint32_t CAN_status = 0;
uint32_t counter=0;
uint32_t CANmsgTx=0;


void delay(unsigned int milliseconds) {
	SysCtlDelay((SYSCLOCK / 3) * (milliseconds / 1000.0f));
}

//*******************************************************************************
//						CAN ISR
//*******************************************************************************
void CANIntHandler()
{									// Read the CAN interrupt status to find the cause of the interrupt
	CAN_status = CANIntStatus(CAN0_BASE,CAN_INT_STS_CAUSE);
	if(CAN_status == CAN_INT_INTID_STATUS)								// controller status interrupt
	{
		CAN_status = CANStatusGet(CAN0_BASE,CAN_STS_CONTROL);			// read back error bits it will
																		// clear also the status interrupt
		if(CAN_status & (CAN_STATUS_BUS_OFF | CAN_STATUS_EWARN | CAN_STATUS_EPASS | CAN_STATUS_LEC_MASK))
				{
		    		char Decoded_ControllerStsReg[30];
		    		UARTprintf("BUS cable disconnected ? Please, reconnect...\n\r");							// print an hint
					UARTprintf("\%s\n\r", app_can_DecodeControllerStsReg(CAN_status,Decoded_ControllerStsReg));	// print errors
					delay(500);
				}
	}
	else if((CAN_status > 0) && CAN_status < 33)						// message object interrupt
	{
		CANIntClear(CAN0_BASE,CAN_status);								// clear interrupt
		UARTprintf("msgObj #%d transmitted\r\n", CAN_status);
		CANmsgTx++;
	}
	else																// should never happen
	{
		UARTprintf("Unexpected CAN bus interrupt\n\r");							// UNKNOWN ERROR
		delay(500);
	}
}


int main(void)
{
//*******************************************************************************
//						INITIALIZATION
//*******************************************************************************
	// setup the system clock to run at 80 MHz from the external crystal:
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	// enable peripherals to operate when CPU is in sleep:
	ROM_SysCtlPeripheralClockGating(true);
	//initialize UART console for debugging purposes
	InitConsole();
	//initialize CAN controller
	app_can_init();

/*_______________________INITIALIZATION OF MESSAGE DATA_________________________*/
	uint64_t msgData [0xFF]; 						// the message data could be up to eight bytes long which we can allocate as an int64
													// each 8 Byte (each element of the vector) will be tx on a single CAN frame
	volatile uint16_t i;
	for(i=0x00;i<0xFF;i++) msgData[i]=0x0101010101010101 * i;
	uint64_t *msgDataPtr = &msgData; 		// make a pointer to msgData so we can access individual bytes

/*______________________________________________________________________________*/

	UARTprintf("MCU is running\n\r");

	app_can_SendMultipleMsg(msgDataPtr,16);

//*******************************************************************************
//*******************************************************************************
//						LOOP FOREVER
//*******************************************************************************
//*******************************************************************************
    while(1)
    {
	delay(100); 		// wait 100ms
	counter++;
	UARTprintf(".");
    }
return 0;
}

