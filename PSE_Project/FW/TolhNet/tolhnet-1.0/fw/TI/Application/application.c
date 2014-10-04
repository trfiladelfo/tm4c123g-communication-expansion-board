/**
  ******************************************************************************
  * @file    Application/application.c
  * @author  Michele Alessandrini, Giorgio Biagetti
  * @version V1.0.0
  * @date    21-May-2013
  * @brief   Application layer source file.
  ******************************************************************************
  */

#include "application.h"

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>

#include <string.h>

#include "../board.h"
#include "../Network/errors.h"

static bool network_up = false;

/* Application datagram */
struct data_gram
{
	uint8_t   *regData;
	uint8_t   rawData[250];
	uint8_t   code;
	uint8_t   regDataSize;
};


// Application-level register addresses:

typedef int (*RegisterFunction) (struct data_gram *);

static int invalidRegister (struct data_gram *);
static int setLoad1 (struct data_gram *);
static int getLoad1 (struct data_gram *);
static int setLoad2 (struct data_gram *);
static int getLoad2 (struct data_gram *);
static int setPwm (struct data_gram *);
static int getPwm (struct data_gram *);
static int setAdc (struct data_gram *);
static int getAdc (struct data_gram *);
static int getUniqueId (struct data_gram *);

//  table of function pointer pairs: {set, get}
static const RegisterFunction registerTable[][2] = {
	{ invalidRegister, invalidRegister },
	{ setLoad1, getLoad1 },
	{ setLoad2, getLoad2 },
	{ setPwm, getPwm},
	{ setAdc, getAdc},
	{ invalidRegister, invalidRegister },
	{ invalidRegister, getUniqueId },
};

#define registerTableSize (sizeof(registerTable) / sizeof(RegisterFunction) / 2)

void app_init (void)
{
}


/* Network event callback definition */
void network_event_handler (enum nwk_events event_type)
{
		network_up = (event_type == nwk_network_up || event_type == nwk_network_reconf);
}

/* Function used to signal data reception at application layer */
void data_received (uint16_t src_address, uint16_t seq, int8_t code, void *data, size_t length) {
	(void)seq;
	struct data_gram datagram;
	/* save local datagram */
	if (length < 1 || code < 0) return;
	datagram.code = code;
	/* TODO: Add support for more multi-byte address */
	memcpy(datagram.rawData, data, length);
	datagram.regData = datagram.rawData + 1;
	datagram.regDataSize = length - 1;

	uint8_t address = *datagram.rawData;  // TODO: 64 bit
	RegisterFunction f = address >= registerTableSize ? invalidRegister :
		datagram.code == CODE_SET ? registerTable[address][0] :
		datagram.code == CODE_GET ? registerTable[address][1] :
		invalidRegister;
	datagram.code = CODE_ACK;  // default, can be changed by register function
	int e = f(&datagram);
	if (e < 0) {
		datagram.code = CODE_NACK;
		datagram.regData[0] = -e;
		datagram.regDataSize = 1;
	} else {
		datagram.regDataSize = e;
	}
	if (network_up) {
		send_datagram(src_address, datagram.regData, datagram.regDataSize, datagram.code);
	}
}


static int invalidRegister (struct data_gram *dg)
{
	dg->code = CODE_NACK;
	return 0;
}


static int setLoad1 (struct data_gram *dg)
{
	if (dg->regDataSize != 1) return -EINVAL;
	BOARD_LED_RED = !!dg->regData[0];
	return 0;
}

static int getLoad1 (struct data_gram *dg)
{
	if (dg->regDataSize) return -EINVAL;
	dg->regData[0] = BOARD_LED_RED;
	return 1;
}

static int setLoad2 (struct data_gram *dg)
{
	if (dg->regDataSize != 1) return -EINVAL;
	BOARD_LED_GREEN = !!dg->regData[0];
	return 0;
}

static int getLoad2 (struct data_gram *dg)
{
	if (dg->regDataSize) return -EINVAL;
	dg->regData[0] = BOARD_LED_GREEN;
	return 1;
}


static int setPwm (struct data_gram *dg)
{
	return -ENOSYS;
}

static int getPwm (struct data_gram *dg)
{
	return -ENOSYS;
}

static int setAdc (struct data_gram *dg)
{
	return -ENOSYS;
}

static int getAdc (struct data_gram *dg)
{
	return -ENOSYS;
}


static int getUniqueId (struct data_gram *dg)
{
	if (dg->regDataSize) return -EINVAL;
	memcpy(dg->regData, (void const *) SYSCTL_BASE, 8);
	return 8;
}

