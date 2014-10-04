//==========================================================================
// utils.h
// Utility functions to handle network configuration data
/***************************************************************************
 * Copyright (C) 2013 Giorgio Biagetti                                     *
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

#ifndef __UTILS_H
#define __UTILS_H

#include <vector>
#include <string>
#include <fstream>

#include <stdint.h>

// Configuration file reader helper class:

class setup_reader
{
public:
	setup_reader (const char *filename, const char *error_category = 0);
	~setup_reader ();
	bool read ();
	bool done () const;
	unsigned line () const;
	std::vector < std::string > fields;
	operator bool () const { return f; }
private:
	std::string error_category;
	std::ifstream f;
	unsigned line_number;
	bool finished;
};


// Utility conversion functions:

uint64_t from_hex (const char *p, int bits);
uint64_t from_dec (const char *p);
uint64_t from_bin (const char *p);
std::string to_hex (uint64_t x, int bits);
std::string to_dec (uint64_t x);
std::string to_bin (uint64_t x, int bits = 0);

uint64_t convert_mac_address (std::string const &addr);
std::string to_hex (std::string const &x, char separator = ' ');

std::vector < std::string > split_fields (std::string const &line);
std::string join_fields (std::vector < std::string> :: const_iterator begin, std::vector < std::string> :: const_iterator end);

#endif
