// uartdma.c
// Low-level UART driver for the first 4 UARTs.
// Uses DMA for TX operations.
// Uses WTIMER0 and WTIMER1 for RX timeouts.
// Copyright (C) 2014 Giorgio Biagetti.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_ints.h>
#include <inc/hw_gpio.h>
#include <inc/hw_uart.h>
#include <inc/hw_timer.h>
#include <inc/hw_sysctl.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/gpio.h>
#include <driverlib/uart.h>
#include <driverlib/udma.h>
#include <driverlib/sysctl.h>
#include <driverlib/rom.h>

#include "uartdma.h"
#include "../Network/errors.h"

#include "../board.h"

volatile uint8_t uart_data_length[4];
volatile bool uart_data_available[4];
static uint32_t uart_counter[4];

static volatile int rx_phase[4];
static volatile uint8_t rx_count[4];
uint8_t uart_rx_buffer[4][256];

static bool uart_common_initialized = false;

static const uint32_t uart_timer_assignment[4] = {
	WTIMER0_BASE + TIMER_O_TAILR,
	WTIMER0_BASE + TIMER_O_TBILR,
	WTIMER1_BASE + TIMER_O_TAILR,
	WTIMER1_BASE + TIMER_O_TBILR
};

static const uint32_t uart_timer_enables[4] = {
	BITBAND(WTIMER0_BASE + TIMER_O_CTL, 0 /* TAEN */ ),
	BITBAND(WTIMER0_BASE + TIMER_O_CTL, 8 /* TBEN */ ),
	BITBAND(WTIMER1_BASE + TIMER_O_CTL, 0 /* TAEN */ ),
	BITBAND(WTIMER1_BASE + TIMER_O_CTL, 8 /* TBEN */ )
};

static const uint32_t uart_base_register[4] = {
	UART0_BASE,
	UART1_BASE,
	UART2_BASE,
	UART3_BASE
};

static const uint32_t uart_dma_channel[4] = {
	UDMA_CH9_UART0TX,
	UDMA_CH23_UART1TX,
	UDMA_CH13_UART2TX,
	UDMA_CH17_UART3TX
};

/*	initialize UART hardware:
 *	 - port    : UART number
 *	 - speed   : baud rate
 *	 - timeout : intercharacter timeout [µs]
 */
int uart_init (unsigned port, uint32_t speed, uint32_t timeout)
{
	if (port > 3) return -EINVAL;
	if (!uart_common_initialized) {
		uart_common_initialized = true;
		// configure timers for timeout detection:
		// WTIMER0A, WTIMER0B, WTIMER1A, WTIMER1B
		const uint32_t prescaler = 79; // set prescaler to 80 (count microseconds @ 80 MHz system clock)
		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER0);
		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER1);
		ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_WTIMER0);
		ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_WTIMER1);
		ROM_IntEnable(INT_WTIMER0A);
		ROM_IntEnable(INT_WTIMER0B);
		ROM_IntEnable(INT_WTIMER1A);
		ROM_IntEnable(INT_WTIMER1B);
		HWREG(WTIMER0_BASE + TIMER_O_CTL)   = 0; // disable timer
		HWREG(WTIMER0_BASE + TIMER_O_CFG)   = 4; // dual 32-bit timers
		HWREG(WTIMER0_BASE + TIMER_O_TAMR)  = TIMER_TAMR_TAMR_1_SHOT;
		HWREG(WTIMER0_BASE + TIMER_O_TBMR)  = TIMER_TBMR_TBMR_1_SHOT;
		HWREG(WTIMER0_BASE + TIMER_O_TAPR)  = prescaler;
		HWREG(WTIMER0_BASE + TIMER_O_TBPR)  = prescaler;
		HWREG(WTIMER0_BASE + TIMER_O_IMR)   = TIMER_IMR_TATOIM | TIMER_IMR_TBTOIM;
		HWREG(WTIMER1_BASE + TIMER_O_CTL)   = 0; // disable timer
		HWREG(WTIMER1_BASE + TIMER_O_CFG)   = 4; // dual 32-bit timers
		HWREG(WTIMER1_BASE + TIMER_O_TAMR)  = TIMER_TAMR_TAMR_1_SHOT;
		HWREG(WTIMER1_BASE + TIMER_O_TBMR)  = TIMER_TBMR_TBMR_1_SHOT;
		HWREG(WTIMER1_BASE + TIMER_O_TAPR)  = prescaler;
		HWREG(WTIMER1_BASE + TIMER_O_TBPR)  = prescaler;
		HWREG(WTIMER1_BASE + TIMER_O_IMR)   = TIMER_IMR_TATOIM | TIMER_IMR_TBTOIM;
	}
	HWREG(uart_timer_assignment[port]) = timeout - 1;

	switch (port) {
		case 0:
			// enable and setup UART0:
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
			ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
			ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
			ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
			ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
			ROM_IntEnable(INT_UART0);
			break;
		case 1:
			// enable and setup UART1:
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
			ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART1);
			ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
			ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
			ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
			ROM_IntEnable(INT_UART1);
			break;
		case 2:
			// enable and setup UART2:
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
			ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART2);
			ROM_GPIOPinConfigure(GPIO_PD6_U2RX);
			ROM_GPIOPinConfigure(GPIO_PD7_U2TX);
			ROM_GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
			ROM_IntEnable(INT_UART2);
			break;
		case 3:
			// enable and setup UART3:
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
			ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART3);
			ROM_GPIOPinConfigure(GPIO_PC6_U3RX);
			ROM_GPIOPinConfigure(GPIO_PC7_U3TX);
			ROM_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
			ROM_IntEnable(INT_UART3);
			break;
	}
	// configure UART parameters and DMA:
	ROM_UARTConfigSetExpClk(uart_base_register[port], ROM_SysCtlClockGet(), speed, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
	ROM_UARTFIFOLevelSet(uart_base_register[port], UART_FIFO_TX4_8, UART_FIFO_RX4_8);
	ROM_UARTEnable(uart_base_register[port]);
	ROM_UARTDMAEnable(uart_base_register[port], UART_DMA_TX);
	ROM_UARTIntEnable(uart_base_register[port], UART_INT_RX | UART_INT_RT);
	HWREG(uart_base_register[port] + UART_O_CTL) |= UART_CTL_EOT; // DMA TX complete interrupt waits for transmitter to be empty.
	// configure uDMA UART0TX channel:
	ROM_uDMAChannelAssign(uart_dma_channel[port]);
	ROM_uDMAChannelAttributeDisable(
		uart_dma_channel[port],
		UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);
	ROM_uDMAChannelAttributeEnable(
		uart_dma_channel[port], UDMA_ATTR_USEBURST
	);
	ROM_uDMAChannelControlSet(
		uart_dma_channel[port] | UDMA_PRI_SELECT,
		UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_4
	);

	return 0;
}

int uart_send_frame (unsigned port, void const *data, uint8_t len)
{
	if (port > 3) return -EINVAL;
	// check if a transfer is already in progress and refuse to proceed if so:
	if (uDMAChannelIsEnabled(uart_dma_channel[port])) return -EBUSY;
	// setup uDMA and start transfer:
	ROM_uDMAChannelTransferSet(
		uart_dma_channel[port] | UDMA_PRI_SELECT,
		UDMA_MODE_BASIC,
		(void *) data, (void *) (uart_base_register[port] + UART_O_DR), len
	);
	ROM_uDMAChannelEnable(uart_dma_channel[port]);
	return 0;
}

int uart_start_timeout (unsigned port, uint32_t timeout)
{
	if (!timeout) return 0;
	__asm("cpsid i");
	if (!HWREG(uart_timer_enables[port])) {
		HWREG(uart_timer_assignment[port]) = timeout - 1;
		HWREG(uart_timer_enables[port]) = 1;
	}
	__asm("cpsie i");
	return 0;
}


// private functions:
#include "../serial.h"

extern uint64_t nwk_station_address;

static void uart_rx_data (unsigned port, uint32_t data)
{
	if (data & 0xF00) { // TODO: process error!
		return;
	}
	if (!rx_count[port]) {
		if (rx_phase[port] == 0) {
			// Watch out for a start of frame:
			if (data == '\005') { // ENQ
				serial_send_48("ToLHnet node ", nwk_station_address, serial_eol);
			} else if (data == '?') {
				rx_count[port] = 1;
				uart_rx_buffer[port][rx_phase[port]++] = data;
			} else if (data == '\002' || data == '\003') { // STX
				uart_rx_buffer[port][rx_phase[port]++] = data;
			} else if (data == '\006' || data == '\012') { // ACK & NACK
				uart_rx_buffer[port][0] = data;
				uart_data_length[port] = 1;
				uart_data_available[port] = true;
			}
		} else if (rx_phase[port] == 1) {
			rx_count[port] = data + 3;
			uart_rx_buffer[port][rx_phase[port]++] = data;
		}
	} else {
		uart_rx_buffer[port][rx_phase[port]++] = data;
		if (!--rx_count[port]) {
			uart_data_length[port] = rx_phase[port];
			uart_data_available[port] = true;
			rx_phase[port] = 0;
		}
	}
}


// ISRs:

void isr_UART0 (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(UART0_BASE + UART_O_MIS);
	HWREG(UART0_BASE + UART_O_ICR) = status;
	// update statistics:
	++uart_counter[0];
	// process receive interrupts (FIFO ready or RX timeout mean the same thing for now...)
	if (status & (UART_INT_RX | UART_INT_RT)) {
		// reset UART watchdog timer:
		HWREG(WTIMER0_BASE + TIMER_O_TAILR) = HWREG(WTIMER0_BASE + TIMER_O_TAILR);
		HWREG(uart_timer_enables[0]) = 1;
		// process the data available in the FIFO:
		while ((HWREG(UART0_BASE + UART_O_FR) & UART_FR_RXFE) == 0) {
			uint32_t data = HWREG(UART0_BASE + UART_O_DR) & 0xFFF;
			uart_rx_data(0, data);
		}
	}
}

void isr_UART1 (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(UART1_BASE + UART_O_MIS);
	HWREG(UART1_BASE + UART_O_ICR) = status;
	// update statistics:
	++uart_counter[1];
	// process receive interrupts (FIFO ready or RX timeout mean the same thing for now...)
	if (status & (UART_INT_RX | UART_INT_RT)) {
		// reset UART watchdog timer:
		HWREG(WTIMER0_BASE + TIMER_O_TBILR) = HWREG(WTIMER0_BASE + TIMER_O_TBILR);
		HWREG(uart_timer_enables[1]) = 1;
		// process the data available in the FIFO:
		while ((HWREG(UART1_BASE + UART_O_FR) & UART_FR_RXFE) == 0) {
			uint32_t data = HWREG(UART1_BASE + UART_O_DR) & 0xFFF;
			uart_rx_data(1, data);
		}
	}
}

void isr_UART2 (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(UART2_BASE + UART_O_MIS);
	HWREG(UART2_BASE + UART_O_ICR) = status;
	// update statistics:
	++uart_counter[2];
	// process receive interrupts (FIFO ready or RX timeout mean the same thing for now...)
	if (status & (UART_INT_RX | UART_INT_RT)) {
		// reset UART watchdog timer:
		HWREG(WTIMER1_BASE + TIMER_O_TAILR) = HWREG(WTIMER1_BASE + TIMER_O_TAILR);
		HWREG(uart_timer_enables[2]) = 1;
		// process the data available in the FIFO:
		while ((HWREG(UART2_BASE + UART_O_FR) & UART_FR_RXFE) == 0) {
			uint32_t data = HWREG(UART2_BASE + UART_O_DR) & 0xFFF;
			uart_rx_data(2, data);
		}
	}
}

void isr_UART3 (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(UART3_BASE + UART_O_MIS);
	HWREG(UART3_BASE + UART_O_ICR) = status;
	// update statistics:
	++uart_counter[3];
	// process receive interrupts (FIFO ready or RX timeout mean the same thing for now...)
	if (status & (UART_INT_RX | UART_INT_RT)) {
		// reset UART watchdog timer:
		HWREG(WTIMER1_BASE + TIMER_O_TBILR) = HWREG(WTIMER1_BASE + TIMER_O_TBILR);
		HWREG(uart_timer_enables[3]) = 1;
		// process the data available in the FIFO:
		while ((HWREG(UART3_BASE + UART_O_FR) & UART_FR_RXFE) == 0) {
			uint32_t data = HWREG(UART3_BASE + UART_O_DR) & 0xFFF;
			uart_rx_data(3, data);
		}
	}
}

void isr_timerW0A (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(WTIMER0_BASE + TIMER_O_MIS);
	HWREG(WTIMER0_BASE + TIMER_O_ICR) = status & TIMER_MIS_TATOMIS;
	// a timeout occurred while receiving a frame - abort transaction:
	if (rx_phase[0]) {
		uart_data_length[0] = 0;
		uart_data_available[0] = true;
	}
	rx_phase[0] = 0;
	rx_count[0] = 0;
}

void isr_timerW0B (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(WTIMER0_BASE + TIMER_O_MIS);
	HWREG(WTIMER0_BASE + TIMER_O_ICR) = status & TIMER_MIS_TBTOMIS;
	// a timeout occurred while receiving a frame - abort transaction:
	if (rx_phase[1]) {
		uart_data_length[1] = 0;
		uart_data_available[1] = true;
	}
	rx_phase[1] = 0;
	rx_count[1] = 0;
}

void isr_timerW1A (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(WTIMER1_BASE + TIMER_O_MIS);
	HWREG(WTIMER1_BASE + TIMER_O_ICR) = status & TIMER_MIS_TATOMIS;
	// a timeout occurred while receiving a frame - abort transaction:
	if (rx_phase[2]) {
		uart_data_length[2] = 0;
		uart_data_available[2] = true;
	}
	rx_phase[2] = 0;
	rx_count[2] = 0;
}

void isr_timerW1B (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(WTIMER1_BASE + TIMER_O_MIS);
	HWREG(WTIMER1_BASE + TIMER_O_ICR) = status & TIMER_MIS_TBTOMIS;
	// a timeout occurred while receiving a frame - abort transaction:
	if (rx_phase[3]) {
		uart_data_length[3] = 0;
		uart_data_available[3] = true;
	}
	rx_phase[3] = 0;
	rx_count[3] = 0;
}
