//==========================================================================
// serial.h
// Serial port driver and framing utilities
/***************************************************************************
 * Copyright (C) 2008-2014 Giorgio Biagetti                                *
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

#ifndef SERIAL_H
#define SERIAL_H

#include <ostream>
#include <string>
#include <queue>

class serial_io
{
public:
	serial_io ();
	~serial_io ();
	enum line_status {undefined, not_present, not_ready, connected} status;
	void set_device (std::string const &devname, int speed);
	void set_log (std::ostream &stream);
	std::string const &get_device () const {return devname;}
	int get_speed () const {return devspeed;}
	int get_file () const {return serial_fd;}
	line_status check_status ();
	void send (std::string const &line);
	bool recv ();
	void open ();
	void close ();
public:
	std::queue <std::string> lines;
private:
	std::string devname;
	int devspeed;
	std::ostream *logstream;
	enum {buflen = 4096};
	char buffer[buflen];
	int bufstart, framing;
	int serial_fd;
};

#endif
