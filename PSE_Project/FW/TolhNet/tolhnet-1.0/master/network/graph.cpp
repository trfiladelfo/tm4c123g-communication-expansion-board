//==========================================================================
// graph.cpp
// Example implementation of the routing discovery algorithm
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


#include <boost/config.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"

#include "routing.h"

static inline double sqr (double x)
{
	return x * x;
}

typedef boost::adjacency_list_traits <
	boost::setS,        // outbound edge container selector
	boost::vecS,        // vertex container selector
	boost::undirectedS  // graph structure selector
> graph_traits_t;

typedef graph_traits_t::vertex_descriptor vertex_descriptor;
typedef graph_traits_t::edge_descriptor edge_descriptor;

struct vertex_property
{
	std::vector < std::pair < vertex_descriptor, char > > children;
	unsigned child_count;
	uint16_t address;
	uint16_t depth;
	uint16_t span_start;
	char span_device;
};

struct edge_property
{
	double cost;
	uint8_t type;
	uint16_t domain;
	char device1;
	char device2;
	bool used;
};

typedef boost::adjacency_list <
	boost::setS,        // outbound edge container selector
	boost::vecS,        // vertex container selector
	boost::undirectedS, // graph structure selector
	vertex_property,    // vertex property type
	edge_property       // edge property type
> graph_t;


static void plot_graph (graph_t const &g, const char *filename, bool print_addresses)
{
	std::ofstream file(filename);

	file << std::fixed << std::setprecision(2);

	file << "graph D {\n"
		<< "  rankdir=LR\n"
		<< "  size=\"4,3\"\n"
		<< "  ratio=\"fill\"\n"
		<< "  edge[style=\"bold\"]\n" << "  node[shape=\"circle\"]\n";

	boost::graph_traits < graph_t > :: vertex_iterator vi, vi_end;
	if (print_addresses)
		for (boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; ++vi)
			file << network.nodes[*vi].name << " ["
				<< " label=\"\\N\\n" << g[*vi].address << '@' << g[*vi].depth << "\""
				<< " pos=\"" << network.nodes[*vi].connections[0].pos_x << ',' << network.nodes[*vi].connections[0].pos_y << "!\""
				<< " ];\n";
	boost::graph_traits < graph_t > :: edge_iterator ei, ei_end;
	for (boost::tie(ei, ei_end) = boost::edges(g); ei != ei_end; ++ei) {
		edge_descriptor e = *ei;
		vertex_descriptor v1 = boost::source(e, g), v2 = boost::target(e, g);
		file << network.nodes[v1].name << " -- " << network.nodes[v2].name << " [ ";
		file << "label=\"" << network.domains[g[e].domain].name << ":" << g[e].cost << "\"";
		file << ", ";
		file << "color=\"" << (g[e].used ? "black" : "grey") << "\"";
		file << " ]\n";
	}
	file << "}";

}

bool net_handler::analyse_topology ()
{
	graph_t g(nodes.size());
	boost::property_map < graph_t, double edge_property::* >::type weights = boost::get(&edge_property::cost, g);

	// compute graph and edge weights:
	for (std::deque < physical_domain > :: size_type i = 0; i < domains.size(); ++i) {
		for (std::vector < uint64_t > :: const_iterator j = domains[i].nodes.begin(); j != domains[i].nodes.end(); ++j) {
			for (std::vector < uint64_t > :: const_iterator k = j + 1; k != domains[i].nodes.end(); ++k) {
				unsigned v1 = mac_map[*j];
				unsigned v2 = mac_map[*k];
				node_description const &n1 = nodes[v1];
				node_description const &n2 = nodes[v2];
				node_description::connection c1 = {'\0'};
				node_description::connection c2 = {'\0'};
				for (unsigned l = 0; l < n1.num_connections; ++l)
					if (n1.connections[l].domain == i)
						c1 = n1.connections[l];
				for (unsigned l = 0; l < n2.num_connections; ++l)
					if (n2.connections[l].domain == i)
						c2 = n2.connections[l];

				double distance_squared = sqr(c1.pos_x - c2.pos_x) + sqr(c1.pos_y - c2.pos_y);
				double cost = media[domains[i].type].cost * (1 + 2 * distance_squared / sqr(media[domains[i].type].range));

//				std::cout << "cost from " << n1.name << " to " << n2.name << " through " << domains[i].name << ": " << cost << std::endl;

				edge_descriptor e;
				bool b;
				boost::tie(e, b) = boost::add_edge(v1, v2, g);
				if (b || cost < g[e].cost) {
					g[e].cost = cost;
					g[e].type = domains[i].type;
					g[e].domain = i;
					if (v2 < v1) std::swap(c1.device, c2.device);
					g[e].device1 = c1.device;
					g[e].device2 = c2.device;
					g[e].used = false;
				}
			}
		}
	}

	std::vector < vertex_descriptor > parents(boost::num_vertices(g));
	std::vector < double > distances(boost::num_vertices(g));
	vertex_descriptor root = boost::vertex(0, g);

	boost::dijkstra_shortest_paths(g, root, boost::weight_map(weights).predecessor_map(&parents[0]).distance_map(&distances[0]));

	// mark tree edges on the graph:
	boost::graph_traits < graph_t > :: edge_iterator ei, ei_end;
	for (boost::tie(ei, ei_end) = boost::edges(g); ei != ei_end; ++ei) {
		vertex_descriptor u = boost::source(*ei, g), v = boost::target(*ei, g);
		if (parents[v] == u || parents[u] == v)
			g[*ei].used = true;
	}

	plot_graph(g, "/tmp/mesh.dot", false);

	// remove non-tree edges from the graph:
	for (boost::tie(ei, ei_end) = boost::edges(g); ei != ei_end; )
		if (!g[*ei].used)
			boost::remove_edge(*ei++, g);
		else
			++ei;


	// visit the tree and assign addresses:
	uint16_t address = 0;
	uint16_t depth = -1;
	vertex_descriptor v = root;
	bool discover = true;
	do {
		vertex_property &node = g[v];
		node_description *n = &nodes[v];
		if (discover) {
			node.address = address++;
			node.depth = depth++;
			n->net_address = node.address;
			n->hop_distance = node.depth;
			vertex_descriptor p = parents[v];
			typedef std::set < boost::tuple < char, uint8_t, uint16_t, vertex_descriptor > > children_t;
			children_t children;
			boost::graph_traits < graph_t >::out_edge_iterator ei, ei_end;
			for (boost::tie(ei, ei_end) = boost::out_edges(v, g); ei != ei_end; ++ei) {
				vertex_descriptor t = boost::target(*ei, g);
				if (t == p) continue;
				edge_property const &prop = g[*ei];
				char dev = v < t ? prop.device1 : prop.device2;
				children.insert(boost::make_tuple(dev, prop.type, prop.domain, t));
			}
			node.children.resize(children.size());
			unsigned count = 0;
			for (children_t::const_iterator i = children.begin(); i != children.end(); ++i)
				node.children[count++] = std::make_pair(i->get<3>(), i->get<0>());
			node.child_count = 0;
			node.span_start = 0;
			node.span_device = '\0';
			discover = false;
		}
		if (node.child_count < node.children.size()) {
			char dev = node.children[node.child_count].second;
			v = node.children[node.child_count++].first;
			discover = true;
			if (!node.span_device) {
				node.span_start = address;
				node.span_device = dev;
			}
			if (dev != node.span_device) {
				uint16_t span_end = address - 1;
				if (n->num_routes < n->max_routes) {
					unsigned r = n->num_routes;
					n->routes[r].device = node.span_device;
					n->routes[r].mask_length = 0xFF;
					n->routes[r].first = node.span_start;
					n->routes[r].last = span_end;
					++n->num_routes;
				} else {
					throw;
				}
				node.span_start = address;
				node.span_device = dev;
			}
			continue;
		}
		if (node.span_device) {
			uint16_t span_end = address - 1;
			if (n->num_routes < n->max_routes) {
				unsigned r = n->num_routes;
				n->routes[r].device = node.span_device;
				n->routes[r].mask_length = 0xFF;
				n->routes[r].first = node.span_start;
				n->routes[r].last = span_end;
				++n->num_routes;
			} else {
				throw;
			}
			node.span_start = 0;
			node.span_device = '\0';
		}
		if (v == root) break;
		v = parents[v];
		--depth;
	} while (true);

	
	plot_graph(g, "/tmp/tree.dot", true);

	for (unsigned i = 0; i < nodes.size(); ++i)
		while (nodes[i].net_address != i)
			std::swap(nodes[i], nodes[nodes[i].net_address]);

	std::cout << "Routing computed!" << std::endl;

	return true;
}


