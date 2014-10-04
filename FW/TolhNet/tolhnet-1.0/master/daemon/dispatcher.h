//==========================================================================
// dispatcher.h
// tolhnet connection dispatcher
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

#ifndef _DISPATCHER_H
#define _DISPATCHER_H

#include <stdint.h>
#include <vector>
#include <string>

#include <sys/time.h>

class controller
{
public:
	explicit controller (int socket);
	virtual ~controller ();
	virtual void send (std::string const &msg);
	virtual bool process_input () = 0;
	virtual void process_datagram (class directed_command const *cmd, uint16_t src, int code, void *data, size_t length) = 0;
	int get_socket () const {return connection;}
	int last_sequence;
protected:
	const int connection;
};


class connection_dispatcher
{
public:
	connection_dispatcher ();
	~connection_dispatcher ();
	void run ();
	typedef void (*callback_t) ();
	static void process_datagram (class directed_command const *cmd, uint16_t src, int code, void *data, size_t length);
	static void schedule_event (callback_t callback, uint32_t microseconds);
	void dispatch (std::string const &msg, int dst);
private:
	static connection_dispatcher *dispatcher;
	void check_link_connection ();
	struct timeval time_next_event;
	int file;
	int lst1, lst2;
	std::vector < controller * > ctrls;
	struct event {
		callback_t function;
		struct timeval time;
	};
	std::vector < event > events;
};

#endif
