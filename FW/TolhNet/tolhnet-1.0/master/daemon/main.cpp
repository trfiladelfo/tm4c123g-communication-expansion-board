//==========================================================================
// main.cpp
// Implementation of a TCP+IO+socket daemon
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
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <sys/un.h>
#include <netinet/in.h>

#include "serial.h"
#include "tolhnet.h"
#include "dispatcher.h"
#include "netshell.h"
#include "../network/utils.h"
#include "../network/errors.h"
#include "../network/routing.h"

#include "main.h"

#define DEFAULT_CONFIGURATION_FILE "tolhnet.conf"

std::string file_media;
std::string file_topology;
std::string file_addressing;
std::string file_commands;
iface_spec interfaces[8];


bool read_config (const char *filename)
{
	setup_reader r(filename, "configuration file");
	if (!r) return false;
	unsigned n_interfaces = 0;
	while (r.read()) {
		if (r.fields.size() < 1) return false;
		std::string::size_type paren_pos = r.fields[0].find('(');
		std::string target = r.fields[0].substr(0, paren_pos);
		std::string option;
		if (paren_pos != std::string::npos && r.fields[0][r.fields[0].size() - 1] == ')') {
			option = r.fields[0].substr(paren_pos + 1, r.fields[0].size() - paren_pos - 2);
		}
		if (target == "TOLHNET" && option.empty()) {
			if (r.fields.size() != 4) return false;
			file_media = r.fields[1];
			file_topology = r.fields[2];
			file_addressing = r.fields[3];
		}
		if (target == "TOLHNET" && option.size() == 1) {
			if (n_interfaces >= 8) return false;
			if (r.fields.size() < 2) return false;
			std::string::size_type colon = r.fields[1].find(':');
			if (!colon || colon == std::string::npos) return false;
			interfaces[n_interfaces].interface = option[0];
			interfaces[n_interfaces].type = r.fields[1].substr(0, colon);
			interfaces[n_interfaces].name = r.fields[1].substr(colon + 1);
			if (r.fields.size() > 2)
				interfaces[n_interfaces].parameters = r.fields[2];
			++n_interfaces;
		}
		if (target == "PROGRAM") {
			if (r.fields.size() != 2) return false;
			file_commands = r.fields[1];
		}
		if (target == "NETSHELL") {
			if (r.fields.size() != 2) return false;
		}
	}
	if (n_interfaces != 1) return false;
	return true;
}

int main (int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);

	bool compute_routing = false;
	if (argc > 1 && argv[1] == std::string("--route")) {
		// compute routing tables instead of starting daemon
		compute_routing = true;
		++argv;
		--argc;
	}

	if (!read_config(argc < 2 ? DEFAULT_CONFIGURATION_FILE : argv[1])) {
		std::cerr << "ERROR: unable to read main configuration file!" << std::endl;
		return -1;
	}

	network.initialize();

	if (!network.read_media_definition(file_media.c_str())) {
		std::cerr << "ERROR: unable to read media definition file!" << std::endl;
		return -1;
	}
	if (!network.read_network_definition(file_topology.c_str())) {
		std::cerr << "ERROR: unable to read network definition file!" << std::endl;
		return -1;
	}
	if (compute_routing) {
		if (!network.analyse_topology()) {
			std::cerr << "ERROR: problems in the network topology!" << std::endl;
			return -1;
		}
		network.save_network_config(file_addressing.c_str());
		return 0;
	}

	if (!network.read_network_config(file_addressing.c_str())) {
		std::cerr << "ERROR: unable to read network configuration file!" << std::endl;
		return -1;
	}
	if (!network.analyse_routing()) {
		std::cerr << "ERROR: problems in the routing tree!" << std::endl;
		return -1;
	}

	network.application_callback = connection_dispatcher::process_datagram;
	network.application_timer    = connection_dispatcher::schedule_event;
	network.lookup_address(0)->handler->nwk_connect_device(interfaces[0].interface, sci_send_packet);

	try {
		connection_dispatcher *dispatcher = new connection_dispatcher();
		dispatcher->run();
		delete dispatcher;
	} catch (const char *message) {
		std::cerr << "ERROR: " << message << std::endl;
	}
	return 0;
}
