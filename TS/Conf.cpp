/*
 *   Copyright (C) 2010-2015,2018,2020,2021 by Jonathan Naylor G4KLX
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

#include "Conf.h"
#include "Log.h"

#include <cstring>
#include <cassert>
#include <cstdio>

const std::string KEY_DAEMON         = "Daemon";
const std::string KEY_SCREEN_PORT    = "ScreenPort";
const std::string KEY_SCREEN_SPEED   = "ScreenSpeed";
const std::string KEY_DAEMON_ADDRESS = "DaemonAddress";
const std::string KEY_DAEMON_PORT    = "DaemonPort";
const std::string KEY_SELF_ADDRESS   = "SelfAddress";
const std::string KEY_SELF_PORT      = "SelfPort";
const std::string KEY_CHANNEL        = "Channel";
const std::string KEY_DESTINATION    = "Destination";
const std::string KEY_VOLUME         = "Volume";
const std::string KEY_METRIC         = "Metric";
const std::string KEY_DIM_LEVEL      = "DimLevel";
const std::string KEY_DIM_TIME		 = "DimTime";
const std::string KEY_BRIGHTNESS	 = "Brightness";

CConf::CConf() :
m_fileName(),
m_daemon(false),
m_screenPort("/dev/ttyUSB0"),
m_screenSpeed(9600U),
m_daemonAddress("127.0.0.1"),
m_daemonPort(7658U),
m_selfAddress("127.0.0.1"),
m_selfPort(7659U),
m_channel(),
m_destination(),
m_volume(100U),
m_dimLevel(100U),
m_dimTime(30U),
m_brightness(100U),
m_metric(true)

{
	char* home = ::getenv("HOME");
	if (home != NULL) {
		m_fileName = std::string(home);
		m_fileName.append("/.M17TS");
	}
}

CConf::~CConf()
{
}

bool CConf::read()
{
	assert(!m_fileName.empty());

	FILE* fp = ::fopen(m_fileName.c_str(), "rt");
	if (fp == NULL)
		return true;

	char buffer[100U];
	while (::fgets(buffer, 100, fp) != NULL) {
		if (buffer[0U] == '#') 
			continue;

		char* key = ::strtok(buffer, "=");
		char* val = ::strtok(NULL, "\r\n");

		if (key == NULL || val == NULL)
			continue;

		if (key == KEY_DAEMON)
			m_daemon = ::atoi(val) == 1;
		else if (key == KEY_SCREEN_PORT)
			m_screenPort = std::string(val);
		else if (key == KEY_SCREEN_SPEED)
			m_screenSpeed = (unsigned int)::atoi(val);
		else if (key == KEY_DAEMON_ADDRESS)
			m_daemonAddress = std::string(val);
		else if (key == KEY_DAEMON_PORT)
			m_daemonPort = (unsigned short)::atoi(val);
		else if (key == KEY_SELF_ADDRESS)
			m_selfAddress = std::string(val);
		else if (key == KEY_SELF_PORT)
			m_selfPort = (unsigned short)::atoi(val);
		else if (key == KEY_CHANNEL)
			m_channel = std::string(val);
		else if (key == KEY_DESTINATION)
			m_destination = std::string(val);
		else if (key == KEY_VOLUME)
			m_volume = (unsigned int)::atoi(val);
		else if (key == KEY_DIM_LEVEL)
			m_dimLevel = (unsigned int)::atoi(val);
		else if (key == KEY_DIM_TIME)
			m_dimTime = (unsigned int)::atoi(val);
		else if (key == KEY_BRIGHTNESS)
			m_brightness = (unsigned int)::atoi(val);			
		else if (key == KEY_METRIC)
			m_metric = ::atoi(val) == 1;									
	}

	::fclose(fp);

	return true;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

std::string CConf::getScreenPort() const
{
	return m_screenPort;
}

unsigned int CConf::getScreenSpeed() const
{
	return m_screenSpeed;
}

std::string CConf::getDaemonAddress() const
{
	return m_daemonAddress;
}

unsigned short CConf::getDaemonPort() const
{
	return m_daemonPort;
}

std::string CConf::getSelfAddress() const
{
	return m_selfAddress;
}

unsigned short CConf::getSelfPort() const
{
	return m_selfPort;
}

std::string CConf::getChannel() const
{
	return m_channel;
}

void CConf::setChannel(const std::string& value)
{
	m_channel = value;
}

std::string CConf::getDestination() const
{
	return m_destination;
}

void CConf::setDestination(const std::string& value)
{
	m_destination = value;
}

unsigned int CConf::getVolume() const
{
	return m_volume;
}

void CConf::setVolume(unsigned int value)
{
	m_volume = value;
}

unsigned int CConf::getDimLevel() const
{
	return m_dimLevel;
}

unsigned int CConf::getDimTime() const
{
	return m_dimTime;
}

unsigned int CConf::getBrightness() const
{
	return m_brightness;
}

bool CConf::getMetric() const
{
	return m_metric;
}



bool CConf::write()
{
	FILE* fp = ::fopen(m_fileName.c_str(), "wt");
	if (fp == NULL) {
		LogError("Cannot open the config file - %s", m_fileName.c_str());
		return false;
	}

	::fprintf(fp, "%s=%d\n", KEY_DAEMON.c_str(), m_daemon ? 1 : 0);

	::fprintf(fp, "%s=%s\n", KEY_SCREEN_PORT.c_str(), m_screenPort.c_str());
	::fprintf(fp, "%s=%u\n", KEY_SCREEN_SPEED.c_str(), m_screenSpeed);


	::fprintf(fp, "%s=%s\n", KEY_DAEMON_ADDRESS.c_str(), m_daemonAddress.c_str());
	::fprintf(fp, "%s=%u\n", KEY_DAEMON_PORT.c_str(), m_daemonPort);

	::fprintf(fp, "%s=%s\n", KEY_SELF_ADDRESS.c_str(), m_selfAddress.c_str());
	::fprintf(fp, "%s=%u\n", KEY_SELF_PORT.c_str(), m_selfPort);

	::fprintf(fp, "%s=%s\n", KEY_CHANNEL.c_str(), m_channel.c_str());
	::fprintf(fp, "%s=%s\n", KEY_DESTINATION.c_str(), m_destination.c_str());

	::fprintf(fp, "%s=%u\n", KEY_VOLUME.c_str(), m_volume);

	::fprintf(fp, "%s=%u\n", KEY_DIM_LEVEL.c_str(), m_dimLevel);
	::fprintf(fp, "%s=%u\n", KEY_DIM_TIME.c_str(), m_dimTime);
	::fprintf(fp, "%s=%u\n", KEY_BRIGHTNESS.c_str(), m_brightness);

	::fprintf(fp, "%s=%u\n", KEY_METRIC.c_str(), m_metric ? 1U : 0U);

	::fclose(fp);

	return true;
}

