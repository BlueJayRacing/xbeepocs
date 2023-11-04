// Goal: Parse this down! We want an IPV4 free verison

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/ipv4.h"
#include "xbee/tx_status.h"
#include "xbee/wpan.h"
#include "parse_serial_args.h"
#include "_xbee_term.h"
#include "_atinter.h"

// TODO: I added this struct def
typedef struct xbee_serial_t
{
    uint32_t baudrate;
    int fd;
    char device[40]; // /dev/ttySxx
} xbee_serial_t;
// END TODO

static volatile sig_atomic_t terminationflag = 0;
static void sigterm(int sig);

static int receive_handler(xbee_dev_t *xbee, const void FAR *frame,
                           uint16_t length, void FAR *context);
static int tx_status_handler(xbee_dev_t *xbee, const void FAR *frame,
                             uint16_t length, void FAR *context);


// There are more robust receive handlers defined in wpan.h
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    {XBEE_FRAME_RECEIVE_EXPLICIT, 0, receive_handler, NULL}, 
    XBEE_FRAME_TRANSMIT_STATUS_DEBUG,
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_TABLE_END};

xbee_serial_t init_serial()
{
    // Our constants
    uint32_t BAUD_RATE = 115200;
    char SOME_DEVICE_ID[40] = {'\0'}; // TODO: Wtf what the device?
    
    // We want to start with a clean slate
    xbee_serial_t serial;
    memset(&serial, 0, sizeof serial);

    // Set the baudrate and device ID. 
    // TODO: In the parse_serial_args function, they look for a /dev/* line
    // to "determine what serial port to use"  
    // Q: Shuould there be two serial ports?
    serial.baudrate = BAUD_RATE;
    strncpy(serial.device, SOME_DEVICE_ID, (sizeof serial.device) - 1);
    serial.device[(sizeof serial.device) - 1] = '\0';
}

int main(int argc, char **argv)
{
    int err;
    int const MAX_PAYLOAD_SIZE = 100;
    xbee_serial_t serial = init_serial();
    xbee_dev_t my_xbee;

    err = xbee_dev_init(&my_xbee, &serial, NULL, NULL);
    if (err)
    {
        printf("Error initializing device: %" PRIsFAR "\n", strerror(-err));
        return EXIT_FAILURE;
    }


    // I think we can remove the cmd_init block
    err = xbee_cmd_init_device(&my_xbee);
    if (!err)
        err = -EBUSY;
    while (err == -EBUSY)
    {
        xbee_dev_tick(&my_xbee);
        err = xbee_cmd_query_status(&my_xbee);
    }
    if (err)
    {
        printf("Error initializing AT command layer: %" PRIsFAR "\n",
               strerror(-err));
        return EXIT_FAILURE;
    }
    // -- END CMD INIT BLOCK
    

    // Dump state to stdout for debug
    xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT); 

    // Create graceful SIGINT handler
    if (signal(SIGTERM, sigterm) == SIG_ERR || signal(SIGINT, sigterm) == SIG_ERR)
    {
        printf("Error setting signal handler\n");
        return EXIT_FAILURE;
    }

    uint8_t frame_id = 1;
    char payload[MAX_PAYLOAD_SIZE + 1]; // xbee_readline null terminates
    for (;;)
    {
        while (!terminationflag && err == -EAGAIN) 
        {
            // Collect any recieved frames, then collect user input
            err = xbee_dev_tick(&my_xbee);
            if (err >= 0)
            {
                // Collect the user input, leave space for appending returns
                err = xbee_readline(payload, MAX_PAYLOAD_SIZE - 2);
            }
        }
        
        // It's possible that SIGINT, EOF, or permamenent error
        if (terminationflag || err == -ENODATA)
            return EXIT_SUCCESS;
        if (err < 0)
        {
            printf("Error reading input: %" PRIsFAR "\n", strerror(-err));
            return EXIT_FAILURE;
        }
        
         
        strcpy(payload + strlen(payload), "\r\n");
        xbee_header_transmit_explicit_t frame_out_header = {
            // Static in all our frames 
            .frame_type = XBEE_FRAME_TRANSMIT_EXPLICIT,
            .frame_id = frame_id,
            .ieee_address = WPAN_IEEE_ADDR_BROADCAST,
            .network_address_be = 0xFFFE, // Possibly wrong. Reserved?
            .source_endpoint = WPAN_ENDPOINT_DIGI_DATA, 
            .dest_endpoint = WPAN_ENDPOINT_DIGI_DATA, 
            .cluster_id_be = DIGI_CLUST_SERIAL,
            .profile_id_be = WPAN_PROFILE_DIGI,
            .broadcast_radius = 0x0,
            .options = 0x0, 
        };

        // Write out the header & payload
        err = xbee_frame_write(&my_xbee, &frame_out_header, 
                    sizeof frame_out_header, &payload, sizeof payload, 0);
                    
        if (err < 0)
        {
            printf("Error writing frame: %" PRIsFAR "\n", strerror(-err));
            return EXIT_FAILURE;
        }
        frame_id++;
    }
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

static void sigterm(int sig)
{
    signal(sig, sigterm);
    terminationflag = 1;
}
