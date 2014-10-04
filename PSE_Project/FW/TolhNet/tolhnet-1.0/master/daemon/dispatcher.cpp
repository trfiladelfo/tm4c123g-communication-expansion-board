//==========================================================================
// dispatcher.cpp
// Implementation of a TCP+IO+socket dispatcher
// Dispatcher based on giorby's teletext server, (C) 2003-2005
/***************************************************************************
 * Copyright (C) 2003-2014 Giorgio Biagetti                                *
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

#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <sys/un.h>
#include <netinet/in.h>

#include "serial.h"
#include "dispatcher.h"
#include "netshell.h"
#include "applogic.h"
#include "tolhnet.h"
#include "../network/utils.h"
#include "../network/errors.h"
#include "../network/routing.h"

// configuration parameters:

#define SOCK_NAME "/tmp/.tolhnet"
#define INET_PORT 7018

#include "main.h"

controller::controller (int socket) : connection(socket)
{
	last_sequence = -ERROR;
}

controller::~controller ()
{
	if (connection)
		::close(connection);
}

void controller::send (std::string const &msg)
{
	const char *p = msg.data();
	std::string::size_type len = msg.size();

	if (!connection || len > 0xFFFFFF) return; // safety check!
	while (len) {
		int w = ::write(connection, p, len);
		if (w <= 0) {
			// TODO: remove from controller list!
			break;
		} else {
			p += w;
			len -= w;
		}
	}
}


// Implementation of the request dispatcher:

serial_io serial;
connection_dispatcher *connection_dispatcher::dispatcher;

connection_dispatcher::connection_dispatcher ()
{
	if (dispatcher) throw "Only one dispatcher can exist!";

	// File IO:
	file = 0;
	serial.set_device(interfaces[0].name, atoi(interfaces[0].parameters.c_str()));

	// unix stream:
	lst1 = socket(PF_UNIX, SOCK_STREAM, 0);
	if (lst1 < 0) throw "Unable to create unix socket";
	sockaddr_un name1 = {AF_UNIX, SOCK_NAME};
	int e1 = bind(lst1, (sockaddr *) &name1, sizeof name1);
	if (e1) { // check if it was a stale socket
		e1 = connect(lst1, (sockaddr *) &name1, sizeof name1);
		if (!e1) throw "Server already executing";
		unlink(SOCK_NAME); // try to remove socket file
		e1 = bind(lst1, (sockaddr *) &name1, sizeof name1);
		if (e1) throw "Unable to bind unix socket";
	}
	e1 = listen(lst1, 5);
	if (e1) throw "Unable to listen to unix socket";

	// TCP stream:
	lst2 = socket(PF_INET, SOCK_STREAM, 0);
	if (lst2 < 0) throw "Unable to create inet socket";
	int opt = 1;
	if (setsockopt(lst2, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) < 0) throw "Unable to set SO_REUSEADDR option";
	if (setsockopt(lst2, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof opt) < 0) throw "Unable to set SO_KEEPALIVE option";
	sockaddr_in name2 = {AF_INET, htons(INET_PORT), {htonl(INADDR_ANY)}};
	int e2 = bind(lst2, (sockaddr *) &name2, sizeof name2);
	if (e2) throw "Unable to bind inet socket";
	e2 = listen(lst2, 5);
	if (e2) throw "Unable to listen to inet socket";

	// register dispatcher singleton:
	dispatcher = this;
}

connection_dispatcher::~connection_dispatcher ()
{
	if (lst1 > 0) close(lst1);
	if (lst2 > 0) close(lst2);
	unlink(SOCK_NAME);
}

void connection_dispatcher::check_link_connection ()
{
	int x = serial.get_file();
	if (!file && (x || serial.check_status() == serial_io::connected)) {
		// just connected!
		file = serial.get_file();
		// start timer for ENQuiry.
		schedule_event(sci_identify_peer, 100000);
	} else if (file && !x) {
		// just disconnected!
		file = 0;
		network.enable_interface(0, false);
	} else {
		file = x;
	}
}


void connection_dispatcher::run ()
{
// 	// !!!!!!!! for test only:
// 	int pts = posix_openpt(O_RDWR);
// 	std::cout << ptsname(pts) << std::endl;
// 	unlockpt(pts);
// 	ctrls.push_back(new Controller(pts));

	if (!file_commands.empty()) ctrls.push_back(new application_logic);

	struct timeval default_timeout = {5};

	while (true) {
		// TODO: execute check_status only every 1 second or so:
		check_link_connection();
		fd_set fs;
		FD_ZERO(&fs);
		if (file > 0) FD_SET(file, &fs);
		if (lst1 > 0) FD_SET(lst1, &fs);
		if (lst2 > 0) FD_SET(lst2, &fs);
		int max = std::max(std::max(lst1, lst2), file);
		for (unsigned i = 0; i < ctrls.size(); ++i) {
			if (!ctrls[i]->get_socket()) continue;
			FD_SET(ctrls[i]->get_socket(), &fs);
			max = std::max(max, ctrls[i]->get_socket());
		}

		struct timeval timeout = default_timeout;
		while (!events.empty()) {
			struct timeval now;
			gettimeofday(&now, NULL);
			if (timercmp(&events.front().time, &now, >)) {
				timersub(&events.front().time, &now, &timeout);
				break;
			} else {
				callback_t action = events.front().function;
				events.erase(events.begin());
				action();
			}
		}
		select(max + 1, &fs, NULL, NULL, &timeout);
		if (lst1 && FD_ISSET(lst1, &fs))
			ctrls.push_back(new netshell(::accept(lst1, 0, 0)));
		if (lst2 && FD_ISSET(lst2, &fs))
			ctrls.push_back(new netshell(::accept(lst2, 0, 0)));
		for (unsigned i = 0; i < ctrls.size(); ++i) {
			if (!ctrls[i]->get_socket()) continue;
			if (FD_ISSET(ctrls[i]->get_socket(), &fs))
				if (!ctrls[i]->process_input()) {
					delete ctrls[i];
					ctrls.erase(ctrls.begin() + i--);
				}
		}
		if (!file || !FD_ISSET(file, &fs)) continue;
		if (serial.recv()) {
			router *master = network.lookup_address(0)->handler;
			while (!serial.lines.empty()) {
				int result = sci_dispatch_packets(serial.lines.front(), master);
				if (result == CODE_RESET) timerclear(&time_next_event);
				serial.lines.pop();
			}
		}
	}
}


void connection_dispatcher::process_datagram (directed_command const *cmd, uint16_t src, int code, void *data, size_t length)
{
	if (!dispatcher) return;
	std::vector < controller * > &ctrls = dispatcher->ctrls;

	for (unsigned i = 0; i < ctrls.size(); ++i) {
		if (cmd) {
			if (ctrls[i]->last_sequence == cmd->seq)
				ctrls[i]->last_sequence = -ERROR;
			else
				continue;
		}
		ctrls[i]->process_datagram(cmd, src, code, data, length);
	}
}

void connection_dispatcher::schedule_event (callback_t callback, uint32_t microseconds)
{
	// add an event to the event list, keeping it ordered.
	std::vector < event > :: iterator i;
	// compute timeout time:
	struct timeval time, delay = {microseconds / 1000000, microseconds % 1000000};
	gettimeofday(&time, NULL);
	timeradd(&time, &delay, &time);
	// remove previous event if any:
	for (i = dispatcher->events.begin(); i != dispatcher->events.end(); ++i)
		if (i->function == callback) {
			dispatcher->events.erase(i);
			break;
		}
	// add new event if timeout was specified:
	if (!microseconds) return;
	event e = {callback, time};
	for (i = dispatcher->events.begin(); i != dispatcher->events.end(); ++i)
		if (timercmp(&i->time, &time, >)) break;
	dispatcher->events.insert(i, e);
}

