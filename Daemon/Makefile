# To use GPSD to transmit GPS data, add -DUSE_GPSD to the CFLAGS line
#
# To disable HamLib to control the radio, remove -DUSE_HAMLIB from the CFLAGS line and remove -lhamlib to the LIBS line
#
# To disable GPIO for PTT, remove -DUSE_GPIO from the CFLAGS line and remove -lgpiod to the LIBS line
#

CC      = cc
CXX     = c++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -DUSE_HAMLIB -DUSE_GPIO
LIBS    = -lpthread -lutil -lasound -lsamplerate -lhamblib -lgpiod
LDFLAGS = -g

OBJECTS = \
		codec2/codebooks.o codec2/codec2.o codec2/kiss_fft.o codec2/lpc.o codec2/nlp.o codec2/pack.o codec2/qbase.o \
		codec2/quantise.o CodePlug.o Conf.o Golay24128.o GPIO.o GPSD.o HamLib.o Log.o M17Client.o M17Convolution.o \
		M17CRC.o M17LSF.o M17RX.o M17TX.o M17Utils.o Modem.o ModemPort.o RSSIInterpolator.o SoundCard.o StopWatch.o \
		Thread.o Timer.o UARTController.o UDPSocket.o Utils.o
		
all:		M17Client

M17Client:	GitVersion.h $(OBJECTS) 
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o M17Client

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:	all
		install -m 755 M17Client /usr/local/bin/

clean:
		$(RM) M17Client codec2/*.o codec2/*.bak codec2/*~ *.o *.bak *~ GitVersion.h

GitVersion.h:
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@

