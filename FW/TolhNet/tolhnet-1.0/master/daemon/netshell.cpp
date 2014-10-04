//==========================================================================
// netshell.cpp
// Implementation of the ToLHnet shell command interpreter
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

#include "netshell.h"
#include "../network/routing.h"
#include "../network/errors.h"
#include "../network/utils.h"

#include <sys/time.h>

#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <iostream>

const std::string netshell::protocol = "ToLHnet-1.0";

netshell::netshell (int socket) : controller(socket)
{
	test_mode = 0;
	timestamps = new timeval[2];
	char myname[64];
	myname[0] = gethostname(myname + 1, sizeof myname - 1) ? 0 : ' ';
	send("# +HELLO (" + protocol + myname + ")\r\n");
}

netshell::~netshell ()
{
	delete [] timestamps;
}

bool netshell::process_input ()
{
	char buf[1024];
	int n = ::read(connection, buf, sizeof buf);
	if (n <= 0) return false;

	// TODO: proper framing!!!!
	// strip trailing newline characters:
	while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == '\n')) --n;
	if (!n) return true;
	std::string line(buf, n);
	char type = line[0];

	if (type == '$' || type == '+' || type == '!' || type == '?') {
		if (last_sequence < 0) {
			// send a packet on the network:
			gettimeofday(timestamps + 0, NULL);
			last_sequence = network.send_command(std::string(buf, n));
			if (last_sequence < 0) send(": ~" + error_name(last_sequence) + "\r\n");
		} else {
			// didn't receive reply to previous packet yet...
			send(": ~EBUSY\r\n");
		}
	} else if (type != '~') {
		send("*ERROR\r\n");
	} else {
		// command:
		std::vector < std::string > const &fields = split_fields(line.substr(1));
		unsigned opts = fields.size();
		if (!opts--) {
			send("*ERROR\r\n");
			return true;
		}
		std::string const &cmd = fields[0];
		if (cmd == "HELP" && !opts) {
			send("? | LIST | START | STOP | HELP | QUIT\r\n");
			send("*\r\n");
		} else if (cmd == "?" && !opts) {
			for (std::map < char, bool > :: const_iterator i = network.interfaces.begin(); i != network.interfaces.end(); ++i) {
				std::string iface_name;
				if (i->first) {
					iface_name += ' ';
					iface_name += i->first;
				}
				if (i->second)
					send("# +CONNECT" + iface_name + "\r\n");
				else
					send("# -ENOLINK" + iface_name + "\r\n");
			}
			send("*\r\n");
		} else if (cmd == "START") {
			if (opts) {
				send("*ENOSYS\r\n");
			} else {
				network.start();
				send("*\r\n");
			}
		} else if (cmd == "STOP") {
			send("*ENOSYS\r\n");
		} else if (cmd == "LIST") {
			for (std::deque < node_description > :: const_iterator i = network.nodes.begin(); i != network.nodes.end(); ++i) {
				if (i->mac_address == net_handler::master_mac) continue;
				send(to_hex(i->mac_address, 48) + "@" + to_hex(i->hop_distance, 16) + " " + to_hex(i->net_address, 16) + " +|--- " + i->name + "\r\n");
			}
			send("*\r\n");
		} else if (cmd == "TEST" && opts == 1) {
			test_mode = atoi(fields[1].c_str());
			send("REPEAT " + to_dec(test_mode) + "\r\n");
			--test_mode;
			send("*\r\n");
		} else if (cmd == "QUIT") {
			send("*\r\n");
			return false;
		} else {
			send("*ERROR\r\n");
		}
	}
	return true;
}

void netshell::process_datagram (directed_command const *cmd, uint16_t src, int code, void *data, size_t length)
{
	std::string payload;
	if (data) payload = std::string((const char *) data, length);
	if (!cmd && code == CODE_MSG) {
		node_description const *node = network.lookup_address(src);
		send("#" + (node ? node->name : "") + " " + to_hex(payload, 0) + "\r\n");
	}
	if (!cmd && !src && code == CODE_LINKDOWN) {
		std::string iface_name;
		if (data && length == sizeof (char) && *(const char *) data) {
			iface_name += ' ';
			iface_name += *(const char *) data;
		}
		send("# -ENOLINK" + iface_name + "\r\n");
	}
	if (!cmd && !src && code == CODE_LINKUP) {
		std::string iface_name;
		if (data && length == sizeof (char) && *(const char *) data) {
			iface_name += ' ';
			iface_name += *(const char *) data;
		}
		send("# +CONNECT" + iface_name + "\r\n");
	}
	if (!cmd && !src && code == CODE_RESET) {
		send("# +RESET\r\n");
	}
	if (!cmd) return;
	if (code == CODE_ACK && test_mode) {
		last_sequence = network.send_command(*cmd);
		--test_mode;
		return;
	}
	gettimeofday(timestamps + 1, NULL);
	if (cmd->op == command::ping && code == CODE_ACK) {
		// just write OK for correct ping replies:
		std::string::size_type len = cmd->dat.size();                     // length of specified ping data
		std::string::size_type max = cmd->limit > len ? cmd->limit : len; // total length of ping payload
		if (payload.size() == max && !memcmp(payload.data(), cmd->dat.data(), len) &&
			// check that data wasn't padded or that pad is actually zero:
			(max == len || (!payload[len] && !memcmp(payload.data() + len, payload.data() + len + 1, max - len - 1)))
		) length = 0;
	}
	if (cmd->op == command::trace && code == CODE_ACK) {
		if (length > 5) {
			int strength = ((int8_t const *) data)[length - 2];
			int quality  = ((int8_t const *) data)[length - 1];
			std::cout << "SIGNAL = " << strength << " dBm";
			if (quality) std::cout << "; SNR = " << quality << " dB";
			std::cout << std::endl;
		}
	} else if (cmd->op == command::app_get && code == CODE_ACK) {
		std::cout << "NODE=" << network.lookup_address(src)->name << " REG=" << to_hex(cmd->reg, 64) << " VAL=" << to_hex(payload) << std::endl;
	}
	// format reply message:
	std::string msg = ":";
	if (code < 0) {
		msg += " ~" + error_name(code);
		length = 0; // ignore payload - not that it should exist...
	} else if (code == CODE_NACK) {
		msg += " -";
		int error = 0;
		if (length && data) {
			error = -((uint8_t const *) data)[0];
			--length;
		}
		if (error) msg += error_name(error);
	} else if (code == CODE_ACK) {
		if (!length) msg += " OK";
	}
	if (length) msg += " " + to_hex(payload, 0);
	uint32_t delay = uint32_t(timestamps[1].tv_sec - timestamps[0].tv_sec) * 1000000 + timestamps[1].tv_usec - timestamps[0].tv_usec;
	msg += " " + to_dec(delay) + "\r\n";
	send(msg);
}

