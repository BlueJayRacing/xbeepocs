# Shared Makefile rules for sample programs on Win32 and POSIX platforms

# Autodepend methods from http://make.paulandlesley.org/autodep.html

# If you get a "no rule to make target" error for some random .h file, try
# deleting all .d files.

# directory for driver
DRIVER = ../..

# directory for platform support
PORTDIR = $(DRIVER)/ports/$(PORT)

# path to include files
INCDIR = $(DRIVER)/include

# path to common source files
SRCDIR = $(DRIVER)/src

# extra defines (in addition to those in platform_config.h)
DEFINE += \
	-DZCL_ENABLE_TIME_SERVER \
	-DXBEE_CELLULAR_ENABLED \
	-DXBEE_DEVICE_ENABLE_ATMODE \
	-DXBEE_XMODEM_TESTING

#removed:
#	install_ebin \
	install_ebl \
	eblinfo \
	xbee_term \
	ipv4_client \
	xbee_netcat \
	zigbee_ota_info \
	zigbee_register_device \
	zigbee_walker \
	apply_profile \
	atinter \
	commissioning_client \
	commissioning_server \
	gpm \
	network_scan \
	remote_at \
	sms_client \
	socket_test \
	transparent_client \
	user_data_relay \
	xbee_ftp \
	xbee3_ota_tool \
	xbee3_secure_session \
	xbee3_srp_verifier \
	zcltime \

EXE += \
	my_transmitter \

all : $(EXE)

# strip debug information from executables
strip :
	strip $(EXE)

SRCS = \
	$(wildcard $(SRCDIR)/*/*.c) \
	$(wildcard $(PORTDIR)/*.c) \
	$(wildcard $(DRIVER)/samples/$(PORT)/*.c) \
	$(wildcard $(DRIVER)/samples/common/*.c) \

base_OBJECTS = xbee_platform_$(PORT).o xbee_serial_$(PORT).o hexstrtobyte.o \
					memcheck.o swapbytes.o swapcpy.o hexdump.o parse_serial_args.o

xbee_OBJECTS = $(base_OBJECTS) xbee_device.o xbee_atcmd.o wpan_types.o

wpan_OBJECTS = $(xbee_OBJECTS) wpan_aps.o xbee_wpan.o

zigbee_OBJECTS = $(wpan_OBJECTS) zigbee_zcl.o zigbee_zdo.o zcl_types.o


	
my_transmitter_OBJECTS = $(zigbee_OBJECTS) my_transmitter.o
my_transmitter : $(my_transmitter_OBJECTS)
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
