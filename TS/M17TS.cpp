/*
 *   Copyright (C) 2015-2021 by Jonathan Naylor G4KLX
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

#include "M17TS.h"
#include "GitVersion.h"
#include "Version.h"
#include "Thread.h"
#include "Timer.h"
#include "Utils.h"
#include "Log.h"

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>

#define	DEG2RAD(x)	((x / 180.0F) * 3.14159F)
#define	RAD2DEG(x)	((x / 3.14159F) * 180.0F)

const char* DELIMITER = ":";

const unsigned int RSSI_BASE = 140U;

static bool m_killed = false;
static int  m_signal = 0;

const int COMPASS_X = 250;
const int COMPASS_Y = 180;
const int COMPASS_R = 140;

static void sigHandler(int signum)
{
	m_killed = true;
	m_signal = signum;
}

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2021 by Jonathan Naylor, G4KLX and others";

int main(int argc, char** argv)
{
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "M17TS version %s git #%.7s\n", VERSION, gitversion);
				return 0;
			} else if (arg.substr(0,1) == "-") {
				::fprintf(stderr, "Usage: M17TS [-v|--version]\n");
				return 1;
			}
		}
	}

	::signal(SIGINT,  sigHandler);
	::signal(SIGTERM, sigHandler);
	::signal(SIGHUP,  sigHandler);

	int ret = 0;

	do {
		m_signal = 0;

		CM17TS* host = new CM17TS;
		ret = host->run();

		delete host;

		if (m_signal == 2)
			::LogInfo("M17TS-%s exited on receipt of SIGINT", VERSION);

		if (m_signal == 15)
			::LogInfo("M17TS-%s exited on receipt of SIGTERM", VERSION);

		if (m_signal == 1)
			::LogInfo("M17TS-%s restarted on receipt of SIGHUP", VERSION);
	} while (m_signal == 1);

	::LogFinalise();

	return ret;
}

CM17TS::CM17TS() :
m_conf(),
m_socket(NULL),
m_uart(NULL),
m_sockaddr(),
m_sockaddrLen(0U),
m_channels(),
m_destinations(),
m_channelIdx(0U),
m_destinationIdx(0U),
m_localTX(false),
m_transmit(false),
m_receive(false),
m_page(0U),
m_slider(SI_NONE),
m_volume(50U),
m_sMeter(0U),
m_dimLevel(100U),
m_dimTime(30U),
m_source(),
m_text(),
m_callsigns(),
m_metric(true)

{
}

CM17TS::~CM17TS()
{
}

int CM17TS::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "M17TS: cannot read the configuration file\n");
		return 1;
	}

	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return -1;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1){
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return -1;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1){
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return -1;
		}

		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("m17");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the m17 user, exiting\n");
				return -1;
			}
			
			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to m17:m17
			if (setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set m17 GID, exiting\n");
				return -1;
			}

			if (setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set m17 UID, exiting\n");
				return -1;
			}
		    
			// Double check it worked (AKA Paranoia) 
			if (setuid(0) != -1) {
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return -1;
			}
		}
	}

	ret = ::LogInitialise(m_daemon, ".", "M17TS", 1U, 1U, true);
	if (!ret) {
		::fprintf(stderr, "M17TS: unable to open the log file\n");
		return 1;
	}

	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}

	LogInfo(HEADER1);
	LogInfo(HEADER2);
	LogInfo(HEADER3);
	LogInfo(HEADER4);

	LogMessage("M17TS-%s is starting", VERSION);
	LogMessage("Built %s %s (GitID #%.7s)", __TIME__, __DATE__, gitversion);

	m_uart = new CUARTController(m_conf.getScreenPort(), m_conf.getScreenSpeed());
	ret = m_uart->open();
	if (!ret) {
		delete m_uart;
		LogError("Unable to open the screen");
		::LogFinalise();
		return 1;
	}

	if (CUDPSocket::lookup(m_conf.getDaemonAddress(), m_conf.getDaemonPort(), m_sockaddr, m_sockaddrLen) != 0) {
		LogError("Could not lookup the daemon address");
		::LogFinalise();
		return 1;
	}

	m_socket = new CUDPSocket(m_conf.getSelfAddress(), m_conf.getSelfPort());
	ret = m_socket->open();
	if (!ret) {
		delete m_socket;
		LogError("Unable to open the control socket");
		::LogFinalise();
		return 1;
	}

	m_metric = m_conf.getMetric();

	m_volume = m_conf.getVolume();
	m_dimLevel = m_conf.getDimLevel();
	m_dimTime = m_conf.getDimTime();
	m_brightness = m_conf.getBrightness();
	setVolume(m_volume);

	LogMessage("M17TS-%s is running", VERSION);

	CTimer timer(1000U, 0U, 100U);
	timer.start();
		//pass timer and brightnessconfig to display
		char text[100U];
		::sprintf(text, "dimLevel=%u", m_dimLevel);
		sendCommand(text);
		::sprintf(text, "dimTime=%u", m_dimTime);
		sendCommand(text);
		::sprintf(text, "brightness=%u", m_brightness);
		sendCommand(text);

	sendCommand("bkcmd=2");

	gotoPage1();

		::sprintf(text, "TEXT.txt=\"M17TS %s started\"", VERSION);
		sendCommand(text);

	uint8_t screenBuffer[50U];
	uint8_t endBuffer[3U] = {0x00U, 0x00U, 0x00U};
	unsigned int screenIdx = 0U;

	while (!m_killed) {
		char command[100U];
		sockaddr_storage sockaddr;
		unsigned int sockaddrLen = 0U;
		int ret = m_socket->read(command, 100U, sockaddr, sockaddrLen);
		if (ret > 0) {
			command[ret] = '\0';
			parseCommand(command);
		}

		uint8_t c;
		ret = m_uart->read(&c, 1U);
		if (ret > 0) {
			screenBuffer[screenIdx++] = c;

			endBuffer[2U] = endBuffer[1U];
			endBuffer[1U] = endBuffer[0U];
			endBuffer[0U] = c;

			if (::memcmp(endBuffer, "\xFF\xFF\xFF", 3U) == 0) {
				parseScreen(screenBuffer, screenIdx);
				::memset(endBuffer, 0x00U, 3U);
				screenIdx = 0U;
			}
		}

		timer.clock(20U);
		if (timer.isRunning() && timer.hasExpired()) {
			if (m_channels.empty()) {
				getChannels();
				timer.start();
			} else if (m_destinations.empty()) {
				getDestinations();
				timer.start();
			} else {
				timer.stop();
			}
		}

		CThread::sleep(20U);
	}

	m_conf.write();

	m_socket->close();
	m_uart->close();

	delete m_uart;
	delete m_socket;

	::LogFinalise();

	return 0;
}

void CM17TS::parseCommand(char* command)
{
	assert(command != NULL);

	std::vector<char *> ptrs;

	char* s = command;
	char* p;
	while ((p = ::strtok(s, DELIMITER)) != NULL) {
		s = NULL;
		ptrs.push_back(p);
	}

	if (::strcmp(ptrs.at(0U), "CHAN") == 0) {
		m_channels.clear();
		for (unsigned int i = 1U; i < ptrs.size(); i++) {
			std::string channel = std::string(ptrs.at(i));
			m_channels.push_back(channel);
		}
		selectChannel();
	} else if (::strcmp(ptrs.at(0U), "DEST") == 0) {
		m_destinations.clear();
		for (unsigned int i = 1U; i < ptrs.size(); i++) {
			std::string destination = std::string(ptrs.at(i));
			m_destinations.push_back(destination);
		}
		selectDestination();
	} else if (::strcmp(ptrs.at(0U), "RX") == 0) {
		bool end                = ::atoi(ptrs.at(1U)) == 1;
		std::string source      = std::string(ptrs.at(2U));
		std::string destination = std::string(ptrs.at(3U));
		showRX(end, source, destination);
	} else if (::strcmp(ptrs.at(0U), "TX") == 0) {
		m_transmit = ::atoi(ptrs.at(1U)) == 1;
//		char text [100U];
		if (m_transmit){
//			gotoPage1();
		//stop dimmer timer
			sendCommand("dim=brightness");
			sendCommand("timer=dimTime");
			sendCommand("tm0.en=0");

/*
			::sprintf(text, "dim=%u", m_brightness);
			sendCommand(text);
			::sprintf(text, "timer=%u", m_dimTime);
			sendCommand(text)
*/
			sendCommand("TX.txt=\"TX\"");
		}
		else
			sendCommand("TX.txt=\"\"");
/*		//reset timer and start counting
			::sprintf(text, "timer=%u", m_dimTime);
			sendCommand(text);

			sendCommand("tm0.en=1");
*/
	} else if (::strcmp(ptrs.at(0U), "TEXT") == 0) {
		m_text = std::string(ptrs.at(1U));
		showText();
	} else if (::strcmp(ptrs.at(0U), "CALLS") == 0) {
		m_callsigns = std::string(ptrs.at(1U));
		showCallsigns();
	} else if (::strcmp(ptrs.at(0U), "RSSI") == 0) {
		int rssi = ::atoi(ptrs.at(1U));
		showRSSI(rssi);
	} else if (::strcmp(ptrs.at(0U), "GPS") == 0) {
		float latitude   = std::stof(ptrs.at(1U));
		float longitude  = std::stof(ptrs.at(2U));
		std::string locator = std::string(ptrs.at(3U));

		std::optional<float> altitude;
		if (::strlen(ptrs.at(4U)) > 0)
			altitude = std::stof(ptrs.at(4U));

		std::optional<float> speed;
		if (::strlen(ptrs.at(5U)) > 0)
			speed = std::stof(ptrs.at(5U));

		std::optional<float> track;
		if (::strlen(ptrs.at(6U)) > 0)
			track = std::stof(ptrs.at(6U));

		std::optional<float> bearing;
		if (::strlen(ptrs.at(7U)) > 0)
			bearing = std::stof(ptrs.at(7U));

		std::optional<float> distance;
		if (::strlen(ptrs.at(8U)) > 0)
			distance = std::stof(ptrs.at(8U));

		showGPS(latitude, longitude, locator, altitude, speed, track, bearing, distance);
	}
}

void CM17TS::parseScreen(const uint8_t* command, unsigned int length)
{
	assert(command != NULL);

	if (command[0U] == 0x65U) {
		if (command[1U] == 1U) {
			if (command[2U] == 3U) {
				LogMessage("Page 1 CHAN_UP pressed");
				channelChanged(+1);
			} else if (command[2U] == 4U) {
				LogMessage("Page 1 CHAN_DOWN pressed");
				channelChanged(-1);
			} else if (command[2U] == 5U) {
				LogMessage("Page 1 DEST_UP pressed");
				destinationChanged(+1);
			} else if (command[2U] == 6U) {
				LogMessage("Page 1 DEST_DOWN pressed");
				destinationChanged(-1);
			} else if (command[2U] == 7U) {
				LogMessage("Page 1 RIGHT pressed");
				gotoPage1();
			} else if (command[2U] == 8U) {
				LogMessage("Page 1 LEFT pressed");
				gotoPage1();
			} else if (command[2U] == 10U) {
				LogMessage("Page 1 VOLUME adjusted");
				volumeChanged();
			} else {
				CUtils::dump(2U, "Button press on page 1 from an unknown button", command, length);
			}
		} else if (command[1U] == 2U) {
			if (command[2U] == 1U) {
				LogMessage("Page 2 RIGHT pressed");
				gotoPage0();
			} else if (command[2U] == 2U) {
				LogMessage("Page 2 LEFT pressed");
				gotoPage0();
			} else if (command[2U] == 3U) {
				LogMessage("Page 2 TRANSMIT pressed");
				transmit();
			} else {
				CUtils::dump(2U, "Button press on page 2 from an unknown button", command, length);
			}
		} else {
			CUtils::dump(2U, "Button press from an unknown page", command, length);
		}
	} else if (command[0U] == 0x71U) {
		switch (m_slider) {
			case SI_VOLUME:
				m_volume = (uint32_t(command[4U]) << 24) | (uint32_t(command[3U]) << 16) | (uint32_t(command[2U]) << 8) | (uint32_t(command[1U]) << 0);
				setVolume(m_volume);
				m_slider = SI_NONE;
				break;
			default:
				// CUtils::dump(2U, "Unknown slider data from the screen", command, length);
				break;
		}
	} else {
		CUtils::dump(2U, "Unknown data from the screen", command, length);
	}
}

void CM17TS::channelChanged(int val)
{
	assert(!m_channels.empty());

	if (val == -1) {
		if (m_channelIdx == 0U)
			m_channelIdx = m_channels.size() - 1U;
		else
			m_channelIdx--;
	} else {
		m_channelIdx++;
		if (m_channelIdx == m_channels.size())
			m_channelIdx = 0U;
	}

	std::string channel = m_channels.at(m_channelIdx);

	char text[100U];
	::sprintf(text, "CHANNEL.txt=\"%s\"", channel.c_str());
	sendCommand(text);

	m_conf.setChannel(channel);
	m_conf.write();

	setChannel(channel);
}

void CM17TS::destinationChanged(int val)
{
	assert(!m_destinations.empty());

	if (val == -1) {
		if (m_destinationIdx == 0U)
			m_destinationIdx = m_destinations.size() - 1U;
		else
			m_destinationIdx--;
	} else {
		m_destinationIdx++;
		if (m_destinationIdx == m_destinations.size())
			m_destinationIdx = 0U;
	}

	std::string destination = m_destinations.at(m_destinationIdx);

	char text[100U];
	::sprintf(text, "DESTINATION.txt=\"%s\"", destination.c_str());
	sendCommand(text);

	m_conf.setDestination(destination);
	m_conf.write();

	setDestination(destination);
}

void CM17TS::volumeChanged()
{
	m_slider = SI_VOLUME;
	sendCommand("get VOLUME.val");
}

void CM17TS::transmit()
{
	m_localTX = !m_localTX;

	setTransmit(m_localTX);
}

void CM17TS::showRX(bool end, const std::string& source, const std::string& destination)
{
	if (end) {
		m_receive = false;

		m_sMeter = 0U;
		m_source.clear();
		m_text.clear();
		m_callsigns.clear();

		sendCommand("S_METER.val=0");
		sendCommand("RX.txt=\"\"");
	//set Lastheard colours and reset the dimmer timer
		sendCommand("CALLSIGNS.pco=BLUE");
		sendCommand("TEXT.pco=BLUE");
		sendCommand("SOURCE.pco=BLUE");

//		char text[100U];
//		::sprintf(text, "timer=%u", m_dimTime);
//		sendCommand(text);
		sendCommand("timer=dimTime");
		sendCommand("tm0.en=1");



	} else {
		m_receive = true;
	//gotoPage1 already sets timer=dimTime and starts the timer
		gotoPage1();
		m_source  = source;
	//set colours for active reception
		sendCommand("CALLSIGNS.pco=WHITE");
		sendCommand("TEXT.pco=YELLOW");
		sendCommand("SOURCE.pco=BLACK");
		char text[100U];
		::sprintf(text, "SOURCE.txt=\"%s > %s\"", source.c_str(), destination.c_str());
		sendCommand(text);
		sendCommand("RX.txt=\"RX\"");
		sendCommand("tm0.en=0");

	}
}

void CM17TS::showText()
{
	char text[100U];
	::sprintf(text, "TEXT.txt=\"%s\"", m_text.c_str());

	sendCommand(text);
}

void CM17TS::showCallsigns()
{
	char text[100U];
	::sprintf(text, "CALLSIGNS.txt=\"%s\"", m_callsigns.c_str());

	sendCommand(text);
}

void CM17TS::showRSSI(int rssi)
{
	m_sMeter = 0U;

	unsigned int dBm = std::abs(rssi);

	if (dBm < RSSI_BASE) {
		m_sMeter = RSSI_BASE - rssi;

		if (m_sMeter > 100U)
			m_sMeter = 100U;
	}

	if (m_page == 1U) {
		char text[100U];
		::sprintf(text, "S_METER.val=%u", m_sMeter);
		sendCommand(text);
	}
}

void CM17TS::showGPS(float latitude, float longitude, const std::string& locator,
	const std::optional<float>& altitude,
	const std::optional<float>& speed, const std::optional<float>& track,
	const std::optional<float>& bearing, const std::optional<float>& distance)
{
	sendCommand("page GPS");

	char text[100U];

	if (latitude < 0.0F)
		::sprintf(text, "LATITUDE.txt=\"%.3f\xB0 S\"", -latitude);
	else
		::sprintf(text, "LATITUDE.txt=\"%.3f\xB0 N\"", latitude);
	sendCommand(text);

	if (longitude < 0.0F)
		::sprintf(text, "LONGITUDE.txt=\"%.3f\xB0 W\"", -longitude);
	else
		::sprintf(text, "LONGITUDE.txt=\"%.3f\xB0 E\"", longitude);
	sendCommand(text);

	::sprintf(text, "LOCATOR.txt=\"%s\"", locator.c_str());
	sendCommand(text);

	if (altitude) {
		if (m_metric)
			::sprintf(text, "ALTITUDE.txt=\"%.1f m\"", altitude.value());
		else
			::sprintf(text, "ALTITUDE.txt=\"%.1f ft\"", altitude.value() * 3.28F);

		sendCommand(text);
	}

	if (speed && track) {
		if (m_metric)
			::sprintf(text, "SPEED.txt=\"%.1f km/h\"", speed.value());
		else
			::sprintf(text, "SPEED.txt=\"%.1f mph\"", speed.value() / 1.602F);

		sendCommand(text);

	//Nextion gauge element goes from 0 to 360. 0 degrees from GPS=value 90 for gauge
		float gauge = track.value() + 90;
		if (gauge>360){gauge=gauge-360;}
		::sprintf(text, "TRACK.txt=\"%.0f\"", gauge);
		sendCommand(text);
	}

	if (bearing && distance) {
		float gauge = bearing.value() + 90;
		if (gauge>360){gauge=gauge-360;}
		::sprintf(text, "BEARING.txt=\"%.0f\"", gauge);
		sendCommand(text);

		if (m_metric)
			::sprintf(text, "DISTANCE.txt=\"%.0f km\"", distance.value());
		else
			::sprintf(text, "DISTANCE.txt=\"%.0f miles\"", distance.value() / 1.602F);

		sendCommand(text);

//		drawPointer(bearing.value());
	}

	sendCommand("delay=5000");

sendCommand ("page page1");
}

void CM17TS::drawPointer(float bearing)
{
	char text[100U];

	// Draw the circle
	::sprintf(text, "cir %d,%d,%d,WHITE", COMPASS_X, COMPASS_Y, COMPASS_R + 10);
	sendCommand(text);

	// Print the "N"
	::sprintf(text, "xstr %d,%d,30,30,3,WHITE,BLACK,1,1,1,\"N\"", COMPASS_X - 15, COMPASS_Y - COMPASS_R - 20);
	sendCommand(text);

	// Draw the lines
	bearing -= 90.0F;

	float degrees = bearing;
	float radians = DEG2RAD(degrees);
	int p1x = COMPASS_X + COMPASS_R * ::cos(radians);
	int p1y = COMPASS_Y + COMPASS_R * ::sin(radians);

	degrees = bearing + 145.0F;
	radians = DEG2RAD(degrees);
	int p2x = COMPASS_X + COMPASS_R * ::cos(radians);
	int p2y = COMPASS_Y + COMPASS_R * ::sin(radians);

	degrees = bearing + 180.0F;
	radians = DEG2RAD(degrees);
	int p3x = COMPASS_X + (COMPASS_R / 2) * ::cos(radians);
	int p3y = COMPASS_Y + (COMPASS_R / 2) * ::sin(radians);

	degrees = bearing - 145.0F;
	radians = DEG2RAD(degrees);
	int p4x = COMPASS_X + COMPASS_R * ::cos(radians);
	int p4y = COMPASS_Y + COMPASS_R * ::sin(radians);

	::sprintf(text, "line %d,%d,%d,%d,YELLOW", p1x, p1y, p2x, p2y);
	sendCommand(text);

	::sprintf(text, "line %d,%d,%d,%d,YELLOW", p2x, p2y, p3x, p3y);
	sendCommand(text);

	::sprintf(text, "line %d,%d,%d,%d,YELLOW", p3x, p3y, p4x, p4y);
	sendCommand(text);

	::sprintf(text, "line %d,%d,%d,%d,YELLOW", p4x, p4y, p1x, p1y);
	sendCommand(text);
}

void CM17TS::gotoPage0()
{
	sendCommand("page page0");
	m_page = 0U;

	char text[100U];

	::sprintf(text, "VOLUME.val=%u", m_volume);
	sendCommand(text);

	if (!m_destinations.empty()) {
		::sprintf(text, "CHANNEL.txt=\"%s\"", m_channels.at(m_channelIdx).c_str());
		sendCommand(text);

		::sprintf(text, "DESTINATION.txt=\"%s\"", m_destinations.at(m_destinationIdx).c_str());
		sendCommand(text);
	}
}

void CM17TS::gotoPage1()
{
	sendCommand("page page1");
	m_page = 1U;

	char text[100U];

	::sprintf(text, "timer=%u", m_dimTime);
	sendCommand(text);
//			::sprintf(text, "dimLevel=%u", m_dimLevel);
//		sendCommand(text);
//				::sprintf(text, "dimTime=%u", m_dimTime);
//		sendCommand(text);

	if (!m_destinations.empty()) {
		::sprintf(text, "CHANNEL.txt=\"%s\"", m_channels.at(m_channelIdx).c_str());
		sendCommand(text);

		::sprintf(text, "DESTINATION.txt=\"%s\"", m_destinations.at(m_destinationIdx).c_str());
		sendCommand(text);
	}

	::sprintf(text, "SOURCE.txt=\"%s\"", m_source.c_str());
	sendCommand(text);

	::sprintf(text, "CALLSIGNS.txt=\"%s\"", m_callsigns.c_str());
	sendCommand(text);

	::sprintf(text, "TEXT.txt=\"%s\"", m_text.c_str());
	sendCommand(text);
	


	if (m_receive)
		sendCommand("RX.txt=\"RX\"");
	else
		sendCommand("RX.txt=\"\"");

	if (m_transmit)
		sendCommand("TX.txt=\"TX\"");
	else
		sendCommand("TX.txt=\"\"");

	::sprintf(text, "S_METER.val=%u", m_sMeter);
	sendCommand(text);
}

bool CM17TS::getChannels()
{
	assert(m_socket != NULL);

	char buffer[20U];
	::strcpy(buffer, "CHAN");
	::strcat(buffer, DELIMITER);
	::strcat(buffer, "?");

	return m_socket->write(buffer, ::strlen(buffer), m_sockaddr, m_sockaddrLen);
}

bool CM17TS::setChannel(const std::string& channel)
{
	assert(m_socket != NULL);

	char buffer[20U];
	::strcpy(buffer, "CHAN");
	::strcat(buffer, DELIMITER);
	::strcat(buffer, channel.c_str());

	return m_socket->write(buffer, ::strlen(buffer), m_sockaddr, m_sockaddrLen);
}

bool CM17TS::getDestinations()
{
	assert(m_socket != NULL);

	char buffer[20U];
	::strcpy(buffer, "DEST");
	::strcat(buffer, DELIMITER);
	::strcat(buffer, "?");

	return m_socket->write(buffer, ::strlen(buffer), m_sockaddr, m_sockaddrLen);
}

bool CM17TS::setDestination(const std::string& destination)
{
	assert(m_socket != NULL);

	char buffer[20U];
	::strcpy(buffer, "DEST");
	::strcat(buffer, DELIMITER);
	::strcat(buffer, destination.c_str());

	return m_socket->write(buffer, ::strlen(buffer), m_sockaddr, m_sockaddrLen);
}

bool CM17TS::setVolume(unsigned int volume)
{
	assert(m_socket != NULL);

	m_conf.setVolume(volume);
	m_conf.write();

	char buffer[20U];
	::strcpy(buffer, "VOL");
	::strcat(buffer, DELIMITER);
	::sprintf(buffer + ::strlen(buffer), "%u", volume);

	return m_socket->write(buffer, ::strlen(buffer), m_sockaddr, m_sockaddrLen);
}

bool CM17TS::setTransmit(bool transmit)
{
	assert(m_socket != NULL);

	char buffer[20U];
	::strcpy(buffer, "TX");
	::strcat(buffer, DELIMITER);
	::strcat(buffer, transmit ? "1" : "0");

	return m_socket->write(buffer, ::strlen(buffer), m_sockaddr, m_sockaddrLen);
}

void CM17TS::sendCommand(const char* command)
{
	assert(command != NULL);
	assert(m_uart != NULL);

	m_uart->write(command, ::strlen(command));
	m_uart->write("\xFF\xFF\xFF", 3U);
}

void CM17TS::selectChannel()
{
	m_channelIdx = 0xFFFFU;

	unsigned int n = 0U;
	for (const auto& it : m_channels) {
		if (it == m_conf.getChannel()) {
			m_channelIdx = n;
			break;
		}

		n++;
	}

	if (m_channelIdx == 0xFFFFU)
		m_channelIdx = 0U;

	std::string channel = m_channels.at(m_channelIdx);

	char text[100U];
	::sprintf(text, "CHANNEL.txt=\"%s\"", channel.c_str());
	sendCommand(text);

	setChannel(channel);
}

void CM17TS::selectDestination()
{
	m_destinationIdx = 0xFFFFU;

	unsigned int n = 0U;
	for (const auto& it : m_destinations) {
		if (it == m_conf.getDestination()) {
			m_destinationIdx = n;
			break;
		}

		n++;
	}

	if (m_destinationIdx == 0xFFFFU)
		m_destinationIdx = 0U;

	std::string destination = m_destinations.at(m_destinationIdx);

	char text[100U];
	::sprintf(text, "DESTINATION.txt=\"%s\"", destination.c_str());
	sendCommand(text);

	setDestination(destination);
}
