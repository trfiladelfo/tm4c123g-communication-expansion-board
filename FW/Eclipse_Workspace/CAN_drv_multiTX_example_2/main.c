
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

#define SYSTICK_PERIOD 		50000000	// the number of clock ticks in each period of the SysTick counter
#define SYSCLOCK			80000000	// 80 MHz

#define MSG_LENGTH 			256			// maximum can msg length is 256 bytes (are tx up to 256/8=32 CAN frame)
#define PACKETS_NO			((MSG_LENGTH % 8) ? ((MSG_LENGTH / 8) +1) : (MSG_LENGTH / 8))	// CAN packets to tx related to MSG_LENGTH

volatile uint8_t err_flag = 0;
volatile uint32_t CAN_status = 0;
uint32_t t = 0;
uint32_t RxCount, TxCount;
bool ErrCountFlag;

static uint64_t msgData[MSG_LENGTH]; 		// can frame payload is 8 bytes max.
											// Static is essential: place the variable in RW block of ram
											// instead of into stack block ?


void delay(unsigned int milliseconds) {
	SysCtlDelay((SYSCLOCK / 3) * (milliseconds / 1000.0f));
}

//*******************************************************************************
//						CAN ISR
//*******************************************************************************
void CANIntHandler()
{									// Read the CAN interrupt status to find the cause of the interrupt
	CAN_status = CANIntStatus(CAN0_BASE,CAN_INT_STS_CAUSE);
	if(CAN_status == CAN_INT_INTID_STATUS)									// controller status interrupt
	{
		CAN_status = CANStatusGet(CAN0_BASE,CAN_STS_CONTROL);				// read back error bits it will
																			// clear also the status interrupt
		if(CAN_status & (CAN_STATUS_BUS_OFF | CAN_STATUS_EWARN | CAN_STATUS_EPASS | CAN_STATUS_LEC_MASK))
				{
					err_flag = 1;											// set the error flag
					//UARTprintf("Device connected on the CAN bus?\n\r");	// hint for connection trouble
				}
		CANIntDisable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);	//disable all CAN0 interrupts
	}
	else if(CAN_status)							// message object 1 interrupt
	{											// number of the highest priority message object that has an interrupt pending
		err_flag = 0;							// clear any error flags
		UARTprintf("Transmitting: %X-%X-%X-%X\n", msgData[0],msgData[1],msgData[2],msgData[3]);
		CANIntClear(CAN0_BASE,CAN_status);		// clear interrupt due to message object#CAN_status [1-32]
	}
	else																			// should never happen
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
/*_______________________INITIALIZATION OF MESSAGE OBJECT_______________________*/

	volatile uint16_t i;
	for(i=0 ; i<PACKETS_NO ; i++){
		msgData[i]=0x0101010101010101 * i;	// fill the 8 bytes content with its vector index number
	}
/*______________________________________________________________________________*/

	UARTprintf("MCU is running\n\r");

//*******************************************************************************
//*******************************************************************************
//						LOOP FOREVER
//*******************************************************************************
//*******************************************************************************
    while(1)
    {

//*******************************************************************************
//						CAN bus stuffs handling
//*******************************************************************************
    	if(err_flag==1)																//  CAN BUS ERROR
    	{
    		char Decoded_ControllerStsReg[30];
    		UARTprintf("BUS cable disconnected ? Please, reconnect...\n\r");							// print an hint
			UARTprintf("\%s\n\r", app_can_DecodeControllerStsReg(CAN_status,Decoded_ControllerStsReg));	// print errors
			delay(500);
    		CANIntEnable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);	//	enable all CAN0 interrupts
    	}
    	else if(err_flag==2)
    	{
    		UARTprintf("Unexpected CAN bus interrupt\n\r");							// UNKNOWN ERROR
    		delay(500);
    		CANIntEnable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);	//	enable all CAN0 interrupts

    	}

    	else																		// NO ERROR -> TRANSMIT PACKET
		{
    		CANIntEnable(CAN0_BASE,CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);//	enable all CAN0 interrupts
			app_can_SendMultipleMsg(msgData,sizeof msgData/8);
		}
    	//TODO: only for debug purposes. Comment out if not useful.
		ErrCountFlag=CANErrCntrGet(CAN0_BASE, &RxCount, &TxCount);					// get the error flag and the error counters
		UARTprintf("error number on CAN bus: RxCount= \%d; TxCount= \%d; Error flag: %s; t= %d;\n\r",
					RxCount, TxCount,(ErrCountFlag?"True":"False"),t);				// print error counter
    	//
	delay(500); 		// wait 500ms
    }
return 0;
}

