//==========================================================================
// netshell.h
// ToLHnet shell command interpreter
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

#ifndef _NETSHELL_H
#define _NETSHELL_H

#include "dispatcher.h"

class netshell : public controller
{
public:
	explicit netshell (int socket);
	virtual ~netshell ();
	bool process_input ();
	void process_datagram (class directed_command const *cmd, uint16_t src, int code, void *data, size_t length);
private:
	static const std::string protocol;
	struct timeval *timestamps;
	int test_mode;
};

#endif
