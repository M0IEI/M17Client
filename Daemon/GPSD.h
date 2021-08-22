/*
 *   Copyright (C) 2018,2020,2021 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if defined(USE_GPSD)

#if !defined(GPSD_H)
#define	GPSD_H

#include "Timer.h"

#include <string>

#include <gps.h>

class CGPSD {
public:
	CGPSD(const std::string& address, const std::string& port);
	~CGPSD();

	bool open();

	bool getData(float& latitude, float& longitude, float& altitude, float& speed, float& track);

	void clock(unsigned int ms);

	void close();

private:
	std::string       m_gpsdAddress;
	std::string       m_gpsdPort;
	struct gps_data_t m_gpsdData;
	CTimer            m_timer;
};

#endif

#endif
