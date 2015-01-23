/*
 * app_can.c
 *
 *  Created on: Dec 31, 2014
 *      Author: Luca Buccolini
 *
 *  Useful document: 	TivaWare Peripheral Driver Library (SW-TM4C-DRL-UG-2.1.0.12573)
 *  					The paragraph's numbers in following comments are related to this document.
 *
 */

#include "app_can.h"

#include <stdbool.h>
#include <stdlib.h>


#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/can.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"



#define CAN_BITRATE 125000	// is the desired bit rate of the CAN transmission
#define CANMSG_ID 	0x0001	// this is the 11-bit CAN message ID (avoid use of msg ID longest than 11 bit)

//*******************************************************************************
//						app_can_init()
//*******************************************************************************

void app_can_init()
{
/*
//27.2.2.34 SysCtlPeripheralPowerOn
	//Powers on CAN peripheral.
	//Not used in CAN example.
SysCtlPeripheralPowerOn(SYSCTL_PERIPH_CAN0);
*/

	//27.2.2.32 SysCtlPeripheralEnable
		// GPIO port B and E needs to be enabled so these pins can be used
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);	//enable GPIO port B
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);	//enable GPIO port E

	//15.2.3.17 GPIOPinConfigure
		// Configures pins for use as a CAN device.
		// Configure the GPIO pin muxing to select CAN0 functions for these pins.
		// This step selects which alternate function is available for these pins.
	GPIOPinConfigure(GPIO_PE4_CAN0RX);
	GPIOPinConfigure(GPIO_PE5_CAN0TX);

	//15.2.3.20 GPIOPinTypeCAN
		// Enable the alternate function on the GPIO pins.  The above step selects
		// which alternate function is available.  This step actually enables the
		// alternate function instead of GPIO for these pins.
		// Configure GPIO Port E pins 4 and 5 to be used as CAN0.
	GPIOPinTypeCAN(GPIO_PORTE_BASE,GPIO_PIN_4 | GPIO_PIN_5);

	//15.2.3.27 GPIOPinTypeGPIOOutput
		// Set GPIO PORTB pins 6 and 7 as output, SW controlled.
		// CAN_RS=PB6, CAN_Enable=PB7
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE,GPIO_PIN_6 | GPIO_PIN_7);

	//15.2.3.48 GPIOPinWrite
		// Writes a "0" to pin PB6 (CAN_RS pin of CAN transceiver TI SN65HVD64).
	GPIOPinWrite(GPIO_PORTB_BASE,GPIO_PIN_6,0);		//enable CAN driver slope control through value of RS resistor.
		// If a high-level input (> 0.75 VCC) is applied to CAN_RS pin (TI SN65HVD64, pin 8), the circuit enters
		// a low-current, listen only standby mode during which the driver is switched off and the receiver
		// remains active.
		// Uncomment following line (and comment previous line) if a listen-only standby-mode is desired.
		//
	 	// GPIOPinWrite(GPIO_PORTB_BASE,GPIO_PIN_6,GPIO_PIN_6); //CAN driver switched off and CAN receiver active
		//
		//
		// Writes a "1" to pin PB7 CAN_Enable pin of CAN transceiver TI SN65HVD64, enabling the external HW.
	GPIOPinWrite(GPIO_PORTB_BASE,GPIO_PIN_7,GPIO_PIN_7);	//enable CAN transceiver

	//27.2.2.32 SysCtlPeripheralEnable
		// The GPIO port and pins have been set up for CAN.  The CAN peripheral
		// must be enabled.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);				//enable CAN 0
		// It takes five clock cycles after the write to enable a peripheral before
		// the the peripheral is actually enabled. Uncomment following line if needed.
		//
	SysCtlDelay(2);	//it generates a delay by executing (two times) a 3 instruction cycle loop

	// 6.2.5.7 CANInit
		// Initialize the CAN controller
    CANInit(CAN0_BASE);

    // 6.2.5.1 CANBitRateSet
    	// Sets the CAN bit timing values to a nominal setting based on a desired bit rate.
    	// If tighter timing requirements or longer network lengths are needed, then the
    	// CANBitTimingSet() function is available for full customization of all of the CAN b
    	// it timing values.
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), CAN_BITRATE);

    // 6.2.5.10 CANIntEnable & 6.2.5.11 CANIntRegister
    	// - Registers an interrupt handler for the CAN controller and
    	// - enable interrupts on the CAN peripheral.
    	// This code uses dynamic allocation of the vector table. If you want static allocation
    	// of interrupt handlers which means the name of the handler is in the vector table of
    	// startup code you must also comment "CANIntRegister(CAN0_BASE, CANIntHandler)".
    CANIntRegister(CAN0_BASE, CANIntHandler); // 	needed only using dynamic vectors
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);

    //18.2.3.2 IntEnable
    	//Enables CAN0 interrupts.
    IntEnable(INT_CAN0);

    //6.2.5.5 CANEnable
    	//Enables the CAN controller.
    CANEnable(CAN0_BASE);
}

//*******************************************************************************
//					app_can_DecodeControllerStsReg
//*******************************************************************************

//Decode the Controller Status Register. Only for debug purposes!
char* app_can_DecodeControllerStsReg(uint32_t Encoded_ControllerStsReg, char* Decoded_ControllerStsReg)
{
	strcpy(Decoded_ControllerStsReg,"");					// clear Decoded_ControllerStsReg string

	if(CAN_STATUS_BUS_OFF & Encoded_ControllerStsReg)		// controller is in bus-off condition
		strcat(Decoded_ControllerStsReg, "BUS_OFF\t");

	if(CAN_STATUS_EWARN & Encoded_ControllerStsReg)			// an error counter has reached a limit of at least 96
		strcat(Decoded_ControllerStsReg, "EWARN\t");

	if(CAN_STATUS_EPASS & Encoded_ControllerStsReg)			// CAN controller is in the error passive state
		strcat(Decoded_ControllerStsReg, "EPASS\t");

	if(CAN_STATUS_RXOK & Encoded_ControllerStsReg)			// a message was received successfully (independent of any
		strcat(Decoded_ControllerStsReg, "RXOK\t");			// message filtering)

	if(CAN_STATUS_TXOK & Encoded_ControllerStsReg)			// a message was successfully transmitted
		strcat(Decoded_ControllerStsReg, "TXOK\t");

	switch(Encoded_ControllerStsReg & CAN_STATUS_LEC_MSK)
	{
		case CAN_STATUS_LEC_STUFF:							// stuffing error detected
			strcat(Decoded_ControllerStsReg, "STUFF\t");
			break;
		case CAN_STATUS_LEC_FORM:							// a format error occurred in the fixed format part of a message
			strcat(Decoded_ControllerStsReg, "FORM\t");
			break;
		case CAN_STATUS_LEC_ACK:							// a transmitted message was not acknowledged
			strcat(Decoded_ControllerStsReg, "ACK\t");
			break;
		case CAN_STATUS_LEC_BIT1:							// dominant level detected when trying to send in recessive
			strcat(Decoded_ControllerStsReg, "BIT1\t");		// mode
			break;
		case CAN_STATUS_LEC_BIT0:							// recessive level detected when trying to send in dominant
			strcat(Decoded_ControllerStsReg, "BIT0\t");		// mode
			break;
		case CAN_STATUS_LEC_CRC:							// CRC error in received message
			strcat(Decoded_ControllerStsReg, "CRC\t");
			break;
	}
	return Decoded_ControllerStsReg;
}

//*******************************************************************************
//					app_can_SetupMultipleMsg
//*******************************************************************************
void app_can_SendMultipleMsg(uint64_t* pui64_msg, const uint16_t msg_length)	//msg_length is in Byte (max. is 256 Byte)
{
	const uint8_t CAN8BytePacketsNo = msg_length/8;		// CAN 8-byte Packets number to transmit
	uint8_t CANByteOnLastPacket = msg_length%8;			// Byte to transmit with last CAN frame (last frame could have length = 8 Bytes)

	uint8_t notExistShortCANPacket;
	uint8_t MsgVectorLastIndex;
	if(CANByteOnLastPacket){							// if you need to transmit a CAN frame with n < 8 bytes of data...
		notExistShortCANPacket = 0;						// exist a CAN frame which data length is < 8 bytes
		MsgVectorLastIndex = CAN8BytePacketsNo;			// thus last index of the CAN messages vector is  "CAN8BytePacketsNo"
	}													// note that ther's some 8 bytes packets and further (shorter) packet
														// if all CAN frame which data length is < 8 bytes
	else{												// if all CAN frame are 8 Bytes lenght
		notExistShortCANPacket = 1;						// not exist a CAN frame which data length is < 8 bytes
		MsgVectorLastIndex = CAN8BytePacketsNo - 1;		// thus last index of the CAN messages vector is  "CAN8BytePacketsNo - 1"
	}


	tCANMsgObject sCANMsgObject[CAN8BytePacketsNo];		// declaration of "msg_lenght" CANMSGObject structure

	if(CANByteOnLastPacket==0) CANByteOnLastPacket=8;	// impossible to have ANByteOnLastPacket=0. If it's =0, means
														// the last packet is an 8-bytes packet.
														// it occurs when msg_length is an integer multiple of 8: (8,16,24,...).

	/*____________________initialize all but the last CAN Message Objects_________________________*/
	volatile uint16_t i;
	for(i=0;i<=(CAN8BytePacketsNo - notExistShortCANPacket);i++){
			sCANMsgObject[i].ui32MsgID = CANMSG_ID;
			sCANMsgObject[i].ui32MsgIDMask = 0;
			sCANMsgObject[i].ui32Flags = MSG_OBJ_TX_INT_ENABLE | MSG_OBJ_FIFO;
			sCANMsgObject[i].ui32MsgLen = 8;
			sCANMsgObject[i].pui8MsgData = (uint8_t*) &pui64_msg[i];

			CANMessageSet(CAN0_BASE,i,&sCANMsgObject[i], MSG_OBJ_TYPE_TX);
	}

	/*________________________initialize the last CAN Message Objects____________________________*/
	// if exist a CAN frame which data length is < 8 bytes, then vector index of last CAN frame is "CAN8BytePacketsNo + 1"
	// if NOT exist a CAN frame which data length is < 8 bytes, then vector index of last CAN frame is "CAN8BytePacketsNo"
	sCANMsgObject[MsgVectorLastIndex].ui32MsgID = CANMSG_ID;
	sCANMsgObject[MsgVectorLastIndex].ui32MsgIDMask = 0;
	sCANMsgObject[MsgVectorLastIndex].ui32Flags = MSG_OBJ_TX_INT_ENABLE;
	sCANMsgObject[MsgVectorLastIndex].ui32MsgLen = CANByteOnLastPacket;
	sCANMsgObject[MsgVectorLastIndex].pui8MsgData = (uint8_t*) &pui64_msg[MsgVectorLastIndex];

	CANMessageSet(CAN0_BASE,MsgVectorLastIndex,&sCANMsgObject[MsgVectorLastIndex], MSG_OBJ_TYPE_TX);
}


