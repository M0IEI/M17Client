/*
 *   Copyright (C) 2010-2014,2018,2020,2021 by Jonathan Naylor G4KLX
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

#ifndef	Conf_H
#define	Conf_H

#include <string>

class CConf {
public:
	CConf();
	~CConf();

	bool read();

	bool           getDaemon() const;

	std::string    getScreenPort() const;
	unsigned int   getScreenSpeed() const;

	std::string    getDaemonAddress() const;
	unsigned short getDaemonPort() const;

	std::string    getSelfAddress() const;
	unsigned short getSelfPort() const;

	void           setChannel(const std::string& value);
	std::string    getChannel() const;

	void           setDestination(const std::string& value);
	std::string    getDestination() const;

	void           setVolume(unsigned int value);
	unsigned int   getVolume() const;
	
//PANTALLA
	unsigned int   getDimLevel() const;
	unsigned int   getDimTime() const;	
//

	bool           getMetric() const;



	bool write();

private:
	std::string    m_fileName;
	bool           m_daemon;
	std::string    m_screenPort;
	unsigned int   m_screenSpeed;

	std::string    m_daemonAddress;
	unsigned short m_daemonPort;
	std::string    m_selfAddress;
	unsigned short m_selfPort;
	std::string    m_channel;
	std::string    m_destination;
	unsigned int   m_volume;
	bool           m_metric;
	//PANTALLA
	unsigned int   m_dimLevel;
	unsigned int	m_dimTime;
//
};

#endif
