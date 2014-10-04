// plc_mac.c
// Copyright (C) 2014 Giorgio Biagetti.
// Based on previous work by Francesco Ricciardi, (C) 2013 CEDAR Solutions.

#include "plc_mac.h"
#include "../../Network/network.h"
#include "../../Network/errors.h"
#include "../../Drivers/uartdma.h"
#include "../../Scheduler/tasks.h"
#include "../../Scheduler/event_scheduler.h"
#include "st7580.h"

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>

/*
 * Defining ST7580 modem parameters!
 */
#define ST7580_USE_MIB_FREQUENCY 0
#define ST7580_FREQUENCY_OVERWRITE 0
#define ST7580_USE_HIGH_FREQUENCY 4
#define ST7580_USE_MIB_TX_GAIN 0
#define ST7580_MODULATION_BPSK 0
#define ST7580_ZEROCROSS_SYNCHRO 0


/* STATE MACHINE STATE DEFINITION */
#define PLC_STATE_INITIALIZING                0x00
#define PLC_STATE_IDLE                        0x01
#define PLC_STATE_RECEIVE                     0x02
#define PLC_STATE_CHECKSUM_CONTROL            0x03
#define PLC_STATE_CHECKSUM_OK                 0x04
#define PLC_STATE_CHECKSUM_ERROR              0x05
#define PLC_STATE_TRANSMIT_TREQ_WAITING       0x06
#define PLC_STATE_TRANSMIT_STATUS_RECEIVED    0x07
#define PLC_STATE_TRANSMIT_INIT               0x08
#define PLC_STATE_TRANSMIT                    0x09
#define PLC_STATE_TRANSMIT_ACK_WAITING        0x0A
#define PLC_STATE_ERROR                       0x0B
#define PLC_STATE_STATUS_BYTE_WAITING         0x0C
#define PLC_STATE_MODEM_BUSY                  0x0D
#define PLC_STATE_MODEM_CONF_ERROR            0x0E

volatile int plc_state = PLC_STATE_INITIALIZING;
volatile int service_request = 0;

/* Buffer declaration */
uint8_t plc_tx_buffer[256];

/* Tx, Rx enable flags */
bool plc_enable_tx = false;
bool plc_enable_rx = false;

/* CALLBACK DECLARATION */
/* Signal the reception of a packet */
static void (*plc_packet_received_cb) (struct packet const *p) = NULL;
/* Signal the transmission of a packet */
//static void (*plc_packet_transmitted_cb) (void *pkt, int result) = NULL;


/* Local function prototypes: */
int plc_send_packet (struct packet const *p);

void plc_modem_reset (void);


/* TODO: adjust the buffering mechanism */


#define PLC_TREQ      HWREGBITW(GPIO_PORTB_BASE + (GPIO_PIN_5 << 2), 5)

#define PLC_RX_ON 0
#define PLC_TX_ON 0

/* Interface initialization function */
int plc_mac_init (void)
{
	/* set initial machine state: */
	plc_state = PLC_STATE_IDLE;
	/* initialize GPIO: */
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);
	PLC_TREQ = 1;
	/* initialize serial port: */
	uart_init(2, 57600, 10000); // [P] UART2 on pins J4.08-09 to ST7580 PLC modem
	/* enable TX and RX */
	plc_enable_receive(true);
	plc_enable_transmit(true);
	/* Callback connection */
	plc_packet_received_cb = packet_received;
	nwk_connect_device('P', plc_send_packet);
	return 0;
}


static char responses[2] = {'\006', '\012'}; // {ACK, NACK}

int plc_receive_packet (int source, uint8_t *uart_rx_buffer, uint8_t rx_len)
{
	if (plc_state == PLC_STATE_TRANSMIT_TREQ_WAITING) {
		// we are expecting the modem status indication
		if (rx_len == 2 && uart_rx_buffer[0] == STATUS_START_CODE) {
			// status received. Check if the modem is properly configured
			uint8_t status = uart_rx_buffer[1];
			if (status & STATUS_BYTE_CONFIGURATION_MASK) {
				if (!(status & STATUS_BYTE_TRANSMISSION_MASK) && !(status & STATUS_BYTE_RECEPTION_MASK)) {
					plc_state = PLC_STATE_TRANSMIT_INIT;

					uart_send_frame(source, plc_tx_buffer, 1); // only send the first byte (STX)
					// TODO: wait for transmission to complete...
					plc_state = PLC_STATE_TRANSMIT;
					PLC_TREQ = 1; // release TREQ
					uart_send_frame(source, plc_tx_buffer + 1, plc_tx_buffer[1] + 4); // send the remaining part
				} else {
					/* Updating machine state: modem busy. */
					plc_state = PLC_STATE_MODEM_BUSY;
					/* Call the callback and notify error */
					/* TODO: che puntatore usare? */
					//if(plc_packet_transmitted_cb) plc_packet_transmitted_cb(upper_layer_packet_ptr, EBUSY);
					PLC_TREQ = 1; // added by giorby
					plc_state = PLC_STATE_IDLE; // added by giorby
				}
			} else {
				/* Updating machine state: modem configuration error. */
				PLC_TREQ = 1; // added by giorby
				plc_state = PLC_STATE_MODEM_CONF_ERROR;
				/* Call the callback and notify error */
				//if(plc_packet_transmitted_cb) plc_packet_transmitted_cb(upper_layer_packet_ptr, ENONET);
				/* Try to reset the modem */
				plc_modem_reset();
			}
			return 0;
		} else {
			// received something unexpected... abort anyway and see what's coming to us...
			PLC_TREQ = 1; // added by giorby
			plc_state = PLC_STATE_IDLE; // added by giorby
		}
	}
// TODO: PLC_STATE_TRANSMIT_ACK_WAITING

	if (rx_len < 5 || (uart_rx_buffer[0] != '\002' && uart_rx_buffer[0] != '\003')) return 0;

	/* compute checksum */
	uint16_t checksum = 0;
	size_t i, len = uart_rx_buffer[1];
	if (len + 5 != rx_len) return -EMSGSIZE;
	for (i = 1; i < len + 3; ++i)
		checksum += uart_rx_buffer[i];

	/* check checksum */
	uint16_t rx_checksum = uart_rx_buffer[i] + ((uint16_t) uart_rx_buffer[i + 1] << 8);
	if (rx_checksum == checksum) {
		uart_send_frame(source, responses + 0, 1); // 0 = ACK

		/* Set the state to idle */
		plc_state = PLC_STATE_IDLE;

		/* Command selection (partially implemented) */
		/* TODO: complete with the other command codes */
		/* TODO: important! Data confirmation command or ack to free the memory? */
		switch(uart_rx_buffer[2]) {
			/* Confirm section */
			case BIO_RESET_CONFIRM:     /* Reset ok! */
				break;
			case MIB_WRITE_CONFIRM:     /* Write ok! */
				break;
			case MIB_READ_CONFIRM:      /* MIB read ok! */
				break;
			case MIB_ERASE_CONFIRM:      /* MIB erase ok! */
				break;
			case PING_CONFIRM:          /* Ping ok! */
				break;
			case PHY_DATA_CONFIRM:      /* Data transmitted! */
				break;
			case DL_DATA_CONFIRM:       /* Data transmitted! */
				/* TODO: which pointer use? */
				//if(plc_packet_transmitted_cb) plc_packet_transmitted_cb(upper_layer_packet_ptr, 0);
				break;
			case SS_DATA_CONFIRM:       /* Data transmitted! */
				break;
			/* Error section */
			case BIO_RESET_ERROR:       /* Reset error! */
				break;
			case MIB_WRITE_ERROR:       /* MIB write error! */
				break;
			case MIB_READ_ERROR:        /* MIB read error! */
				break;
			case MIB_ERASE_ERROR:       /* MIB erase error! */
				break;
			case PHY_DATA_ERROR:        /* PHY data error! */
				break;
			case DL_DATA_ERROR:         /* DL data error! */
				/* Call the callback and notify packet transimission error */
				/* TODO: which pointer use? */
				//if(plc_packet_transmitted_cb) plc_packet_transmitted_cb(upper_layer_packet_ptr, EIO);
				break;
			case SS_DATA_ERROR:         /* SS data error! */
				break;
			/* Indication section */
			case BIO_RESET_INDICATION:  /* Reset ok! Send the machine state in idle. */
				if (plc_state == PLC_STATE_MODEM_CONF_ERROR) plc_state = PLC_STATE_IDLE;
				break;
			case PHY_DATA_INDICATION:   /* Data received! */
				break;
			case DL_DATA_INDICATION:    /* Data received! */
				if (plc_packet_received_cb && plc_enable_rx) {
					/* Prepare packet structure */
					struct packet incoming_pkt;
					incoming_pkt.device          = 'P';
					incoming_pkt.flags           = 0;
					incoming_pkt.signal_strength = 6 * (4 - (uart_rx_buffer[3] >> 4));  // PGA to DB (see datasheet), here: 6 - PGA gain
					incoming_pkt.signal_quality  = uart_rx_buffer[4];  // SNR, signed value (see UM0932 user manual)
					incoming_pkt.timestamp       = (uart_rx_buffer[5] << 8) | uart_rx_buffer[6];
					incoming_pkt.length          = uart_rx_buffer[1] - 4;
					incoming_pkt.data            = uart_rx_buffer + 7;
					plc_packet_received_cb(&incoming_pkt);
				}
				break;
			case DL_SNIFFER_INDICATION: /* Data received with error! DO nothing! */
				break;
			case SS_DATA_INDICATION:    /* Data received! */
				break;
			case SS_SNIFFER_INDICATION: /* Data received with error! */
				break;
		}
	} else {
		uart_send_frame(source, responses + 1, 1); // 1 = NACK
		plc_state = PLC_STATE_IDLE;
		return -EINVAL;
	}
	return 0;
}



/* Send packet function */
int plc_send_packet (struct packet const *p)
{
	if (!plc_enable_tx) return -ENODEV;
	if (p->length > PLC_PACKET_MAX_LENGTH) return -EMSGSIZE;
	/* check to see if the PLC interface is not busy: */
	if (plc_state == PLC_STATE_INITIALIZING) return -ENONET;
	if (plc_state == PLC_STATE_ERROR) return -EIO;
	if (plc_state == PLC_STATE_IDLE) {
		/* Frame preparation: header construction */
		plc_tx_buffer[0] = '\002';
		plc_tx_buffer[1] = p->length + 1; // add one for configuration byte
		plc_tx_buffer[2] = DL_DATA_REQUEST;
		plc_tx_buffer[3] = ST7580_USE_MIB_FREQUENCY | ST7580_FREQUENCY_OVERWRITE | ST7580_USE_HIGH_FREQUENCY | ST7580_USE_MIB_TX_GAIN | ST7580_MODULATION_BPSK | ST7580_ZEROCROSS_SYNCHRO;

		/* Frame preparation: initial setup of checksum */
		uint16_t checksum = plc_tx_buffer[1] + plc_tx_buffer[2] + plc_tx_buffer[3];

		/* Frame preparation: copy data and compute checksum */
		size_t i;
		for (i = 0; i < p->length; ++i)
			checksum += plc_tx_buffer[4 + i] = *((uint8_t const *) p->data + i);

		/* Frame preparation: append checksum */
		i += 4; // skip header
		plc_tx_buffer[i++] = (uint8_t) (checksum & 0xFF);
		plc_tx_buffer[i++] = (uint8_t) ((checksum >> 8) & 0xFF);

		/* should check if the modem is busy, but RX_ON and TX_ON lines are not available on this demo board! */
		if (PLC_RX_ON || PLC_TX_ON) return -EBUSY;

		plc_state = PLC_STATE_TRANSMIT_TREQ_WAITING;
		PLC_TREQ = 0;
		uart_start_timeout(2, 200000);
		/* Start a timeout if the status reply doesn't arrive  */
//		event_schedule(TASK_PLC_STATE_TRANSMIT_TREQ_WAITING, 20);

		/* If the modem will be ready for transmission it will be initialized inside the
			* pls_start_detect_ISR */
		return 0;
	} else {
		return -EBUSY;
	}
}


/* Function used to reset the modem */
void plc_modem_reset (void)
{
	/* prepare frame: */
	plc_tx_buffer[0] = '\002';
	plc_tx_buffer[1] = 0;
	plc_tx_buffer[2] = BIO_RESET_REQUEST;
	uint16_t checksum = plc_tx_buffer[1] + plc_tx_buffer[2];
	plc_tx_buffer[3] = (uint8_t) (checksum & 0xFF);
	plc_tx_buffer[4] = (uint8_t) ((checksum >> 8) & 0xFF);

	/* Signal a service request to avoid undesired ACK signal to upper layer */
	service_request = 1;
	/* Update machine state */
	plc_state = PLC_STATE_TRANSMIT_TREQ_WAITING;
	PLC_TREQ = 0;
	/* Start a timeout if the status reply doesn't arrive  */
	event_schedule(TASK_PLC_STATE_TRANSMIT_TREQ_WAITING, 20); // added by giorby 2014-05-07
}


/* Enable frame receive operation */
int plc_enable_receive (bool en)
{
	plc_enable_rx = en;
	return 0;
}

/* Enable frame transmit operation */
int plc_enable_transmit (bool en)
{
	plc_enable_tx = en;
	return 0;
}
