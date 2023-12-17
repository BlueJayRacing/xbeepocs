# Autodepend methods from http://make.paulandlesley.org/autodep.html
CC ?= gcc

# Path to Digi XBee ANSI library
XBEE_LIBRARY_DIR = /home/pi/xbeepocs/libraries/xbee_ansic_library
PORT = posix

# Port and driver config
DRIVER = $(XBEE_LIBRARY_DIR)
PORTDIR = $(DRIVER)/ports/$(PORT)

# path to include and source files
INCDIR = $(DRIVER)/include
SRCDIR = $(DRIVER)/src

# Define macros for compilation on POSIX
DEFINE = \
	-DPOSIX \
	-DZCL_ENABLE_TIME_SERVER \
	-DXBEE_CELLULAR_ENABLED \
	-DXBEE_DEVICE_ENABLE_ATMODE \
	-DXBEE_XMODEM_TESTING \

# compiler parameters for building each file
# -MMD generates dependency files automatically, omitting system files
# -MP creates phony targets for each prerequisite in a .d file
CFLAGS += -I$(INCDIR) -I$(PORTDIR) -std=gnu99 -g -MMD -MP -Wall $(DEFINE)

EXE = \
	configureXbee

all : $(EXE)

# strip debug information from executables
strip :
	strip $(EXE)

SRCS = \
	$(wildcard $(SRCDIR)/*/*.c) \
	$(wildcard $(PORTDIR)/*.c) \
	$(wildcard $(DRIVER)/samples/$(PORT)/*.c) \
	$(wildcard $(DRIVER)/samples/common/*.c) \

# Dependency object files
base_OBJECTS = xbee_platform_$(PORT).o xbee_serial_$(PORT).o hexstrtobyte.o \
					memcheck.o swapbytes.o swapcpy.o hexdump.o
xbee_OBJECTS = $(base_OBJECTS) xbee_device.o xbee_atcmd.o wpan_types.o
wpan_OBJECTS = $(xbee_OBJECTS) wpan_aps.o xbee_wpan.o
zigbee_OBJECTS = $(wpan_OBJECTS) zigbee_zcl.o zigbee_zdo.o zcl_types.o

# The executables are the only explicit targets we need
transmitter : transmitter.o $(zigbee_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	
reciever : reciever.o $(zigbee_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Use the dependency files created by the -MD option to gcc.
-include $(SRCS:.c=.d)

# to build a .o file, use the .c file in the current dir...
.c.o :
	$(CC) $(CFLAGS) -c $<

# ...or in the port support directory...
%.o : $(PORTDIR)/%.c
	$(CC) $(CFLAGS) -c $<

# ...or in common samples directory...
%.o : ../common/%.c
	$(CC) $(CFLAGS) -c $<

# ...or in a subdirectory of SRCDIR...
%.o : $(SRCDIR)/*/%.c
	$(CC) $(CFLAGS) -c $<
	
# --- END INCLUDE MAKEFILE ---

clean :
	- rm -f *.o *.d $(EXE)

