//==========================================================================
// main.h
// Implementation of a TCP+IO+socket daemon
// Dispatcher based on giorby's teletext server, (C) 2003-2005
/***************************************************************************
 * Copyright (C) 2003-2014 Giorgio Biagetti                                *
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

#ifndef _MAIN_H
#define _MAIN_H

#include <string>

extern std::string file_media;
extern std::string file_topology;
extern std::string file_addressing;

extern std::string file_commands;

struct iface_spec {
	char interface;
	std::string type;
	std::string name;
	std::string parameters;
};

extern iface_spec interfaces[8];

#endif
