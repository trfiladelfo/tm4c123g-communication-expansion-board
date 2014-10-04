// uart_mac.c
// Copyright (C) 2014 Giorgio Biagetti.
// Based on previous work by Francesco Ricciardi, (C) 2013 CEDAR Solutions.

#include "uart_mac.h"
#include "../../Network/network.h"
#include "../../Network/errors.h"
#include "../../Drivers/uartdma.h"

/* Buffer declaration */ // TODO: sizes!
uint8_t uart_tx_buffer[256];

/* Tx, Rx enable flags */
static bool uart_enable_tx = false;
static bool uart_enable_rx = false;

/* CALLBACK DECLARATION */
static void (*uart_packet_received_cb) (struct packet const *p) = NULL;

/* Local function prototypes: */
int uart_send_packet_to_A (struct packet const *p);
int uart_send_packet_to_B (struct packet const *p);
int uart_send_packet_to_C (struct packet const *p);
int uart_send_packet (struct packet const *p, unsigned port);


/* Interface initialization function */
int uart_mac_init (void)
{
	/* Call the hardware initialization function */
	uart_init(0, 1000000, 10000); // [A] UART0 via USB debug interface
	uart_init(1, 1000000, 10000); // [B] UART1 on pins J1.03-04
	uart_init(3, 1000000, 10000); // [C] UART3 on pins J4.06-07
	/* Enabling TX and RX */
	uart_enable_receive(true);
	uart_enable_transmit(true);
	/* Callback connection */
	uart_packet_received_cb = packet_received;
	nwk_connect_device('A', uart_send_packet_to_A);
	nwk_connect_device('B', uart_send_packet_to_B);
	nwk_connect_device('C', uart_send_packet_to_C);
	return 0;
}


static char responses[2] = {'\006', '\012'}; // {ACK, NACK}

int uart_receive_packet (int source, uint8_t *uart_rx_buffer, uint8_t rx_len)
{
	/* compute checksum */
	uint16_t checksum = 0;
	size_t i, len = uart_rx_buffer[1];
	if (len + 5 != rx_len) return -EMSGSIZE;
	for (i = 1; i < len + 3; ++i)
		checksum += uart_rx_buffer[i];

	/* check checksum */
	uint16_t rx_checksum = uart_rx_buffer[i] + ((uint16_t) uart_rx_buffer[i + 1] << 8);
	if (rx_checksum == checksum && uart_rx_buffer[2] == '$') {
		uart_send_frame(source, responses + 0, 1); // 0 = ACK
		if (uart_packet_received_cb && uart_enable_rx) {
			/* prepare packet structure */
			struct packet incoming_pkt;
			incoming_pkt.device          = source["AB C"];
			incoming_pkt.flags           = 0;
			incoming_pkt.signal_strength = 0;
			incoming_pkt.signal_quality  = 0;
			incoming_pkt.timestamp       = 0;
			incoming_pkt.length          = len;
			incoming_pkt.data            = uart_rx_buffer + 3;
			uart_packet_received_cb(&incoming_pkt);
		}
	} else {
		uart_send_frame(source, responses + 1, 1); // 1 = NACK
		return -EINVAL;
	}
	return 0;
}


/* Send packet functions */

int uart_send_packet_to_A (struct packet const *p)
{
	return uart_send_packet(p, 0);
}

int uart_send_packet_to_B (struct packet const *p)
{
	return uart_send_packet(p, 1);
}

int uart_send_packet_to_C (struct packet const *p)
{
	return uart_send_packet(p, 3);
}

int uart_send_packet (struct packet const *p, unsigned port)
{
	if (!uart_enable_tx) return -ENODEV;
	if (p->length > UART_PACKET_MAX_LENGTH) return -EMSGSIZE;

	/* Frame preparation: header construction */
	uart_tx_buffer[0] = '\002';
	uart_tx_buffer[1] = p->length;
	uart_tx_buffer[2] = '$';

	/* Frame preparation: initial setup of checksum */
	uint16_t checksum = uart_tx_buffer[1] + uart_tx_buffer[2];

	/* Frame preparation: copy data and compute checksum */
	size_t i;
	for (i = 0; i < p->length; ++i)
		checksum += uart_tx_buffer[3 + i] = *((uint8_t const *) p->data + i);

	/* Frame preparation: append checksum */
	i += 3; // skip header
	uart_tx_buffer[i++] = (uint8_t) (checksum & 0xFF);
	uart_tx_buffer[i++] = (uint8_t) ((checksum >> 8) & 0xFF);

	/* Send data */
	if (!uart_send_frame(port, uart_tx_buffer, i)) return -EBUSY;
	return 0;
}

/* Enable frame receive operation */
int uart_enable_receive (bool en)
{
	uart_enable_rx = en;
	return 0;
}

/* Enable frame transmit operation */
int uart_enable_transmit (bool en)
{
	uart_enable_tx = en;
	return 0;
}

