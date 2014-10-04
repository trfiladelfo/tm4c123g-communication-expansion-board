//==========================================================================
// applogic.cpp
// ToLHnet simple application
/***************************************************************************
 * Copyright (C) 2014 Giorgio Biagetti                                     *
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

#include "applogic.h"
#include "../network/routing.h"
#include "../network/errors.h"
#include "../network/utils.h"

#include <iostream>

extern std::string file_commands;

application_logic::application_logic () : controller(0)
{
	setup_reader r(file_commands.c_str(), "command file");
	if (!r) return;
	std::string target;
	while (r.read()) {
		if (r.fields.size() && !r.fields[0].empty()) target = r.fields[0];
		if (r.fields.size() > 1) commands[target].push_back(join_fields(r.fields.begin() + 1, r.fields.end()));
	}
	pointer = commands.end();
	counter = 0;
}

bool application_logic::process_input ()
{
	return false;
}

void application_logic::process_datagram (directed_command const *cmd, uint16_t src, int code, void *data, size_t length)
{
	std::string payload;
	if (data) payload = std::string((const char *) data, length);

	if (!cmd && !src && code == CODE_RESET) network.start();

	if (!cmd && code == CODE_MSG && length > 1 && payload[0] == 1) {
		node_description const *node = network.lookup_address(src);
		uint8_t button = payload[1];
		std::string target = to_hex(button, 8) + "@" + node->name;
		std::cout << "BUTTON " << target << " PRESSED!" << std::endl;
		pointer = commands.find(target);
		if (pointer != commands.end())
			last_sequence = network.send_command(pointer->second[counter = 0]);
	}

	if (cmd && pointer != commands.end()) {
		if (++counter < pointer->second.size())
			last_sequence = network.send_command(pointer->second[counter]);
		else
			pointer = commands.end();
	}
}

