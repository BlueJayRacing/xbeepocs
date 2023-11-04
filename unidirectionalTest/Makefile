# One make command for each POC

CC ?= gcc
PORT = posix
DEFINE = -DPOSIX
CFLAGS += -std=gnu99 -g -MMD -MP -Wall $(DEFINE)
include ../libraries/xbee_ansic_library/samples/common/common.mk


CPP = g++
CONSERVATIVE_FLAGS = -std=c++20 -Wall -Wextra -pedantic # Possible conflict that c++17 not gnu++17
DEBUGGING_FLAGS = -g -Ofast -pg
IGNORE_FLAGS = -Wno-psabi -Wvla
LINKING_FLAGS = -pthread -lm
CPPFLAGS = $(CONSERVATIVE_FLAGS) $(DEBUGGING_FLAGS) $(IGNORE_FLAGS) $(LINKING_FLAGS)


# Get the .d files
# Assemble the .o files for each dependency (-c flag)
# build the final main application (-o flag)

all: unidirectionalTestReceiver unidirectionalTestTransmitter

unidirectionalTestReceiver: unidirectionalTest/receiver.cpp
	$(CPP) $(CPPFLAGS) -o unidirectionalTest/receiver unidirectionalTest/receiver.cpp
	
unidirectionalTestTransmitter: unidirectionalTest/transmitter.cpp
	$(CPP) $(CPPFLAGS) -o unidirectionalTest/transmitter unidirectionalTest/transmitter.cpp

clean:
	rm -rf *.o *.gch *out *.d $(EXE)


