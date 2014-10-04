//==========================================================================
// serial.cpp
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

#include "serial.h"
#include <iostream>
#include <iomanip>

#define log (logstream ? *logstream : std::cerr)

#undef  LOG_SERIAL_BYTES
#undef  LOG_SERIAL_LINES

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


serial_io::serial_io ()
{
	status = undefined;
	bufstart = 0;
	framing = 0;
	devspeed = 0;
	serial_fd = 0;
	logstream = 0;
}

serial_io::~serial_io ()
{
	close();
}

void serial_io::set_device (std::string const &devname, int speed)
{
	close();
	this->devname = devname;
	this->devspeed = speed;
}

void serial_io::set_log (std::ostream &stream)
{
	logstream = &stream;
}

void serial_io::open ()
{
	if (serial_fd) {
		// serial port already open - do nothing.
		return;
	}
	if (devname.empty() || !devspeed) {
		// serial port not yet specified - abort.
		status = undefined;
		return;
	}
	struct termios mode;
	speed_t baud;
	serial_fd = ::open(devname.c_str(), O_RDWR);
	if (serial_fd < 0) {
		// unable to open serial port.
		status = not_present;
		serial_fd = 0;
		return;
	}
	switch (devspeed) {
	case    1200: baud =    B1200; break;
	case    2400: baud =    B2400; break;
	case    4800: baud =    B4800; break;
	case    9600: baud =    B9600; break;
	case   19200: baud =   B19200; break;
	case   38400: baud =   B38400; break;
	case   57600: baud =   B57600; break;
	case  115200: baud =  B115200; break;
	case  500000: baud =  B500000; break;
	case 1000000: baud = B1000000; break;
	default:      baud = B0;
	}
	tcgetattr(serial_fd, &mode);
	cfmakeraw(&mode);
	cfsetispeed(&mode, baud);
	cfsetospeed(&mode, baud);
	mode.c_cflag |= CLOCAL;
	tcsetattr(serial_fd, TCSANOW, &mode);
	log << "Serial port " << devname << " opened at " << devspeed << " baud." << std::endl;
	check_status();
}

void serial_io::close ()
{
	if (serial_fd) {
		::close(serial_fd);
		log << "Serial port " << devname << " closed." << std::endl;
	}
	serial_fd = 0;
	status = undefined;
}

serial_io::line_status serial_io::check_status ()
{
	line_status status = not_present;
	if (!serial_fd) open();
	if (serial_fd) {
		int lines;
		if (ioctl(serial_fd, TIOCMGET, &lines) < 0) {
			// error? assume serial port is not working, so don't set status.
		} else if (lines & TIOCM_CTS)
			status = connected;
		else
			status = not_ready;
	}
	this->status = status;
	return status;
}


bool serial_io::recv ()
{
	int r;
	bool pending = !lines.empty();

	if (!serial_fd) return pending;
	if ((r = read(serial_fd, buffer + bufstart, buflen - bufstart)) <= 0) {
		log << "Serial port read returned error " << errno << std::endl;
		close();
		return pending;
	}

#ifdef LOG_SERIAL_BYTES
	log << "RX:";
	for (std::string::size_type i = 0; i < (unsigned) r; ++i)
		log << ' ' << std::hex << std::setw(2) << std::setfill('0') << (unsigned) (unsigned char) buffer[bufstart + i];
	log << std::endl;
#endif

	r += bufstart;

	int old = 0;
	for (int i = bufstart; i < r; ) {
		if (framing) {
			/* Check if there are at least 2 chars */
			if (r - old < 2) break;
			/* Check if the string is complete */
			int msglen = 5 + (int) (unsigned char) buffer[old + 1];
			if (r - old < msglen) break;
			/* The string is complete, extract it: */
			framing = 0;
			std::string line(buffer + old, msglen);
#ifdef LOG_SERIAL_LINES
			log << "DATA:";
			for (std::string::size_type j = 0; j < line.size(); ++j)
				log << ' ' << std::hex << std::setw(2) << std::setfill('0') << (unsigned) (unsigned char) line[j];
			log << std::endl;
#endif
			lines.push(line);
			pending = true;
			/* remove the complete string from the buffer and keep looking: */
			i = old += msglen;
		} else {
			int end = -1;
			if (buffer[i] == '\n') end = i + 1;
			if (buffer[i] == '\006' /*  ACK */ ) end = i + 1;
			if (buffer[i] == '\025' /* NACK */ ) end = i + 1;
			if (buffer[i] == '\002' /*  STX */ ) {
				end = i;
				framing = 1;
			}
			if (end > old) {
				std::string line(buffer + old, buffer + end);
#ifdef LOG_SERIAL_LINES
				log << "TEXT:";
				for (std::string::size_type j = 0; j < line.size(); ++j)
					log << ' ' << std::hex << std::setw(2) << std::setfill('0') << (unsigned) (unsigned char) line[j];
				log << std::endl;
#endif
				lines.push(line);
				pending = true;
				old = end;
			}
			++i;
		}
	}
	if ((bufstart = r -= old)) memmove(buffer, buffer + old, r);

	return pending;
}


void serial_io::send (std::string const &line)
{
	if (!serial_fd) return;

	const char *p = line.c_str();
	std::string::size_type len = line.size();

#ifdef LOG_SERIAL_BYTES
	log << "TX:";
	for (std::string::size_type i = 0; i < len; ++i)
		log << ' ' << std::hex << std::setw(2) << std::setfill('0') << (unsigned) (unsigned char) line[i];
	log << std::endl;
#endif

	if (len > 0xFFFFFF) return; // safety check!
	while (len) {
		int w = ::write(serial_fd, p, len);
		if (w <= 0) {
			log << "Serial write failed!" << std::endl;
			close();
			break;
		} else {
			p += w;
			len -= w;
		}
	}
}

