//==========================================================================
// routing.h
// Example implementation of the routing code
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

#ifndef __ROUTING_H
#define __ROUTING_H

#include "network.h"

#include <vector>
#include <string>
#include <deque>
#include <map>

// generic command format:

struct command
{
	enum opcode {
		nop,     // '~'
		ping,    // '!'
		trace,   // '?'
		config,  // '#'
		net_msg, // '+' ':'
		app_msg, // '$' ':'
		net_set, // '+' '='
		app_set, // '$' '='
		net_get, // '+' '?'
		app_get  // '$' '?'
	} op;
	uint16_t limit;
	uint32_t timeout;
	uint64_t reg;
	std::string dat;
// functions:
	command (opcode = nop);
	command (std::vector < std::string > :: const_iterator b, std::vector < std::string > :: const_iterator e);
	std::string text () const;
};

struct directed_command : command
{
	directed_command ();
	directed_command (command const &cmd, uint16_t dst, router *handler = 0);
	static directed_command msg_send     (uint16_t dst, uint64_t reg, uint8_t const *data = NULL, size_t size = 0);
	static directed_command set_register (uint16_t dst, uint64_t reg, uint8_t const *data = NULL, size_t size = 0, uint32_t timeout = 0);
	static directed_command get_register (uint16_t dst, uint64_t reg, uint8_t const *data = NULL, size_t size = 0, uint32_t timeout = 0, uint16_t limit = 0);
	router *handler;
	uint16_t dst;
	int seq;
};

// network description:

struct media_parameters
{
	bool operator == (std::string const &val) const {return type == val;}
	std::string type;
	double cost;
	double range;
	unsigned time_per_packet;
	unsigned time_per_byte;
};

struct node_description
{
	std::string name;
	uint64_t mac_address;
	uint16_t net_address;
	uint16_t hop_distance;
	uint16_t num_routes;
	uint16_t num_connections;
	enum {max_routes = 16};
	enum {max_connections = 8};
	struct route {
		char device;
		uint8_t mask_length;
		uint16_t first;
		uint16_t last;
	} routes[max_routes];
	struct connection {
		char device;
		uint8_t media_type;
		uint16_t domain;
		double pos_x;
		double pos_y;
	} connections[max_connections];
	std::vector < command > commands;
	node_description const *parent;
	router *handler;
	unsigned time_per_packet;
	unsigned time_per_byte;
};

struct physical_domain
{
	std::string name;
	uint8_t type;
	std::vector < uint64_t > nodes;
};

// network management:

#define CODE_RESET    16
#define CODE_LINKUP   17
#define CODE_LINKDOWN 18

class net_handler
{
public:
	net_handler ();
	int initialize ();
	int start (uint16_t root = 0);
	int stop (uint16_t root = 0);
	int enable_interface (char iface, bool enable);
	int send_command (directed_command const &cmd);
	int send_command (std::string const &cmd);
	node_description const *lookup_name (std::string const &name) const;
	node_description const *lookup_address (uint16_t net_address) const;
public: // temporary hack --- TODO: make private!
	void (*application_callback) (directed_command const *cmd, uint16_t src, int code, void *data, size_t length);
	void (*application_timer) (void (*callback) (), uint32_t microseconds);
	void process_timeout ();
	bool read_media_definition (const char *filename);
	bool read_network_definition (const char *filename);
	bool read_network_config (const char *filename);
	bool save_network_config (const char *filename);
	bool analyse_topology ();
	bool analyse_routing ();
	std::deque < media_parameters > media;
	std::deque < node_description > nodes;
	std::deque < physical_domain > domains;
	std::map < char, bool > interfaces;
	static const uint64_t master_mac;
	static const uint16_t invalid_address;
private:
	router *master;
	std::map < uint64_t, unsigned > mac_map;
	std::map < uint16_t, unsigned > net_map;
	std::map < std::string, uint16_t > addresses;
	std::deque < std::map < unsigned, uint16_t > > domain_map;
	std::deque < directed_command > commands;

	std::deque < directed_command > ofifo;
	enum {
		halted,
		idle,
		delaying,
		awaiting_reply,
		done
	} controller_status;
	enum {
		unconfigured,
		configuring,
		ready,
		busy
	} network_status;
	int age;
//	static const int timeout = 20;
	int process_command (directed_command const &cmd);
	void process_reply (uint16_t src_address, uint16_t sequence, int8_t code);
	int send_config ();

	static void netreply_received (uint16_t src_address, uint16_t sequence, int8_t code, void *data, size_t length);
	static void netreply_timedout ();

	unsigned setup_count, config_count, command_count;
};

extern net_handler network;

#endif
