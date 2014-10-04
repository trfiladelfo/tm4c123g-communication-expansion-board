//==========================================================================
// tolhnet.cpp
// Implementation of the tolhnet master server
/***************************************************************************
 * Copyright (C) 2013-2014 Giorgio Biagetti                                *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.            *
 **************************************************************************/

#include "tolhnet.h"
#include "serial.h"
#include "../network/routing.h"
#include "../network/errors.h"
#include "../network/utils.h"

#include "dispatcher.h"

#include <sys/time.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <iostream>

struct frame
{
    uint8_t data[256];
};

extern serial_io serial;

int sci_send_packet (packet const *p)
{
#ifdef NWK_DEBUG_DUMP
	p->dump(" <-", network.lookup_address(0)->handler->address());
#endif
	frame f;
	size_t length = p->length;
	if (length > sizeof f.data - 5) return -EMSGSIZE;
	f.data[0] = 0x02;   // STX
	f.data[1] = length; // payload length
	f.data[2] = '$';    // command code
	if (length) memcpy(f.data + 3, p->data, length);
	length += 3;
	uint16_t checksum = 0;
	for (size_t i = 1; i < length; ++i)
		checksum += f.data[i];
	f.data[length++] = (uint8_t) (checksum & 0xFF);
	f.data[length++] = (uint8_t) (checksum >> 8);
	serial.send(std::string((const char *) f.data, length));
	return 0;
}

bool wait_for_identification;

void sci_identify_peer ()
{
	serial.send("\005"); /* sends an ENQuiry */
	wait_for_identification = true;
	network.enable_interface(0, true);
}

int sci_dispatch_packets (std::string const &line, router *target)
{
	if (line.empty()) return -ERROR;
	if (line[0] == '\006' /*  ACK */ ) return CODE_ACK;
	if (line[0] == '\025' /* NACK */ ) return CODE_NACK;
	if (line[0] != '\002' /*  STX */ ) {
		printf(" :: %s", line.c_str());
		if (!wait_for_identification && line.size() > 16 && line.substr(0, 16) == "ToLHnet firmware") {
			connection_dispatcher::schedule_event(sci_identify_peer, 100000);
//			serial.send("\005"); /* sends an ENQuiry */
			wait_for_identification = true;
			return CODE_MSG;
		}
		if (line.size() > 12 && line.substr(0, 12) == "ToLHnet node") {
			wait_for_identification = false;
			// sends reset message:
			if (network.application_callback)
				network.application_callback(NULL, 0, CODE_RESET, NULL, 0);
			return CODE_RESET;
		}
	}

	if (wait_for_identification) return -EPROTO;

	frame f;
	if (line.size() < 5 || line.size() > sizeof f.data) return -EMSGSIZE;

	memcpy(f.data, line.data(), line.size());

	size_t length = f.data[1];
	if (length != line.size() - 5) {
		printf(" :: FRAME SIZE ERROR!\n");
		return -ERROR;
	}
	uint16_t data_checksum = 0;
	for (size_t i = 1; i < length + 3; ++i)
		data_checksum += f.data[i];
	uint16_t frame_checksum = f.data[length + 3] | ((uint16_t) f.data[length + 4] << 8);
	if (data_checksum != frame_checksum) {
		printf(" :: FRAME CHECKSUM ERROR!\n");
		return -ERROR;
	}

	packet pkt = {
		'X',
		0, 0, 0, 0,
		f.data[1],
		f.data + 3
	};
#ifdef NWK_DEBUG_DUMP
	pkt.dump(" ->", target->address());
#endif
	target->packet_received(&pkt);

	return CODE_MSG;
}


