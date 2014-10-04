// w433_mac.h: wireless data-link layer for the HopeRF RF23B module
// Copyright (C) 2014 Giorgio Biagetti.
// Based on previous work by Francesco Ricciardi, (C) 2013 CEDAR Solutions.


#include "w433_mac.h"
#include "../../Network/network.h"
#include "../../Network/errors.h"
#include <string.h>

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>
#include <inc/hw_ssi.h>
#include <inc/hw_udma.h>
#include <inc/hw_ints.h>
#include <inc/hw_sysctl.h>
#include <inc/hw_timer.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/ssi.h>
#include <driverlib/udma.h>
#include <driverlib/interrupt.h>
#include <driverlib/rom.h>

#include "../../board.h"
#include "../../serial.h"

#include "RFM23B.h"

/* Define transaction status */
#define W433_STATE_UNINITIALIZED     0
#define W433_STATE_RESETTING         1
#define W433_STATE_INITIALIZING      2
#define W433_STATE_DISCONNECTED      3
#define W433_STATE_RECEIVE           4
#define W433_STATE_TRANSMIT          5
#define W433_STATE_RECEIVING         6
#define W433_STATE_ABORTED           7

#define W433_PACKET_MAX_LENGTH    250


/* Define machine state variable */
static uint8_t w433_state = W433_STATE_UNINITIALIZED;

/* Data buffer structure */
typedef struct {
	size_t length;
	size_t offset;
	uint8_t data[W433_PACKET_MAX_LENGTH];
} buffer_t;


/* Buffer declaration */
static buffer_t w433_rx_buffer;
static buffer_t w433_tx_buffer;


/* CALLBACK DECLARATION */
/* Signal the reception of a packet */
static void (*w433_packet_received_cb) (struct packet const *p);

static uint8_t w433_single_transaction(uint8_t address, uint8_t data);
static uint16_t w433_read_16BE (uint8_t address);
static int w433_send_packet (struct packet const *p);
static void goIdle (void);
static void goReceive (void);
static void goTransmit (void);
static void goAbort (void);
static void feedTxFifo (size_t len);
static void emptyRxFifo (size_t len);
static int8_t rssiToDb (uint8_t);

volatile uint32_t w433_event;
#define W433_EVENT_INTERRUPT  HWREG(BITBAND(&w433_event, 0))
#define W433_EVENT_TIMEOUT    HWREG(BITBAND(&w433_event, 1))
#define W433_EVENT_TEST       HWREG(BITBAND(&w433_event, 2))

#define W433_SDN   HWREGBITW(GPIO_PORTB_BASE + (GPIO_PIN_4 << 2), 4)
#define W433_TIMER HWREG(BITBAND(WTIMER2_BASE + TIMER_O_CTL, 0 /* TAEN */ ))
#define W433_TIMEOUT(x) do { HWREG(WTIMER2_BASE + TIMER_O_TAILR) = (x) - 1; W433_TIMER = 1; } while (0)
#define W433_TIMEOUT_DISABLE() do { W433_TIMER = 0; } while (0)

int w433_mac_init (void)
{
	// configure shutdown pin:
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4); // W433_SDN
	W433_SDN = 1;
	// configure SPI interface:
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_SSI0);
	//TODO: configure drive strength and pull-up/down on data pins?
	ROM_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	ROM_GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	ROM_GPIOPinConfigure(GPIO_PA4_SSI0RX);
	ROM_GPIOPinConfigure(GPIO_PA5_SSI0TX);
	ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);
	ROM_SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);
	ROM_SSIConfigSetExpClk(SSI0_BASE, ROM_SysCtlClockGet(), SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 10000000, 8);
	HWREG(BITBAND(SSI0_BASE + SSI_O_CR1, 4 /* EOT */)) = 1;
	ROM_SSIEnable(SSI0_BASE);
	ROM_IntEnable(INT_SSI0);
	// uDMA configuaration:
	ROM_uDMAChannelAssign(UDMA_CH10_SSI0RX);
	ROM_uDMAChannelAssign(UDMA_CH11_SSI0TX);
	ROM_uDMAChannelAttributeDisable(
		UDMA_CH11_SSI0TX,
		UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK | UDMA_ATTR_USEBURST);
	ROM_uDMAChannelAttributeDisable(
		UDMA_CH10_SSI0RX,
		UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK | UDMA_ATTR_USEBURST);
	// configure IRQ pin:
	ROM_GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0);
	ROM_GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_0, 0, GPIO_PIN_TYPE_STD_WPU);
	ROM_IntEnable(INT_GPIOE);
	GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_LOW_LEVEL);
	GPIOIntEnable(GPIO_PORTE_BASE, GPIO_PIN_0);
	// configure WTIMER2A for timeouts:
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER2);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_WTIMER2);
	ROM_IntEnable(INT_WTIMER2A);
	HWREG(WTIMER2_BASE + TIMER_O_CTL)   = 0;  // disable timer
	HWREG(WTIMER2_BASE + TIMER_O_CFG)   = 4;  // dual 32-bit timers
	HWREG(WTIMER2_BASE + TIMER_O_TAMR)  = TIMER_TAMR_TAMR_1_SHOT;
	HWREG(WTIMER2_BASE + TIMER_O_TAPR)  = 79; // set prescaler to 80 (count microseconds @ 80 MHz system clock)
	HWREG(WTIMER2_BASE + TIMER_O_IMR)   = TIMER_IMR_TATOIM;

	// internal state configuration:
	w433_rx_buffer.length = 0;
	w433_tx_buffer.length = 0;
	w433_rx_buffer.offset = 0;
	w433_tx_buffer.offset = 0;
	// schedule the modem sw-reset after W433_POWERON_DELAY  */
	w433_state = W433_STATE_RESETTING;
	W433_TIMEOUT(W433_POWERON_DELAY);
	W433_SDN = 0;
	w433_packet_received_cb = packet_received;
	nwk_connect_device('W', w433_send_packet);
	return 0;
}

void isr_W433 (void)
{
	// disable interrupt:
	HWREG(BITBAND(GPIO_PORTE_BASE + GPIO_O_IM, 0)) = 0;
	// signal event:
	W433_EVENT_INTERRUPT = 1;
}

void isr_timerW2A (void)
{
	// acknowledge interrupt:
	uint32_t status = HWREG(WTIMER2_BASE + TIMER_O_MIS);
	HWREG(WTIMER2_BASE + TIMER_O_ICR) = status & TIMER_MIS_TATOMIS;
	W433_EVENT_TIMEOUT = 1;
}

static volatile bool w433_dma_interrupt;

void isr_SSI0 (void)
{
	w433_dma_interrupt = true;
}

static uint8_t msg[250] = {
	0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
	0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
	0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
	0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
	0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
	0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
	0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
	0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB
};

#define ISR_TIMEOUT 0x10000

int w433_process_event (void)
{
	uint32_t isr = 0;
	if (W433_EVENT_INTERRUPT) {
		// clear event:
		W433_EVENT_INTERRUPT = 0;
		// read interrupt casuse:
		isr = w433_read_16BE(0x03);
//		serial_send_16("ISR=", isr, serial_eol);
		// re-enable interrupt:
		HWREG(BITBAND(GPIO_PORTE_BASE + GPIO_O_IM, 0)) = 1;
	}
	if (W433_EVENT_TIMEOUT) {
		// clear event:
		W433_EVENT_TIMEOUT = 0;
		// simulate special interrupt:
		isr |= ISR_TIMEOUT;
//		serial_send("TIMEOUT!");
//		serial_send(serial_eol);
	}
	if (W433_EVENT_TEST) {
		// clear event:
		W433_EVENT_TEST = 0;
		// send test packet:
		struct packet p;
		p.data = msg;
		p.length = sizeof msg;
		return w433_send_packet(&p);
	}
	switch (w433_state) {
		case W433_STATE_RESETTING:
			if (isr & ISR_ICHIPRDY) {
				W433_TIMEOUT_DISABLE();
				w433_state = W433_STATE_INITIALIZING;
			} else if (isr & ISR_TIMEOUT) {
				w433_state = W433_STATE_DISCONNECTED;
				break;
			} else {
				break;
			}
		case W433_STATE_INITIALIZING:
			// send the configuration table to the RFM23B:
			for (const uint8_t *p = RFM23BP_config; *p; p += 2)
				w433_single_transaction(p[0], p[1]);
			w433_state = W433_STATE_ABORTED;
			W433_TIMEOUT(W433_RX_ABORT_DELAY);
			BOARD_LED_GREEN = 1;
			break;
		case W433_STATE_ABORTED:
			goReceive();
			break;
		case W433_STATE_TRANSMIT:
			if (isr & ISR_ITXFFAEM) {
				/* TX FIFO almost empty (FIFO size <= threshold) */
				W433_TIMEOUT(W433_TX_TIMEOUT);
				feedTxFifo(W433_TX_FIFO_LEN - W433_TX_FIFO_AEM - 1);
			}
			if (isr & ISR_IPKSENT) {
				/* Packet sent */
				W433_TIMEOUT_DISABLE();
				goReceive();
			}
			if (isr & ISR_TIMEOUT) {
				goAbort();
			}
			if (isr & ISR_IFFERR) {
				BOARD_LED_BLUE = 1;
				serial_send_16("ISR=", isr, serial_eol);
				goAbort();
			}
			break;
		case W433_STATE_RECEIVE:
		case W433_STATE_RECEIVING:
			if (isr & ISR_IRXFFAFULL) {
				/* RX FIFO almost full (FIFO size > threshold) */
				w433_state = W433_STATE_RECEIVING;
				W433_TIMEOUT(W433_RX_TIMEOUT);
				if (w433_rx_buffer.length > W433_PACKET_MAX_LENGTH - W433_RX_FIFO_AFULL) {
					goAbort();
					return 0;
				} else {
					emptyRxFifo(W433_RX_FIFO_AFULL);
				}
			}
			if (isr & ISR_IPKVALID) {
				/* Valid packet received */
				W433_TIMEOUT_DISABLE();
				/* Read the total packet length after reception */
				size_t totLen = w433_single_transaction(0x4B, 0);
				/* Read the received packet's RSSI */
				uint8_t rssi = w433_single_transaction(0x26, 0);
				/* Check the packet length and the already received bytes */
				if (totLen < w433_rx_buffer.length || totLen > W433_PACKET_MAX_LENGTH) {
					goAbort();
					return 0;
				}
				/* Read the remaining bytes from rx FIFO */
				emptyRxFifo(totLen - w433_rx_buffer.length);

// 				serial_send_16("L=", totLen, " : ");
// 				serial_send_h(w433_rx_buffer.data, 16);
// 				serial_send(serial_eol);
// 				serial_wait();

				/* Go to RECEIVE state before calling the packet-received callback, because */
				/*   the callback may initiate a new transmission */
				goReceive();
				/* Signal data reception and pass the packet to the upper layer */
				if (w433_packet_received_cb) {
					struct packet incoming_pkt;
					incoming_pkt.device          = 'W';
					incoming_pkt.flags           = 0;
					incoming_pkt.signal_strength = rssiToDb(rssi);
					incoming_pkt.signal_quality  = 0;
					incoming_pkt.timestamp       = 0;
					incoming_pkt.length          = totLen;
					incoming_pkt.data            = w433_rx_buffer.data;  // preserved by goIdle()
					w433_packet_received_cb(&incoming_pkt);
				}
			}
			if (isr & ISR_TIMEOUT) {
				goAbort();
			}
			if (isr & ISR_ICRCERROR) {
				goAbort();
			}
			if (isr & ISR_IFFERR) {
				BOARD_LED_RED = 1;
				serial_send_16("ISR=", isr, serial_eol);
				goAbort();
			}
			break;
	}
	return 0;
}



/* Function used to realize a single SPI transaction for RFM23B.
 * It sends one data byte or returns the byte at the given address.
 * If MSb bit of address field is 1 the function perform an SPI write
 * @ address and return 0. In case of MSb equal to 0 the function perform
 * an SPI read and return the data read at the given address. */
static uint8_t w433_single_transaction (uint8_t address, uint8_t data)
{
	__asm("cpsid i");
	HWREG(SSI0_BASE + SSI_O_DR) = address;
	HWREG(SSI0_BASE + SSI_O_DR) = data;
	__asm("cpsie i");
	while (HWREG(BITBAND(SSI0_BASE + SSI_O_SR, 4 /* BSY */ ))) {}
	(void) HWREG(SSI0_BASE + SSI_O_DR);
	data = HWREG(SSI0_BASE + SSI_O_DR);
	return data;
}

// read a 16-bit word in big-endian order starting from address given:
static uint16_t w433_read_16BE (uint8_t address)
{
	__asm("cpsid i");
	HWREG(SSI0_BASE + SSI_O_DR) = address;
	HWREG(SSI0_BASE + SSI_O_DR) = 0;
	HWREG(SSI0_BASE + SSI_O_DR) = 0;
	__asm("cpsie i");
	while (HWREG(BITBAND(SSI0_BASE + SSI_O_SR, 4 /* BSY */ ))) {}
	(void) HWREG(SSI0_BASE + SSI_O_DR);
	uint8_t data_hi = HWREG(SSI0_BASE + SSI_O_DR);
	uint8_t data_lo = HWREG(SSI0_BASE + SSI_O_DR);
	return data_lo + ((uint16_t) data_hi << 8);
}


static uint8_t burst_tx_register[] = {0x7F + 0x80};
static uint8_t burst_rx_register[] = {0x7F};
static uint8_t dummy[1];

static void feedTxFifo (size_t len)
{
	size_t remaining = w433_tx_buffer.length - w433_tx_buffer.offset;
	if (len > remaining) len = remaining;
	if (!len) return; // no more bytes to upload

	tDMAControlTable SPI_TX_burst[] = {
		uDMATaskStructEntry(
			1, UDMA_SIZE_8,
			UDMA_SRC_INC_NONE, (void *) burst_tx_register,
			UDMA_DST_INC_NONE, (void *) (SSI0_BASE + SSI_O_DR),
			UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER
		),
		uDMATaskStructEntry(
			len, UDMA_SIZE_8,
			UDMA_SRC_INC_8, (void *) (w433_tx_buffer.data + w433_tx_buffer.offset),
			UDMA_DST_INC_NONE, (void *) (SSI0_BASE + SSI_O_DR),
			UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER
		)
	};

	ROM_SSIDMAEnable(SSI0_BASE, SSI_DMA_RX | SSI_DMA_TX);

	// RX:
	ROM_uDMAChannelControlSet(
		UDMA_CH10_SSI0RX | UDMA_PRI_SELECT,
		UDMA_SIZE_8 | UDMA_DST_INC_NONE | UDMA_SRC_INC_NONE | UDMA_ARB_4
	);
	ROM_uDMAChannelTransferSet(
		UDMA_CH10_SSI0RX | UDMA_PRI_SELECT,
		UDMA_MODE_BASIC,
		(void *) (SSI0_BASE + SSI_O_DR),
		(void *) dummy,
		len + 1
	);

	// TX:
	ROM_uDMAChannelControlSet(
		UDMA_CH11_SSI0TX | UDMA_PRI_SELECT,
		UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_32 | UDMA_ARB_4
	);
	ROM_uDMAChannelScatterGatherSet(
		UDMA_CH11_SSI0TX | UDMA_PRI_SELECT,
		2, SPI_TX_burst,
		true /* PERIPHERAL */
	);

	ROM_uDMAChannelEnable(UDMA_CH10_SSI0RX);
	ROM_uDMAChannelEnable(UDMA_CH11_SSI0TX);
	// wait for transmission to complete. TODO: avoid spinning?
	while (!w433_dma_interrupt) ROM_SysCtlSleep();
	w433_dma_interrupt = 0;

	ROM_SSIDMADisable(SSI0_BASE, SSI_DMA_RX | SSI_DMA_TX);

	w433_tx_buffer.offset += len;
}




static void emptyRxFifo (size_t len)
{
	if (!len) return;

	tDMAControlTable SPI_RX_burst[] = {
		uDMATaskStructEntry(
			1, UDMA_SIZE_8,
			UDMA_SRC_INC_NONE, (void *) (SSI0_BASE + SSI_O_DR),
			UDMA_DST_INC_NONE, (void *) dummy,
			UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER
		),
		uDMATaskStructEntry(
			len, UDMA_SIZE_8,
			UDMA_SRC_INC_NONE, (void *) (SSI0_BASE + SSI_O_DR),
			UDMA_DST_INC_8, (void *) (w433_rx_buffer.data + w433_rx_buffer.offset),
			UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER
		)
	};

	ROM_SSIDMAEnable(SSI0_BASE, SSI_DMA_RX | SSI_DMA_TX);

	// RX:
	ROM_uDMAChannelControlSet(
		UDMA_CH10_SSI0RX | UDMA_PRI_SELECT,
		UDMA_SIZE_32 | UDMA_SRC_INC_32 | UDMA_DST_INC_32 | UDMA_ARB_4
	);
	ROM_uDMAChannelScatterGatherSet(
		UDMA_CH10_SSI0RX | UDMA_PRI_SELECT,
		2, SPI_RX_burst,
		true /* PERIPHERAL */
	);

	// TX:
	ROM_uDMAChannelControlSet(
		UDMA_CH11_SSI0TX | UDMA_PRI_SELECT,
		UDMA_SIZE_8 | UDMA_DST_INC_NONE | UDMA_SRC_INC_NONE | UDMA_ARB_4
	);
	ROM_uDMAChannelTransferSet(
		UDMA_CH11_SSI0TX | UDMA_PRI_SELECT,
		UDMA_MODE_BASIC,
		(void *) burst_rx_register,
		(void *) (SSI0_BASE + SSI_O_DR),
		len + 1
	);

	ROM_uDMAChannelEnable(UDMA_CH10_SSI0RX);
	ROM_uDMAChannelEnable(UDMA_CH11_SSI0TX);
	// wait for transmission to complete. TODO: avoid spinning?
	while (!w433_dma_interrupt) ROM_SysCtlSleep();
	w433_dma_interrupt = 0;

	ROM_SSIDMADisable(SSI0_BASE, SSI_DMA_RX | SSI_DMA_TX);

	w433_rx_buffer.offset += len;
	w433_rx_buffer.length += len;
}




/* Send packet function */
static int w433_send_packet (struct packet const *p)
{
	if (p->length > W433_PACKET_MAX_LENGTH) return -EMSGSIZE;
	/* TODO: !!! Here we should test the GPIO of the modem to ensure that modem isn't receiving */
	if (w433_state != W433_STATE_RECEIVE) return -EBUSY;
	goIdle();  // reset state machine
	memcpy(w433_tx_buffer.data, p->data, p->length);
	w433_tx_buffer.length = p->length;
	w433_tx_buffer.offset = 0;
	/* Write data to tx FIFO */
	feedTxFifo(W433_TX_FIFO_LEN);
	/* Write the TX packet length in the 0x3E length register */
	w433_single_transaction(0x3E + 0x80, p->length);
	/* Setup a timeout for the pending transmission */
	W433_TIMEOUT(W433_TX_TIMEOUT);
	/* Switch to transmit state */
	goTransmit();
	return 0;
}



static void goIdle (void)
{
	// abort all operations
	w433_single_transaction(0x07 + 0x80, 0x01);  // switch to idle
//	w433_single_transaction(0x03, 0);  // clear interrupt flags
//	w433_single_transaction(0x04, 0);  //  "              "
	w433_single_transaction(0x08 + 0x80, 0x03);  // clear FIFOs
	w433_single_transaction(0x08 + 0x80, 0x00);  //  "      "
	// reset buffers
	w433_rx_buffer.offset = 0;
	w433_rx_buffer.length = 0;
	w433_tx_buffer.offset = 0;
	w433_tx_buffer.length = 0;
}


static void goReceive (void)
{
	goIdle();
	w433_state = W433_STATE_RECEIVE;
	w433_single_transaction(0x07 + 0x80, 0x05);  // rx mode
}


static void goTransmit (void)
{
	w433_state = W433_STATE_TRANSMIT;
	w433_single_transaction(0x07 + 0x80, 0x09);  // tx mode
}


static void goAbort (void)
{
	goIdle();
	w433_state = W433_STATE_ABORTED;
	W433_TIMEOUT(W433_RX_ABORT_DELAY);
}




static int8_t rssiToDb (uint8_t rssi)
{
	// See RFM23B datasheet, section "RSSI and Clear Channel Assessment"
	if (rssi < 10) return -120;
	if (rssi > 230) return -8;
	static const float x1 = -120.f, y1 = 10.f, x2 = -8.f, y2 = 230.f;
	return x1 + (x2 - x1) / (y2 - y1) * (rssi - y1);
}


