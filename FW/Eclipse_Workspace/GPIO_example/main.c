
#include <stdint.h>
#include <stdbool.h>

#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h" //for macro declaration of GPIO_O_LOCK and GPIO_O_CR
#include "inc/hw_types.h"

#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

//#include "driverlib/systick.h"

#define SYSTICKS_PER_SECOND 100

//definition of isr for PORT F
void GPIO_PORTF_isr(void)
{
	uint32_t actual_GPIO_PORTF_status=((GPIO_PIN_0 | GPIO_PIN_4) & ~GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0 | GPIO_PIN_4));

	switch (actual_GPIO_PORTF_status)
	{
	case GPIO_PIN_4:																			//if SW1 is pressed
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);								//turn on RED LED
	break;

	case GPIO_PIN_0:																			//if SW2 is pressed
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);								//turn on BLUE LED
	break;

	case (GPIO_PIN_0|GPIO_PIN_4):																//either SW1 and SW2 are pressed
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_1, GPIO_PIN_2 | GPIO_PIN_1);	//turn on RED & BLUE LEDs
	break;

	default:																					//neither SW1 nor SW2 are pressed
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_1 , 0);							//turn off RED & BLUE LEDs
	break;

	}

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
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6 | GPIO_PIN_7);

	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

	ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_7);

// TODO: test either the interrupts on a simple pushbutton to turn-on a led:
// 			1- interrupt with static allocation on the vector table
//			2- interrupt with dynamic allocation on the vector table

//			2- interrupt with dynamic allocation on the vector table

// setup pin connected to SW1 and SW2

	// Unlock PF0 so we can change it to a GPIO input
	// Once we have enabled (unlocked) the commit register then re-lock it
	// to prevent further changes.  PF0 is muxed with NMI thus a special case.
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_AFSEL) |= 0x000;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

	//Configures pin(s) for use as GPIO inputs
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE,GPIO_PIN_4 | GPIO_PIN_0);
    //Sets the pad configuration for the specified pin(s).
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4 | GPIO_PIN_0,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
    // Make PORT F pin 0,4 high level triggered interrupts.
    ROM_GPIOIntTypeSet(GPIO_PORTF_BASE,GPIO_PIN_4 | GPIO_PIN_0,GPIO_BOTH_EDGES);


    //dynamic allocation on the vector table of GPIO_PORTF_isr interrupt handler
    GPIOIntRegister(GPIO_PORTF_BASE, GPIO_PORTF_isr);

    //Enables the specified GPIO interrupt
    IntEnable(INT_GPIOF);
    GPIOIntEnable(GPIO_PORTF_BASE,GPIO_INT_PIN_4 | GPIO_INT_PIN_0);

    IntMasterEnable();

    uint8_t PORTF_status;

	//uint32_t ui32Loop = 0;

    //
    // Loop forever
    //
    while(1)
    {
    	uint8_t PORTF_status=GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0 | GPIO_PIN_4);
    	/*
    	if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4))
    	{
    		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 ,0);
    	}
    	else
    	{
    		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 , GPIO_PIN_1);
    	}
    	*/

    	/*
        //
        // Turn on the red LED .
        //
        ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 , GPIO_PIN_1);
        ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
        //
        // Delay for a bit.
        //
        for(ui32Loop = 0; ui32Loop < 2000000; ui32Loop++)
        {
        }

        //
        // Turn on the green LED.
        //
        ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 , GPIO_PIN_2);
        ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0);
        //
        // Delay for a bit.
        //
        for(ui32Loop = 0; ui32Loop < 2000000; ui32Loop++)
        {
        }

        //
        // Turn on the blue LED.
        //
        ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 , GPIO_PIN_3);
        //
        // Delay for a bit.
        //
        for(ui32Loop = 0; ui32Loop < 2000000; ui32Loop++)
        {
        }
        */
    }
}
