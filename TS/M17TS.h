/*
 *   Copyright (C) 2015,2016,2017,2019,2020,2021 by Jonathan Naylor G4KLX
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

#if !defined(M17TS_H)
#define	M17TS_H

#include "UARTController.h"
#include "UDPSocket.h"
#include "Conf.h"

#include <vector>
#include <string>
#include <optional>

enum SLIDER_ID {
	SI_NONE,
	SI_VOLUME
};

class CM17TS
{
public:
	CM17TS();
	virtual ~CM17TS();

	int run();

private:
	CConf            m_conf;
	CUDPSocket*      m_socket;
	CUARTController* m_uart;
	sockaddr_storage m_sockaddr;
	unsigned int     m_sockaddrLen;

	std::vector<std::string> m_channels;
	std::vector<std::string> m_destinations;

	unsigned int m_channelIdx;
	unsigned int m_destinationIdx;
	
	bool         m_localTX;
	bool         m_transmit;
	bool         m_receive;

	unsigned int m_page;

	SLIDER_ID    m_slider;
	unsigned int m_volume;

	unsigned int m_sMeter;
	
	unsigned int m_dimLevel;
	unsigned int m_dimTime;

	std::string  m_source;
	std::string  m_text;
	std::string  m_callsigns;

	bool         m_metric;
	
	void parseCommand(char* command);
	void parseScreen(const uint8_t* command, unsigned int length);

	void channelChanged(int val);
	void destinationChanged(int val);

	void transmit();

	void showRX(bool end, const std::string& source, const std::string& destination);
	void showText();
	void showCallsigns();
	void showRSSI(int value);
	void showGPS(float latitude, float longitude, const std::string& locator,
		const std::optional<float>& altitude,
		const std::optional<float>& speed, const std::optional<float>& track,
		const std::optional<float>& bearing, const std::optional<float>& distance);

	void drawPointer(float bearing);
	
	void volumeChanged();

	void gotoPage0();
	void gotoPage1();

	bool getChannels();
	bool setChannel(const std::string& channel);
	bool getDestinations();
	bool setDestination(const std::string& destination);
	bool setVolume(unsigned int volume);
	bool setTransmit(bool transmit);

	void sendCommand(const char* command);
	
	void selectChannel();
	void selectDestination();
};

#endif

