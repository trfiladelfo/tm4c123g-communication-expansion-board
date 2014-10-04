//==========================================================================
// routing.cpp
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

#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "routing.h"
#include "errors.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>

command::command (opcode x)
{
	op = x;
	limit = 0;
	timeout = 0;
	reg = 0;
}

command::command (std::vector < std::string > :: const_iterator b, std::vector < std::string > :: const_iterator e)
{
	op = nop;
	limit = 0;
	timeout = 0;
	reg = 0;
	if (b == e || b->size() != 1) throw;
	char cmd = (*b)[0];
	enum {sys, net, app, nowhere} dst = cmd == '~' || cmd == '!' || cmd == '?' ? sys : cmd == '+' ? net : cmd == '$' ? app : nowhere;
	if (dst == nowhere) throw;
	enum {msg, set, get, empty} type = empty;
	if (dst == sys) {
		// check for pings & traces:
		if (cmd == '!') type = set;
		if (cmd == '?') type = get;
	} else {
		// check for a set/get/msg:
		if (b + 1 != e && b + 2 != e) {
			std::string const &maybe_op = *(b + 2);
			if (maybe_op == "=") type = set;
			if (maybe_op == "?") type = get;
			if (maybe_op == ":") type = msg;
			if (type) {
				reg = from_hex((++b)->c_str(), 64);
				++b;
			};
		}
	}
	if (type == empty) throw;
	while (++b != e) {
		if (b->empty()) continue;
		char param = (*b)[0];
		if (param == '"') {
			// insert payload string:
			dat.append(b->begin() + 1, b->end() - (b->size() > 1 && *(b->rbegin()) == '"'));
		} else if (param == '{') { // response limit specification
			limit = from_hex(b->c_str() + 1, 16);
		} else if (param == '[') { // timeout specification
			timeout = from_hex(b->c_str() + 1, 32);
		} else if (param == '(') { // delayed register substitution
			// TODO: implement me!
		} else if (param == '$') { // "length of string" placeholder
			if (b->size() > 1 && b + 1 != e && !(b + 1)->empty()) {
				uint64_t len = (b + 1)->size();
				if ((*(b + 1))[0] == '"')
					len -= 1 + ((b + 1)->size() > 1 && *((b + 1)->rbegin()) == '"');
				else
					len /= 2; // assume it's an hex field
				if ((*b)[1] == ':') // variable length integer
					dat += to_bin(len);
				else if ((*b)[1] == '#') // fixed length integer
					dat += to_bin(len, 8 * (b->size() - 1));
			}
		} else if (param == '?') { // GET after SET specification
			// TODO: implement me!
		} else if (param == '>') { // store result into register
			break;
			// TODO: implement me!
		} else {                   // just hex data to send
			for (std::string::size_type i = 0; i + 1 < b->size(); i += 2)
				dat += (char) from_hex(b->c_str() + i, 8);
		}
	}
	opcode opcodes[3][3] = {{nop, ping, trace}, {net_msg, net_set, net_get}, {app_msg, app_set, app_get}};
	op = opcodes[dst][type];
//	printf("CMD: %s\n", text().c_str());
}

std::string command::text () const
{
	std::string x;
	x += op["~!?#+$+$+$"];
	if (op >= 4) x += ' ' + to_hex(reg, 64) + ' ' + ":=?"[op / 2 - 2];
	for (std::string::size_type i = 0; i < dat.size(); ++i)
		x += ' ' + to_hex(dat[i], 8);
	if (timeout) x += " [" + to_hex(timeout, 32) + "]";
	if (limit) x += " {" + to_hex(limit, 16) + "}";
	return x;
}

directed_command::directed_command ()
{
	handler = 0;
	dst = 0;
	seq = -ERROR;
}

directed_command::directed_command (command const &cmd, uint16_t dst, router *handler) :
	command(cmd)
{
	this->handler = handler;
	this->dst = dst;
	this->seq = -ERROR;
}

directed_command directed_command::set_register (uint16_t dst, uint64_t reg, uint8_t const *data, size_t size, uint32_t timeout)
{
	directed_command cmd;
	cmd.op = command::app_set;
	cmd.limit = 0;
	cmd.timeout = timeout;
	cmd.reg = reg;
	cmd.dat = std::string((const char *) data, size);
	cmd.handler = 0;
	cmd.dst = dst;
	cmd.seq = -ERROR;
	return cmd;
}

directed_command directed_command::get_register (uint16_t dst, uint64_t reg, uint8_t const *data, size_t size, uint32_t timeout, uint16_t limit)
{
	directed_command cmd;
	cmd.op = command::app_get;
	cmd.limit = limit;
	cmd.timeout = timeout;
	cmd.reg = reg;
	cmd.dat = std::string((const char *) data, size);
	cmd.handler = 0;
	cmd.dst = dst;
	cmd.seq = -ERROR;
	return cmd;
}

directed_command directed_command::msg_send (uint16_t dst, uint64_t reg, uint8_t const *data, size_t size)
{
	directed_command cmd;
	cmd.op = command::app_msg;
	cmd.limit = 0;
	cmd.timeout = 0;
	cmd.reg = reg;
	cmd.dat = std::string((const char *) data, size);
	cmd.handler = 0;
	cmd.dst = dst;
	cmd.seq = -ERROR;
	return cmd;
}


net_handler network;

const uint64_t net_handler::master_mac = UINT64_C(0xFFFFFFFFFFFF);
const uint16_t net_handler::invalid_address = UINT16_C(0xFFFF);

net_handler::net_handler ()
{
	master = NULL;
	application_callback = NULL;
	application_timer = NULL;
	controller_status = halted;
	network_status = unconfigured;
}

bool net_handler::read_media_definition (const char *filename)
{
	setup_reader m(filename, "media description file");
	if (!m) return false;
	while (m.read()) {
		if (m.fields.size() != 5) break;
		media_parameters medium = {
			m.fields[0],               // name
			atof(m.fields[1].c_str()), // cost
			atof(m.fields[2].c_str()), // range
			(unsigned) atoi(m.fields[3].c_str()), // time per packet
			(unsigned) atoi(m.fields[4].c_str()), // time per byte
		};
		media.push_back(medium);
	}
	return true;
}

bool net_handler::read_network_definition (const char *filename)
{
	setup_reader dev(filename, "topology file");
	if (!dev) return false;
	domain_map.resize(media.size());
	while (dev.read()) {
		if (dev.fields.size() < 3) break;
		uint64_t mac = dev.fields[1] == "MASTER-CONTROLLER" ? master_mac : convert_mac_address(dev.fields[1]);
		node_description n = {dev.fields[0], mac, (uint16_t) (mac == master_mac ? 0 : invalid_address)};
		for (unsigned i = 2; i < dev.fields.size(); ++i) {
			if (n.num_connections >= node_description::max_connections) return false;
			node_description::connection &c = n.connections[n.num_connections];
			// connection syntax must be "X=YYn(...)", where X is the interface name, YY the media type, n the optional subdomain, (...) optional in-domain location
			std::string domain = dev.fields[i];
			// extract output device name:
			std::string::size_type equal_sign = domain.find('=');
			if (equal_sign == 1) {
				c.device = domain[0];
				domain.erase(0, 2);
			} else {
				// conventional name of master's only interface:
				c.device = 'X';
			}
			// extract media type name:
			std::string::size_type digit_pos = domain.find_first_of("0123456789");
			std::string::size_type paren_pos = domain.find('(');
			if (paren_pos < digit_pos) digit_pos = std::string::npos;
			std::deque < media_parameters > :: const_iterator type = std::find(media.begin(), media.end(), domain.substr(0, std::min(digit_pos, paren_pos)));
			if (type == media.end()) return false;
			c.media_type = type - media.begin();
			// extract subdomain number, and make a unique domain number across all media types:
			unsigned subdomain = digit_pos == std::string::npos ? 0 : atoi(domain.c_str() + digit_pos);
			std::map < unsigned, uint16_t > &map = domain_map[c.media_type];
			std::map < unsigned, uint16_t > :: const_iterator d = map.find(subdomain);
			if (d != map.end()) {
				c.domain = d->second;
			} else {
				c.domain = map[subdomain] = domains.size();
				physical_domain pd = {domain.substr(0, paren_pos), c.media_type};
				domains.push_back(pd);
			}
			domains[c.domain].nodes.push_back(mac);
			// extract in-domain position:
			if (paren_pos != std::string::npos) {
				c.pos_x = atof(domain.c_str() + paren_pos + 1);
				paren_pos = domain.find(',', paren_pos);
			}
			if (paren_pos != std::string::npos) {
				c.pos_y = atof(domain.c_str() + paren_pos + 1);
			}
			++n.num_connections;
		}
		std::map < uint64_t, unsigned > :: const_iterator m = mac_map.find(n.mac_address);
		if (m == mac_map.end()) {
			mac_map[n.mac_address] = nodes.size();
			if (n.mac_address == master_mac) net_map[n.net_address] = nodes.size();
			nodes.push_back(n);
		} else {
			// preserve handler but update everything else:
			n.handler = nodes[m->second].handler;
			nodes[m->second] = n;
		}
	}
	return true;
}

bool net_handler::read_network_config (const char *filename)
{
	if (!nodes.size() || nodes[0].mac_address != master_mac) return false;
	setup_reader r(filename, "address file");
	if (!r) return false;
	node_description *node = 0;
	unsigned counter = 0;
	while (r.read()) {
		if (
			// check for a set_address directive:
			r.fields.size() == 3 &&
			r.fields[0].size() == 17 && r.fields[0][12] == '@' &&
			r.fields[1].size() == 4
		) {
			++counter;
			node_description n = {
				r.fields[2],                                       // node name
				(uint64_t) from_hex(r.fields[0].c_str()     , 48), // mac_address
				(uint16_t) from_hex(r.fields[1].c_str()     , 16), // net_address
				(uint16_t) from_hex(r.fields[0].c_str() + 13, 16), // hop_distance
			};
			std::map < uint64_t, unsigned > :: iterator i = mac_map.find(n.mac_address);
			if (i == mac_map.end()) {
				// node hasn't been created yet, create it:
				mac_map[n.mac_address] = nodes.size();
				net_map[n.net_address] = nodes.size();
				addresses[n.name] = n.net_address;
				nodes.push_back(n);
				node = &nodes.back();
			} else {
				// node already exists, reorder array and update maps:
				mac_map[nodes[counter].mac_address] = i->second;
				net_map[nodes[counter].net_address] = i->second;
				std::swap(nodes[counter], nodes[i->second]);
				i->second = counter;
				net_map[n.net_address] = counter;
				addresses[n.name] = n.net_address;
				node = &nodes[counter];
				node->name = n.name;
				node->net_address = n.net_address;
				node->hop_distance = n.hop_distance;
			}
		} else if (
			// check for a set_route directive
			r.fields.size() == 3 &&
			r.fields[0].empty() && r.fields[1].size() >= 4 && r.fields[2].size() == 1
		) {
			if (!node || node->num_routes == node_description::max_routes) return false;
			if (r.fields[2].size() != 1) return false;
			char device = r.fields[2][0];
			if (device == '0') device = 0;
			if (r.fields[1].size() == 4) {
				// single device route
				node_description::route &route = node->routes[node->num_routes++];
				route.device = device;
				route.mask_length = 16;
				route.first = from_hex(r.fields[1].c_str(), 16);
				route.last = route.first;
			} else if (r.fields[1].size() == 6 && r.fields[1][4] == '/') {
				// netmask route
				node_description::route &route = node->routes[node->num_routes++];
				route.device = device;
				route.mask_length = from_hex(r.fields[1].c_str() + 5, 4);
				route.first = from_hex(r.fields[1].c_str(), 16);
				route.last = route.first + ((1 << (16 - route.mask_length)) - 1);
			} else if (r.fields[1].size() == 9 && r.fields[1][4] == '-') {
				// range route
				node_description::route &route = node->routes[node->num_routes++];
				route.device = device;
				route.mask_length = 0xFF;
				route.first = from_hex(r.fields[1].c_str() + 0, 16);
				route.last  = from_hex(r.fields[1].c_str() + 5, 16);
			} else return false;
		} else if (
			// check for an embedded send_command directive:
			r.fields.size() > 1 && r.fields[0].empty() && r.fields[1].size() == 1
		) {
			if (!node) return false;
			command c(r.fields.begin() + 1, r.fields.end());
			node->commands.push_back(c);
		} else if (!r.fields.empty()) {
			// assume it's a generic send_command directive:
			node_description const *src_node = 0;
			unsigned start = 0;
			std::string::size_type spec = r.fields[start].find('>');
			if (spec != std::string::npos) {
				// source node specified:
				src_node = lookup_name(r.fields[start].substr(0, spec));
				++start;
			} else {
				// otherwise send from master:
				src_node = lookup_address(0);
			}
			if (!src_node) return false;
			if (r.fields.size() < start) return false;
			if (r.fields[start].empty()) return false;
			if (r.fields[start].size() == 1 || (r.fields[start].size() > 1 && r.fields[start][1] != '*')) {
				// lookup destination node:
				uint16_t dst = 0;
				if (r.fields[start].size() > 1) {
					node_description const *dst_node = lookup_name(r.fields[start].substr(1));
					if (!dst_node) return false;
					dst = dst_node->net_address;
					r.fields[start].erase(1);
				}
				directed_command c(command(r.fields.begin() + start, r.fields.end()), dst, src_node->handler);
				commands.push_back(c);
			} else if (r.fields[start].size() > 1 && r.fields[start][1] == '*') {
				// broadcast command to all nodes:
				r.fields[start].erase(1);
				for (std::deque < node_description > :: const_iterator i = nodes.begin(); i != nodes.end(); ++i) {
					directed_command c(command(r.fields.begin() + start, r.fields.end()), i->net_address, src_node->handler);
					commands.push_back(c);
				}
			}
		}
	}
	// add default route to master node if not specified in configuration file:
	std::map < uint64_t, unsigned > :: const_iterator i = mac_map.find(master_mac);
	if (i != mac_map.end() && !(node = &nodes[i->second])->num_routes) {
		node_description::route &route = node->routes[node->num_routes++];
		route.device = node->connections[0].device;
		route.mask_length = 0;
		route.first = 0;
		route.last  = 65535;
	}
	return true;
}

#include <fstream>

bool net_handler::save_network_config (const char *filename)
{
	std::ofstream f(filename);
	for (std::deque < node_description > :: const_iterator i = nodes.begin(); i != nodes.end(); ++i) {
		if (i->mac_address == master_mac) continue;
		f << to_hex(i->mac_address, 48) << '@' << to_hex(i->hop_distance, 16) << '\t' << to_hex(i->net_address, 16) << '\t' << i->name << '\n';
		for (unsigned r = 0; r < i->num_routes; ++r) {
			f << '\t' << to_hex(i->routes[r].first, 16);
			if (i->routes[r].mask_length == 0xFF)
				// span-based rule:
				f << '-' << to_hex(i->routes[r].last, 16);
			else if (i->routes[r].mask_length < 16)
				// mask-based rule:
				f << '/' << to_hex(i->routes[r].mask_length, 4);
			f << '\t' << i->routes[r].device << '\n';
		}
	}
	return true;
}

bool net_handler::analyse_routing ()
{
	node_description *master = NULL;
	for (std::deque < node_description > :: iterator i = nodes.begin(); i != nodes.end(); ++i) {
		node_description *node = &*i;
		if (node->net_address == invalid_address) continue;
// 		printf("checking '%s'", node->name.c_str());
// 		for (unsigned l = 0; l < node->num_connections; ++l)
// 			printf(" %c=%s[%d](%f,%f)", node->connections[l].device, domains[node->connections[l].domain].name.c_str(), node->connections[l].domain, node->connections[l].pos_x, node->connections[l].pos_y);
// 		printf("\n");
		if (!node->net_address) {
			if (!master) {
				master = node;
				continue;
			}
			printf("ERROR: multiple masters found!\n");
			return false;
		}
		// try to determine parent node: // TODO: optimize search! TODO: correct interpretation of routing tables!
		for (std::deque < node_description > :: const_iterator j = nodes.begin(); j != nodes.end(); ++j) {
			if (node->hop_distance ? !j->net_address || j->hop_distance != node->hop_distance - 1 : j->net_address) continue;
			for (unsigned k = 0; k < j->num_routes; ++k) {
				if (j->routes[k].first <= node->net_address && j->routes[k].last >= node->net_address) {
					if (!node->parent) {
						node->parent = &*j;
						// identify parent connection:
						node_description::connection parent_connection = {'\0'};
						for (unsigned l = 0; l < j->num_connections; ++l)
							if (j->connections[l].device == j->routes[k].device)
								parent_connection = j->connections[l];
						if (!parent_connection.device) {
							printf("WARNING: parent '%s' of node '%s' is not connected to it!\n", j->name.c_str(), node->name.c_str());
						}
						node_description::connection child_connection = {'\0'};
						for (unsigned l = 0; l < i->num_connections; ++l)
							if (i->connections[l].domain == parent_connection.domain)
								child_connection = i->connections[l];
						if (!child_connection.device) {
							printf("WARNING: node '%s' is not connected to its parent!\n", node->name.c_str());
						}
						if (parent_connection.device && child_connection.device) {
							// compute distance, cost, delay:
							media_parameters const &medium = media[parent_connection.media_type];
							i->time_per_packet = j->time_per_packet + medium.time_per_packet;
							i->time_per_byte   = j->time_per_byte   + medium.time_per_byte;
// 							printf("'%s' delay: %u,%u\n", node->name.c_str(), node->time_per_packet, node->time_per_byte);
						}
					} else {
						printf("ERROR: node '%s' is reachable from both '%s' and '%s'!\n", node->name.c_str(), node->parent->name.c_str(), j->name.c_str());
						return false;
					}
				}
			}
		}
		if (!node->parent) {
			printf("ERROR: node '%s' is unreachable!\n", node->name.c_str());
			return false;
		}
	}
	return true;
}

node_description const *net_handler::lookup_name (std::string const &name) const
{
	std::map < std::string, uint16_t > :: const_iterator i = addresses.find(name);
	if (i == addresses.end()) return 0;
	std::map < uint16_t, unsigned > :: const_iterator j = net_map.find(i->second);
	if (j == net_map.end()) return 0;
	return &nodes[j->second];
}

node_description const *net_handler::lookup_address (uint16_t net_address) const
{
	std::map < uint16_t, unsigned > :: const_iterator j = net_map.find(net_address);
	if (j == net_map.end()) return 0;
	return &nodes[j->second];
}

int net_handler::initialize ()
{
	node_description const *node = lookup_address(0);
	if (!node) {
		// create the master controller node if it does not already exist:
		node_description n = {"", master_mac};
		n.handler = new router(n.mac_address);
		mac_map[n.mac_address] = nodes.size();
		net_map[n.net_address] = nodes.size();
		addresses[n.name] = n.net_address;
		// add defaults connection using first media type:
		n.connections[0].device = 'X';
		n.num_connections = 1;
		// push node description:
		nodes.push_back(n);
		node = &nodes.back();
	}
	master = node->handler;
	if (!master) return -ENONET;

	interfaces['\0'] = false;

	// local setup of the master node (host) simulator:
	master->fib_flush();
	master->fib_add(0x0000, 16, '\0', 0);
	master->fib_add(0x0000,  0,  'X', 0);
	master->options.enable_config = 0;
	master->options.enable_forward = 0;
	master->datagram_received = netreply_received;
	master->netreply_received = netreply_received;
	master->netevent_received = NULL;

	return 0;
}

int net_handler::start (uint16_t root)
{
	// should start the network subtree starting at logical address "root"
	// this is not yet implemented...
	if (root) return -ENOSYS;

	// the master node must be accessible:
	if (!master) return -ENODEV;

	// reset previous state:
	// TODO: notify application of canceled events?
	controller_status = halted;
	ofifo.clear();

	// start configuration sequence:
	network_status = configuring;
	setup_count = 0;
	config_count = 0;
	command_count = 0;
	// simulate an ACK to start the configuration state machine:
	// TODO: replace with a timed event!
	netreply_received(0, 0, CODE_ACK, NULL, 0);

	// return the number of nodes to be brought up:
	return nodes.size() - 1;
}

int net_handler::stop (uint16_t root)
{
	// TODO: implement me!
	if (root) return -ENOSYS;
	return -ENOSYS;
}

int net_handler::enable_interface (char iface, bool enable)
{
	std::map < char, bool > :: iterator i = interfaces.find(iface);
	if (i == interfaces.end()) return -ENODEV;
	i->second = enable;
	if (network.application_callback)
		network.application_callback(NULL, 0, enable ? CODE_LINKUP : CODE_LINKDOWN, &iface, sizeof iface);
	return 0;
}

///////////
// the following two functions are borrowed from the EMG software, check if they are usuful or remove!

void net_handler::process_reply (uint16_t src_address, uint16_t sequence, int8_t code)
{
	if (ofifo.front().dst == src_address && ofifo.front().seq == sequence) {
		// TODO: mark node as reachable?
		if (controller_status == awaiting_reply /* && ofifo.advance_mask(src_address) */ ) {
			// all replies received.
			ofifo.pop_front();
			controller_status = done;
		}
	}
}

void net_handler::process_timeout ()
{
	switch (controller_status) {

	case halted:
		break;

	case done:
		controller_status = idle;
		if (!ofifo.size()) {
			// TODO: shutdown transmitter to conserve power?
			break;
		}

	case idle:
		if (ofifo.size()) {
			directed_command const &cmd = ofifo.front();
			if (cmd.op == command::nop) {
				// no-operation command, used to make pauses: the parameter is the delay.
				// This is handled by the command processor itself.
				if ((age = cmd.timeout)) {
					controller_status = delaying;
				} else {
					// zero-time wait: just remove the command from the queue...
					ofifo.pop_front();
				}
			} else {
				// it means that the command is for one or more nodes.
				age = 0;
				controller_status = awaiting_reply;
				// encode the command:
				if (controller_status != halted) {
					send_command(cmd);
				}
			}
		}
		break;

	case delaying:
		if (!--age) {
			ofifo.pop_front();
			controller_status = idle;
		}
		break;

	case awaiting_reply:
		// TODO: a timeout occurred - let's see who didn't respond!
		ofifo.pop_front();
		controller_status = done;
		break;

	}

}


/////////////

int net_handler::send_config ()
{
	// send next setup packet if any:
	int status = 0;
	while (config_count < nodes.size()) {
		node_description const &n = nodes[config_count];
		if (!n.net_address || n.net_address == invalid_address) {
			++config_count;
			continue;
		}
		if (command_count) {
			if (command_count <= n.commands.size()) {
				directed_command cmd(n.commands[command_count++ - 1], n.net_address, master);
				printf("injecting set-up command #%u for node %04X: %s\n", ++setup_count, cmd.dst, cmd.text().c_str());
				status = process_command(cmd);
				if (status < 0) return status;
				return 1;
			} else {
				++config_count;
				command_count = 0;
				continue;
			}
		} else {
			++command_count;
		}
		printf("injecting config command #%u for node %04X (%s).\n", ++setup_count, n.net_address, n.name.c_str());
		status = master->nwk_config_prepare(n.net_address, n.mac_address, n.hop_distance);
		if (status) return status;
		for (unsigned i = 0; i < n.num_routes; ++i) {
			if (n.routes[i].mask_length == 0xFF) {
				status = master->nwk_config_add_span(n.routes[i].first, n.routes[i].last, n.routes[i].device);
			} else {
				status = master->nwk_config_add_mask(n.routes[i].first, n.routes[i].mask_length, n.routes[i].device);
			}
			if (status) return status;
		}
		directed_command cmd(command::config, n.net_address, master);
		status = process_command(cmd);
		if (status < 0) return status;
		return 1;
	}
	if (command_count < commands.size()) {
		directed_command const &cmd = commands[command_count++];
		printf("injecting set-up command #%u for node %04X: %s\n", ++setup_count, cmd.dst, cmd.text().c_str());
		status = process_command(cmd);
		if (status < 0) return status;
		return 1;
	} else {
		printf("network configuration complete!\n");
		return 0;
	}
}

int net_handler::send_command (directed_command const &cmd)
{
	if (!master) return -ENONET;
	if (network_status < ready) return -ENONET;
	if (network_status > ready) return -EBUSY;
	return process_command(cmd);
}

int net_handler::process_command (directed_command const &cmd)
{
	static const uint8_t cmd_codes[] = {CODE_ACK, CODE_PING, CODE_TRACE, CODE_CONFIG, CODE_MSG, CODE_MSG, CODE_SET, CODE_SET, CODE_GET, CODE_GET};
	uint8_t code = cmd_codes[cmd.op];
	router *sender = cmd.handler ? cmd.handler : master;
	node_description const *node = lookup_address(cmd.dst);
	if (!node) return -ENODEV;

	std::string data;
	int seq = -ERROR;

	switch (cmd.op) {
	case command::ping:
		data = cmd.dat;
		if (cmd.limit > data.size()) // pad data
			data.append(cmd.limit - data.size(), '\0');
		seq = sender->send_ping(cmd.dst, data.data(), data.size());
		break;
	case command::trace:
		{
			uint16_t const max_traceble_hops = 24;
			uint16_t const span_limit = (cmd.limit && cmd.limit < max_traceble_hops ? cmd.limit : max_traceble_hops) - 1;
			uint16_t first = 0;
			uint16_t last = node->hop_distance;
			if (last - first > span_limit)
				first = last - span_limit;
			seq = sender->send_trace(cmd.dst, first, last - first);
			break;
		}
	case command::config:
		// special handling of configuration commands!
		// command must have already been prepared by the appropriate network-layer functions.
		seq = sender->nwk_send_buffer();
		break;
	case command::net_set:
	case command::net_get:
	case command::net_msg:
		data = to_bin(cmd.reg);
		data += cmd.dat;
		seq = sender->send_nwk(cmd.dst, data.data(), data.size(), code);
		break;
	case command::app_set:
	case command::app_get:
	case command::app_msg:
		data = to_bin(cmd.reg);
		if (cmd.timeout || cmd.limit) {
			data += to_bin(log_encode(cmd.timeout), 8);
			++code;
		}
		if (cmd.limit) {
			data += to_bin(cmd.limit, 8);
			code += 2;
		}
		data += cmd.dat;
		seq = sender->send_datagram(cmd.dst, data.data(), data.size(), code);
		break;
	case command::nop:
		return -ENOSYS;
	}
	if (seq >= 0) {
		ofifo.push_back(cmd);
		ofifo.back().seq = seq;
		age = 0;
		if (network_status == ready)
			network_status = busy;
		if (application_timer && node->time_per_packet) {
			if (!cmd.timeout)
				ofifo.back().timeout = 2 * (node->time_per_packet + sender->nwk_packet_size() * node->time_per_byte);
			//printf("timeout in %u µs\n", ofifo.back().timeout);
			application_timer(netreply_timedout, ofifo.back().timeout);
		}
	}
	return seq;
}

int net_handler::send_command (std::string const &cmd)
{
	std::vector <std::string> fields = split_fields(cmd);
	if (!fields.size() || fields[0].size() <= 1) return -EINVAL;
	// lookup destination node:
	uint16_t dst = 0;
	node_description const *dst_node = lookup_name(fields[0].substr(1));
	if (!dst_node) return -ENODEV;
	dst = dst_node->net_address;
	fields[0].erase(1);
	directed_command c(command(fields.begin(), fields.end()), dst);
	return send_command(c);
}

void net_handler::netreply_received (uint16_t src_address, uint16_t sequence, int8_t code, void *data, size_t length)
{
	if (network.network_status == configuring) {
		int status = -ERROR;
		if (code == CODE_TRACE) code = CODE_ACK;
		if (!network.ofifo.empty()) {
			directed_command const &cmd = network.ofifo.front();
			if (src_address != cmd.dst || sequence != cmd.seq || code > CODE_NACK) return; // discard extraneous packet.
			network.ofifo.pop_front();
			if (network.application_timer)
				network.application_timer(netreply_timedout, 0);
		} else {
			if (src_address || data) return;
		}
		if (code == CODE_ACK) {
			status = network.send_config();
			if (status == 0) {
				network.network_status = ready;
			}
		}
		if (status < 0) {
			// configuration refused --- abort!
			printf("configuration aborted.\n");
			network.network_status = unconfigured;
			return;
		}
	} else if (network.network_status >= ready) {
		network.network_status = ready;
		if (code == CODE_TRACE) code = CODE_ACK; // TODO: format reply in a better way?
		if (code <= CODE_NACK) {
			// this is a reply for the application layer:
			if (network.ofifo.empty()) {
				printf("Unexpected reply received!\n");
				return;
			}
			directed_command const &cmd = network.ofifo.front();
			if (sequence != cmd.seq || src_address != cmd.dst) {
				printf("Some other reply received!\n");
				return;
			}
			if (network.application_timer)
				network.application_timer(netreply_timedout, 0);
			if (network.application_callback)
				network.application_callback(&cmd, src_address, code, data, length);
			network.ofifo.pop_front();
		} else {
			// this is a spontaneous message originating from the network:
			if (network.application_callback)
				network.application_callback(NULL, src_address, code, data, length);
		}
	}
}

void net_handler::netreply_timedout ()
{
	if (network.ofifo.empty()) {
		printf("Unexpected timeout received!\n");
		return;
	}
	directed_command const &cmd = network.ofifo.front();
	if (network.age < 4) {
		++network.age;
		printf("Retransmission #%d.\n", network.age);
		router *sender = cmd.handler ? cmd.handler : network.master;
		sender->nwk_send_buffer();
		if (network.application_timer)
			network.application_timer(netreply_timedout, network.ofifo.front().timeout);
	} else {
		netreply_received(cmd.dst, cmd.seq, -ETIMEOUT, NULL, 0);
	}
}
