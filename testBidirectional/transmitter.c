#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/wpan.h"
#include "platform_config.h"

uint32_t BAUD_RATE = 115200;
char SERIAL_DEVICE_ID[] = "/dev/ttyS0";
int const MAX_PAYLOAD_SIZE = 100;
char TEST_MESSAGES_SRC[] = "random_text.txt";

// Local Functions
xbee_serial_t init_serial();
static void sigterm(int sig);
int tx_status_handler(xbee_dev_t *xbee, const void FAR *raw,
                      uint16_t length, void FAR *context);
static int receive_handler(xbee_dev_t *xbee, const void FAR *raw,
                           uint16_t length, void FAR *context);

// Shared Variables. NOTE: There are better receive handlers in wpan.h
static volatile sig_atomic_t terminationflag = 0;
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    {XBEE_FRAME_RECEIVE_EXPLICIT, 0, receive_handler, NULL},
    {XBEE_FRAME_TRANSMIT_STATUS, 0, tx_status_handler, NULL},
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_TABLE_END};

int main(int argc, char **argv)
{
  int err;
  xbee_serial_t serial = init_serial();
  xbee_dev_t my_xbee;

  // Dump state to stdout for debug
  err = xbee_dev_init(&my_xbee, &serial, NULL, NULL);
  if (err)
  {
    printf("Error initializing abstraction: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }
  printf("Initialized XBee device abstraction.\n");
  
  
  // Need to initialize AT layer so we can transmit
  err = xbee_cmd_init_device(&my_xbee);
  if (err)
  {
    printf("Error initializing AT layer: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }
  
  printf( "Waiting for driver to query the XBee device...\n");
  do {
    xbee_dev_tick( &my_xbee);
    err = xbee_cmd_query_status( &my_xbee);
  } while (err == -EBUSY);
  if (err)
  {
    printf( "Error %d waiting for query to complete.\n", err);
  }
  
  printf("Initialized XBee AT layer...\n");
  xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

  // Create graceful SIGINT handler
  if (signal(SIGTERM, sigterm) == SIG_ERR || signal(SIGINT, sigterm) == SIG_ERR)
  {
    printf("Error setting signal handler\n");
    return EXIT_FAILURE;
  }

  // Every outbound message is a broadcast frame
  xbee_header_transmit_explicit_t frame_out_header = {
      .frame_type = XBEE_FRAME_TRANSMIT_EXPLICIT,
      .frame_id = 0,
      .ieee_address = *WPAN_IEEE_ADDR_BROADCAST,
      .network_address_be = 0xFFFE,
      .source_endpoint = WPAN_ENDPOINT_DIGI_DATA,
      .dest_endpoint = WPAN_ENDPOINT_DIGI_DATA,
      .cluster_id_be = DIGI_CLUST_SERIAL,
      .profile_id_be = WPAN_PROFILE_DIGI,
      .broadcast_radius = 0x0,
      .options = 0x0,
  };

  // Arbitrary messages
  FILE *messages = fopen(TEST_MESSAGES_SRC, "r");
  if (messages == NULL)
  {
    printf("Error opening %s\n", TEST_MESSAGES_SRC);
    return EXIT_FAILURE;
  }

  // Send messages & tick XBEE
  int frame_id = 0;
  char payload[MAX_PAYLOAD_SIZE];
  while (fgets(payload, MAX_PAYLOAD_SIZE, messages) != NULL)
  {
    // Send the next message, but don't forget to update the header
    frame_id++;
    printf("Sending message number: %d\n", frame_id);
    frame_out_header.frame_id = frame_id;
    err = xbee_frame_write(&my_xbee, &frame_out_header,
                           sizeof frame_out_header, &payload, sizeof payload, 0);
    if (err < 0)
    {
      printf("Error writing frame: %" PRIsFAR "\n", strerror(-err));
    }

    usleep(100000);
    // Tick to get TX status updates
    err = xbee_dev_tick(&my_xbee);
    if (terminationflag)
    {
      printf("Recieved SIGINT while waiting ticking device. Exiting\n");
      return EXIT_FAILURE;
    }
    if (err < 0)
    {
      printf("ERROR: Could not tick device: %" PRIsFAR "\n", strerror(-err));
      return EXIT_FAILURE;
    }
  }

  // Cleanup
  usleep(1000000);
  fclose(messages);
  err = xbee_dev_tick(&my_xbee);
  if (err < 0)
  {
    printf("ERROR: Could not tick device: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }

  // Summary
  printf("\n");
  printf("Could not read more from message source.\n");
  printf("Sent %d messages!\n", frame_id);
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

int tx_status_handler(xbee_dev_t *xbee,
                      const void FAR *raw, uint16_t length, void FAR *context)
{
  const xbee_frame_transmit_status_t FAR *frame = raw;
  XBEE_UNUSED_PARAMETER(xbee);
  XBEE_UNUSED_PARAMETER(length);
  XBEE_UNUSED_PARAMETER(context);
  printf("TX Status: id %d, delivery=0x%02x\n", frame->frame_id, frame->delivery);
  return 0;
}

static void sigterm(int sig)
{
  signal(sig, sigterm);
  terminationflag = 1;
}

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