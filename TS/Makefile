CC      = cc
CXX     = c++
CFLAGS  = -g -O3 -Wall -std=c++17 -pthread
LIBS    = -lpthread -lutil
LDFLAGS = -g

OBJECTS = 	Conf.o Log.o M17TS.o Thread.o Timer.o UARTController.o UDPSocket.o Utils.o
		
all:		M17TS

M17TS:		GitVersion.h $(OBJECTS) 
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o M17TS

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:	all
		install -m 755 M17TS /usr/local/bin/

clean:
		$(RM) M17TS *.o *.bak *~ GitVersion.h

GitVersion.h:
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@

