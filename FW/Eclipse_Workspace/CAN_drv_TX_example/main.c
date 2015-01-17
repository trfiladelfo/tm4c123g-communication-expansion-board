
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
#include "drivers/rgb.h"

#define PI 3.14159265359f
#define SYSTICK_PERIOD 		50000000	// the number of clock ticks in each period of the SysTick counter
#define SYSCLOCK			80000000	// 80 MHz

volatile uint8_t err_flag= 0;
volatile uint32_t CAN_status = 0;
uint32_t RxCount, TxCount;
bool ErrCountFlag;

void delay(unsigned int milliseconds) {
	SysCtlDelay((SYSCLOCK / 3) * (milliseconds / 1000.0f));
}

//*******************************************************************************
//						CAN ISR
//*******************************************************************************
void CANIntHandler ()
{									// Read the CAN interrupt status to find the cause of the interrupt
	CAN_status = CANIntStatus(CAN0_BASE,CAN_INT_STS_CAUSE);
	if(CAN_status == CAN_INT_INTID_STATUS)								// controller status interrupt
	{
		CAN_status = CANStatusGet(CAN0_BASE,CAN_STS_CONTROL);			// read back error bits it will
																		// clear also the status interrupt
		if(CAN_status & (CAN_STATUS_BUS_OFF | CAN_STATUS_EWARN | CAN_STATUS_EPASS | CAN_STATUS_LEC_MASK))
				{
					err_flag = 1;										// set the error flag
					UARTprintf("Device connected on the CAN bus?\n\r");	// hint for connection trouble
				}
	}
	else if(CAN_status == 1)											// message object 1 interrupt
	{
		err_flag = 0;													// clear any error flags
		CANIntClear(CAN0_BASE,1);										// clear interrupt
	}
	else																// should never happen
	{
		err_flag = 2 ;
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
	// Set up LED driver
	RGBInit(1);

/*_______________________INITIALIZATION OF MESSAGE OBJECT_______________________*/

	tCANMsgObject msg; 								// the CAN message object
	uint64_t msgData = 0; 							// the message data could be up to eight bytes long which we can allocate as an int64
	uint8_t *msgDataPtr = (uint8_t *)&msgData; 		// make a pointer to msgData so we can access individual bytes

	// Set up of CAN msg object
	msgData = 0;
	msg.ui32MsgID = 1;
	msg.ui32MsgIDMask = 0;
	msg.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
	msg.ui32MsgLen = sizeof(msgDataPtr);
	msg.pui8MsgData = msgDataPtr;
/*______________________________________________________________________________*/

	uint32_t t = 0; // loop counter
	float freq = 0.3; // frequency scaler

	UARTprintf("MCU is running\n\r");

//*******************************************************************************
//*******************************************************************************
//						LOOP FOREVER
//*******************************************************************************
//*******************************************************************************
    while(1)
    {
		// set up next colour (scale sinf (0-1) to 0-255)
		msgDataPtr[0] = (0.5 + 0.5*sinf(t*freq)) * 0xFF;
		msgDataPtr[1] = (0.5 + 0.5*sinf(t*freq + (2*PI/3))) * 0xFF; 	// 120 degrees out of phase
		msgDataPtr[2] = (0.5 + 0.5*sinf(t*freq + (4*PI/3))) * 0xFF; 	// 240 degrees out of phase
		msgDataPtr[3] = 128;											// 50% intensity

//*******************************************************************************
//						CAN bus stuffs handling
//*******************************************************************************
    	if(err_flag==1)																//  CAN BUS ERROR
    	{
    		char Decoded_ControllerStsReg[30];
    		UARTprintf("BUS cable disconnected ? Please, reconnect...\n\r");							// print an hint
			UARTprintf("\%s\n\r", app_can_DecodeControllerStsReg(CAN_status,Decoded_ControllerStsReg));	// print errors
    	}
    	else if(err_flag==2) UARTprintf("Unexpected CAN bus interrupt\n\r");		// UNKNOWN ERROR
    	else																		// NO ERROR -> TRANSMIT PACKET
		{
																					// write colour to UART for debugging
			UARTprintf("Sending colour\tr: %d\tg: %d\tb: %d\n", msgDataPtr[0], msgDataPtr[1], msgDataPtr[2]);
			CANMessageSet(CAN0_BASE, 1, &msg, MSG_OBJ_TYPE_TX); 					// send message object 1 as CAN packet
		}
    	//TODO: only for debug purposes. Comment out if not useful.
		ErrCountFlag=CANErrCntrGet(CAN0_BASE, &RxCount, &TxCount);			// get the error flag and the error counters
		UARTprintf("error number on CAN bus: RxCount= \%d; TxCount= \%d; Error flag: %s; t= %d;\n\r",
					RxCount, TxCount,(ErrCountFlag?"True":"False"),t);		// print error counter
    	//
	delay(100); 		// wait 100ms
	t++; 				// overflow is fine
    }
return 0;
}
