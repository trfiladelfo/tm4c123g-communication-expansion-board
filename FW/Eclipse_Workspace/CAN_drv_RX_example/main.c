
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
#include "driverlib/systick.h"

#include "drivers/uartstdio.h"
#include "drivers/rgb.h"

#define SYSTICK_PERIOD 	50000000	// the number of clock ticks in each period of the SysTick counter

volatile uint8_t rxFlag;
volatile uint8_t err_flag=0;
volatile uint32_t CAN_status;

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
		CANIntDisable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);	//disable all CAN0 interrupts
	}
	else if(CAN_status == 1)											// message object 1 interrupt
	{
		rxFlag = 1;														// set rx flag
		err_flag = 0;													// clear any error flags
		CANIntClear(CAN0_BASE,1);										// clear interrupt
	}
	else																// should never happen
	{
		err_flag = 2 ;
		CANIntDisable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);	//disable all CAN0 interrupts
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
	uint8_t msgData[8]; 							// 8 byte buffer for rx message data

	// Set up of CAN msg object
	// Use ID and mask 0 to receive messages with any CAN ID
	msg.ui32MsgID = 0;
	msg.ui32MsgIDMask = 0;
	msg.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
	msg.ui32MsgLen = 6; 							// allow up to 6 bytes
/*______________________________________________________________________________*/

	// Load msg into CAN peripheral message object 1 so it can trigger interrupts on any matched rx messages
	CANMessageSet(CAN0_BASE, 1, &msg, MSG_OBJ_TYPE_RX);

	uint32_t colour[3];
	float intensity;

	UARTprintf("MCU is running: waiting for CAN packets\n\r");

//*******************************************************************************
//*******************************************************************************
//						LOOP FOREVER
//*******************************************************************************
//*******************************************************************************
    while(1)
    {

    	if(err_flag==1)																//  CAN BUS ERROR
    	    	{
    	    		char Decoded_ControllerStsReg[30];
    	    		UARTprintf("BUS cable disconnected ? Please, reconnect...\n\r");							// print an hint
    				UARTprintf("\%s\n\r", app_can_DecodeControllerStsReg(CAN_status,Decoded_ControllerStsReg));	// print errors
    				CANIntEnable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);	//	enable all CAN0 interrupts
    	    	}
		else if(err_flag==2){
			UARTprintf("Unexpected CAN bus interrupt\n\r");		// UNKNOWN ERROR
			CANIntEnable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);	//	enable all CAN0 interrupts
		}
		else																		// NO ERROR -> RECEIVE PACKETS
		{
			CANIntEnable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);	//	enable all CAN0 interrupts
			if(rxFlag) 										// rx interrupt has occured
			{
				msg.pui8MsgData = msgData; 					// set pointer to rx buffer
				CANMessageGet(CAN0_BASE, 1, &msg, 0); 		// read CAN message object 1 from CAN peripheral
				rxFlag = 0; 								// clear rx flag
				if(msg.ui32Flags & MSG_OBJ_DATA_LOST) { 	// check msg flags for any lost messages
					UARTprintf("CAN message loss detected\n\r");
				}
				// read colour data from rx buffer (scale from 0-255 to 0-0xFFFF for LED driver)
				colour[0] = msgData[0] * 0xFF;
				colour[1] = msgData[1] * 0xFF;
				colour[2] = msgData[2] * 0xFF;
				intensity = msgData[3] / 255.0f; 			// scale from 0-255 to float 0-1
				// write to UART for debugging
				UARTprintf("Received colour\tr: %d\tg: %d\tb: %d\ti: %d\n", msgData[0], msgData[1], msgData[2], msgData[3]);
				// set colour and intensity
				RGBSet(colour, intensity);
			}

		}
    }

return 0;
}
