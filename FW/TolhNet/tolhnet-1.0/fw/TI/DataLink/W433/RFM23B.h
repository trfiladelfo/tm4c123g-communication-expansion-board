/* Define Interrupt Status Register bit map */
#define ISR_IFFERR        0x8000
#define ISR_ITXFFAFULL    0x4000
#define ISR_ITXFFAEM      0x2000
#define ISR_IRXFFAFULL    0x1000
#define ISR_IEXT          0x0800
#define ISR_IPKSENT       0x0400
#define ISR_IPKVALID      0x0200
#define ISR_ICRCERROR     0x0100
#define ISR_ISWDET        0x0080
#define ISR_IPREAVAL      0x0040
#define ISR_IPREAINVAL    0x0020
#define ISR_IRSSI         0x0010
#define ISR_IWUT          0x0008
#define ISR_ILBD          0x0004
#define ISR_ICHIPRDY      0x0002
#define ISR_IPOR          0x0001

/* Definition of FIFO length */
#define W433_TX_FIFO_LEN   64
#define W433_RX_FIFO_LEN   64

#define W433_POWERON_DELAY  30000  // 30 ms
#define W433_RX_ABORT_DELAY  2000  //  2 ms

/*
 * Definition of the max timeout for data transmission.
 * With SPI prescaler set @ 16 in bsp.h the byte duration is
 * of about 3.5us. We can transmit max 256 byte per session
 * so the maximum transmission time is of 896us, but the average
 * time that the modem need to transmit 59 byte is of about
 * 8ms so we need to schedule this time inside the send_packet
 * and refresh it inside the TX FIFO almost empty branch of
 * ISR. The timeout is set @ 15ms.
 */
#define W433_TX_TIMEOUT       15000

/*
 * Definition of the timeout for pending receive status.
 *
 */
#define W433_RX_TIMEOUT       15000

/* Definition of TX FIFO almost empty threshold */
#define W433_TX_FIFO_AEM    32
/* Definition of RX FIFO almost full threshold */
#define W433_RX_FIFO_AFULL  32

/* Definition of transmit power
 * For the possible values of this parameter
 * see the HoperRF AN440 application notes
 * on page 51: "Register 6Dh. TX Power"
 * TX power = -8 dBm + W433_TX_POWER * 3 dBm
 * (available range: 0 = -8 dBm, 7 = +13 dBm)
 * */
#define W433_TX_POWER 0x03 /* Set transmit power to 1dBm */

/* Configuration table.
 * Configuration parameters are arranged in the form
 *
 *      register address + 0x80, register value
 *
 * the 0x80 is added to inform the modem that this is
 * a write operation.
 * */
static const uint8_t RFM23BP_config[] = {
	0x03 + 0x00, 0x00,     /* Clear IRQ */
	0x04 + 0x00, 0x00,

	0x75 + 0x80, 0x53,     /* 434 MHz frequency configuration */
	0x76 + 0x80, 0x64,
	0x77 + 0x80, 0x00,

	0x7D + 0x80, W433_TX_FIFO_AEM,     /* Setting TX FIFO almost empty threshold */
	0x7E + 0x80, W433_RX_FIFO_AFULL,      /* Setting RX FIFO almost full threshold */

	0x05 + 0x80, 0xB7,     /* interrupt: packet sent, packet received, TX almost empty, RX almost full, underflow/overflow, CRC error */
	0x06 + 0x80, 0x00,

	0x08 + 0x80, 0x03,     /* Software reset FIFO */
	0x08 + 0x80, 0x00,

	0x09 + 0x80, 0x7F,     /* Setting Crystal Oscillator Load Capacitance */

	0x1C + 0x80, 0x9A,     /* IF filter bandwidth */
	0x1D + 0x80, 0x40,     /* AFC Loop Gearshift Override */

	0x20 + 0x80, 0x3C,
	0x21 + 0x80, 0x02,
	0x22 + 0x80, 0x22,
	0x23 + 0x80, 0x22,
	0x24 + 0x80, 0x07,
	0x25 + 0x80, 0xFF,
	0x2A + 0x80, 0x48,

	0x30 + 0x80, 0xAC,     /* Enable packet RX and TX handler, CCIT CRC enabled over data */
	0x32 + 0x80, 0x8C,     /* Header control over byte 3 */
	0x33 + 0x80, 0x22,     /* Match header 3,2, no fixed pkt length, Sync word 3,2 */
	0x34 + 0x80, 0x08,     /* Preamble length of 32 bits */
	0x35 + 0x80, 0x22,     /* Preamble Detection over 16 bits, add 8 dB to RSSI*/
	0x36 + 0x80, 0x2D,     /* Sync words 3, 2, 1, 0 */
	0x37 + 0x80, 0xD4,
	0x38 + 0x80, 0x00,
	0x39 + 0x80, 0x00,
	0x3A + 0x80, 0xCE,     /* Tx Header bytes 3, 2, 1, 0 */
	0x3B + 0x80, 0xDA,
	0x3C + 0x80, 0x00,
	0x3D + 0x80, 0x55,
	0x3E + 0x80, 0x00,     /* Packet length (should be written every transmission) */
	0x3F + 0x80, 0xCE,     /* Rx Header bytes 3, 2, 1, 0 */
	0x40 + 0x80, 0xDA,
	0x41 + 0x80, 0x00,
	0x42 + 0x80, 0x55,
	0x43 + 0x80, 0xFF,     /* Enable header check over all bits */
	0x44 + 0x80, 0xFF,
	0x45 + 0x80, 0xFF,
	0x46 + 0x80, 0xFF,

	0x6D + 0x80, 0x00 + (0x07 & W433_TX_POWER), /* Set transmit power */
	0x6E + 0x80, 0x19,     /* Set data rate @ 100 kbit/s */
	0x6F + 0x80, 0x9A,

	0x70 + 0x80, 0x0C,     /* Disabling manchester encoding */
	0x71 + 0x80, 0x23,     /* No TX clock, enable FIFO, MSb of FDS, GFSK*/
	0x72 + 0x80, 0x38,     /* Setting frequency deviation to 35kHz */

	0x07 + 0x80, 0x05,     /* Send the modem in READY mode */
	
	0x00, 0x00             /* Terminator */
};

