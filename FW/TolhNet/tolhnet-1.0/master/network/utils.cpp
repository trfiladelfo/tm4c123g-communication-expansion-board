//==========================================================================
// utils.cpp
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

#include "utils.h"

#include <fstream>
#include <sstream>
#include <iomanip>


// Implementation of the configuration file reader class:

setup_reader::setup_reader (const char *filename, const char *error_category) :
	error_category(error_category ? error_category : filename)
{
	f.open(filename);
	finished = false;
	line_number = 0;
}

setup_reader::~setup_reader ()
{
	if (!finished)
		printf("Error in %s at line %d.\n", error_category.c_str(), line_number);
	f.close();
}

std::vector < std::string > split_fields (std::string const &line)
{
	// This function ignores final blanks, removes comments, and coalesces consecutive blanks.
	std::vector <std::string> fields;
	bool initial_blanks = false;
	for (std::string::size_type t1 = 0, t2 = 0; (t2 = line.find_first_of(" \t", t1)) || 1; t1 = t2 + 1) {
		if (t2 == 0) initial_blanks = true;
		if (t2 > t1 && t1 < line.size()) {
			if (line[t1] == '#') break;
			if (initial_blanks) fields.push_back(std::string());
			initial_blanks = false;
			if (line[t1] != '"') {
				fields.push_back(line.substr(t1, t2 - t1));
			} else {
				// parse quoted string:
				std::string field = "\"";
				t2 = t1;
				bool escape = false;
				while (++t2 < line.size())
					if (escape) {
						escape = false;
						switch (line[t2]) {
						case '0': field += '\0'; break;
						case 'a': field += '\a'; break;
						case 'b': field += '\b'; break;
						case 'f': field += '\f'; break;
						case 'n': field += '\n'; break;
						case 'r': field += '\r'; break;
						case 't': field += '\t'; break;
						case 'v': field += '\v'; break;
						default : field += line[t2];
						}
					} else {
						if (line[t2] == '\\') {
							escape = true;
						} else {
							field += line[t2];
							if (line[t2] == '"') break;
						}
					}
				fields.push_back(field);
			}
		}
		if (t2 == std::string::npos) break;
	}
	return fields;
}

std::string join_fields (std::vector < std::string> :: const_iterator begin, std::vector < std::string> :: const_iterator end)
{
	std::string result;
	while (begin != end) {
		if (!result.empty()) result += ' ';
		result += *begin++;
	}
	return result;
}


bool setup_reader::read ()
{
	if (finished) return false;
	std::string line;
	while (std::getline(f, line)) {
		++line_number;
		// split line at blank spaces:
		fields = split_fields(line);
		// ignores empty lines:
		if (fields.empty()) continue;
		return true;
	}
	finished = true;
	return false;
}

bool setup_reader::done () const
{
	return finished;
}

unsigned setup_reader::line () const
{
	return line_number;
}

// Utility conversion functions:

uint64_t from_hex (const char *p, int bits)
{
	uint64_t result;
	for (result = 0; *p && (bits -= 4) >= 0; ++p) {
		uint8_t nibble;
		if (*p >= '0' && *p <= '9')
			nibble = *p - '0';
		else if (*p >= 'A' && *p <= 'F')
			nibble = *p - 'A' + 10;
		else if (*p >= 'a' && *p <= 'f')
			nibble = *p - 'a' + 10;
		else
			break;
		result <<= 4;
		result += nibble;
	}
	return result;
}

uint64_t from_dec (const char *p)
{
	uint64_t result;
	for (result = 0; *p; ++p) {
		result *= 10;
		result += *p - '0';
	}
	return result;
}

uint64_t from_bin (const char *p)
{
	uint64_t result = 0;
	uint8_t const *buf = (const uint8_t *) p;
	uint8_t len;
	for (len = 1; len < 9; ++len)
		if (!(buf[0] & ((uint8_t) 1 << (8 - len)))) break;
	for (uint8_t bit = (((uint8_t) 1 << (len - 1)) - (uint8_t) 1) << (9 - len); len--; bit = 0)
		result |= (uint64_t) (*buf++ ^ bit) << (len * 8);
	return result;
}

std::string to_hex (uint64_t x, int bits)
{
	std::ostringstream s;
	x &= ((uint64_t) 2 << (bits - 1)) - (uint64_t) 1;
	s << std::setw(bits / 4) << std::setfill('0') << std::right << std::hex << std::uppercase << x;
	return s.str().substr(0, bits / 4);
}

std::string to_hex (std::string const &x, char separator)
{
	std::ostringstream s;
	for (std::string::size_type i = 0; i < x.size(); ++i) {
		if (separator && i) s << separator;
		s << std::setw(2) << std::setfill('0') << std::right << std::hex << std::uppercase << (unsigned) (uint8_t) x[i];
	}
	return s.str();
}

std::string to_dec (uint64_t x)
{
	std::ostringstream s;
	s << x;
	return s.str();
}

// Variable-length binary encoding, big-endian.
// Number of bits set to 1 at the beginning of the first byte
// denotes the number of bytes that follows.
// If the parameter "bits" is specified,
// then use a simple fixed-length encoding.
std::string to_bin (uint64_t x, int bits)
{
	uint8_t len = bits / 8, bit, buf[9];
	if (!len) // determine the required length:
		for (len = 1; (bit = 8 * (len - 1) + (len < 8 ? 8 - len : 0)) < 64; ++len)
			if (x <= (((uint64_t) 1 << bit) - (uint64_t) 1)) break;
	// store in big-endian:
	for (bit = 0; bit < len; ++bit)
		buf[bit] = (x >> (8 * (len - 1 - bit))) & 0xFF;
	if (!bits) // if variable-length encoding has been requested, store length:
		buf[0] |= (((uint8_t) 1 << (len - 1)) - (uint8_t) 1) << (9 - len);
	// return encoded buffer:
	return std::string((char *) buf, len);
}

uint64_t convert_mac_address (std::string const &addr)
{
	if (addr.size() == 12 + 5 &&
		addr[ 2] == ':' &&
		addr[ 5] == ':' &&
		addr[ 8] == ':' &&
		addr[11] == ':' &&
		addr[14] == ':'
	) {
		const char *s = addr.c_str();
		uint64_t x =
			(from_hex(s +  0, 8) << 40) |
			(from_hex(s +  3, 8) << 32) |
			(from_hex(s +  6, 8) << 24) |
			(from_hex(s +  9, 8) << 16) |
			(from_hex(s + 12, 8) <<  8) |
			(from_hex(s + 15, 8) <<  0);
		return x;
	} else {
		return 0;
	}
}
