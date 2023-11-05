// Goal: Parse this down! We want an IPV4 free verison

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/wpan.h"
#include "parse_serial_args.h"
#include "../common/_atinter.h"
#include "../../ports/posix/platform_config.h"

uint32_t BAUD_RATE = 115200;
char SERIAL_DEVICE_ID[] = "/dev/ttyS0";
int const MAX_PAYLOAD_SIZE = 100;

static volatile sig_atomic_t terminationflag = 0;
static int receive_handler(xbee_dev_t *xbee, const void FAR *frame,
                           uint16_t length, void FAR *context);

// There are more robust receive handlers defined in wpan.h
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    {XBEE_FRAME_RECEIVE_EXPLICIT, 0, receive_handler, NULL}, 
    XBEE_FRAME_TRANSMIT_STATUS_DEBUG,
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_TABLE_END};

xbee_serial_t init_serial()
{
    // We want to start with a clean slate
    xbee_serial_t serial;
    memset(&serial, 0, sizeof serial);

    // Set the baudrate and device ID. 
    serial.baudrate = BAUD_RATE;
    strncpy(serial.device, SERIAL_DEVICE_ID, (sizeof serial.device));
    return serial;
}

int main(int argc, char **argv)
{
    int err;
    xbee_serial_t serial = init_serial();
    xbee_dev_t my_xbee;

    // Dump state to stdout for debug
    err = xbee_dev_init(&my_xbee, &serial, NULL, NULL);
    if (err)
    {
        printf("Error initializing device: %" PRIsFAR "\n", strerror(-err));
        return EXIT_FAILURE;
    }
    printf("Initialized XBee device abstraction:\n");
    xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT); 
    
    char payload[] = "First payload!\r\n";
    xbee_header_transmit_explicit_t frame_out_header = {
        // Static in all our frames 
        .frame_type = XBEE_FRAME_TRANSMIT_EXPLICIT,
        .frame_id = 1,
        .ieee_address = *WPAN_IEEE_ADDR_BROADCAST,
        .network_address_be = 0xFFFE, // Possibly wrong. Reserved?
        .source_endpoint = WPAN_ENDPOINT_DIGI_DATA, 
        .dest_endpoint = WPAN_ENDPOINT_DIGI_DATA, 
        .cluster_id_be = DIGI_CLUST_SERIAL,
        .profile_id_be = WPAN_PROFILE_DIGI,
        .broadcast_radius = 0x0,
        .options = 0x0, 
    };

    // Write out the header & payload
    printf("Writing frame to XBee...\n");
    err = xbee_frame_write(&my_xbee, &frame_out_header, 
                sizeof frame_out_header, &payload, sizeof payload, 0);
                
    if (err < 0)
    {
        printf("Error writing frame: %" PRIsFAR "\n", strerror(-err));
        return EXIT_FAILURE;
    }
    
    printf("Successfully wrote frame! Ticking XBee to get TX status...\n");

}

static int receive_handler(xbee_dev_t *xbee, const void FAR *raw,
                           uint16_t frame_len, void FAR *context)
{
    const xbee_frame_receive_explicit_t FAR *frame_in = raw;
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(frame_len);
    XBEE_UNUSED_PARAMETER(frame_in);
    XBEE_UNUSED_PARAMETER(context); // Will never use context
    return 0;
}