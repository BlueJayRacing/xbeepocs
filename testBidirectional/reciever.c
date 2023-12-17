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

#define MAX_PAYLOAD_SIZE 100
#define NUM_EXPECTED_MESSAGES 50
#define BAUD_RATE 115200
#define SERIAL_DEVICE_ID "/dev/ttyS0"
#define TEST_MESSAGES_SRC "random_text.txt"

// Local Functions
xbee_serial_t init_serial();
static void sigterm(int sig);
static int get_expected_messages(char *messages[], int max_num_messages);
int tx_status_handler(xbee_dev_t *xbee, const void FAR *raw,
                      uint16_t length, void FAR *context);
static int receive_handler(xbee_dev_t *xbee, const void FAR *raw,
                           uint16_t length, void FAR *context);

// Shared Variables. NOTE: There are better receive handlers in wpan.h
int num_msgs_rx = 0;
char* recieved_msgs[NUM_EXPECTED_MESSAGES] = {NULL};
char* expected_msgs[NUM_EXPECTED_MESSAGES] = {NULL};
static volatile sig_atomic_t terminationflag = 0;
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    {XBEE_FRAME_RECEIVE, 0, receive_handler, NULL},
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_TABLE_END};

int main(int argc, char **argv)
{
  xbee_serial_t serial = init_serial();
  xbee_dev_t my_xbee;

  // Dump state to stdout for debug
  int err = xbee_dev_init(&my_xbee, &serial, NULL, NULL);
  if (err)
  {
    printf("Error initializing abstraction: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }
  printf("Initialized XBee device abstraction...\n");
  
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
  printf("Set SIGINT handler\n");

  // Get array of expected messages
  err = get_expected_messages(expected_msgs, NUM_EXPECTED_MESSAGES);
  if (err != 0)
  {
    printf("Could not read all expected messages from file source");
    return EXIT_FAILURE;
  }
  printf("Loaded expected messages from file\n");
  
  // Tick Xbee to search for recieved messages
  printf("Beggining to wait & tick for messages...\n");
  while (num_msgs_rx < NUM_EXPECTED_MESSAGES)
  {
    usleep(10000);
    // Tick to get TX status updates
    err = xbee_dev_tick(&my_xbee);
    if (err > 0) {
      printf("Tick returned %d\n", err);
    }
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

  // Clean up the device
  usleep(1000000);
  err = xbee_dev_tick(&my_xbee);
  if (err < 0)
  {
    printf("ERROR: Could not tick device: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }

  // Summary
  printf("\nExpected number of messages: %d\n", NUM_EXPECTED_MESSAGES);
  printf("Number of messages recieved: %d\n\n", num_msgs_rx);

  // Cleanup Allocations
  for (int i = 0; i < NUM_EXPECTED_MESSAGES; i++)
  {
    if (expected_msgs[i] != NULL) {
      free(expected_msgs[i]);
    }
    if (recieved_msgs[i]) {
      free(recieved_msgs[i]);
    }
  }
  printf("Finished clean up\n");
}

static int receive_handler(xbee_dev_t *xbee, const void FAR *raw,
                           uint16_t frame_len, void FAR *context)
{
  
  XBEE_UNUSED_PARAMETER( context);
  const xbee_frame_receive_t FAR *frame = raw;
  
  if (frame == NULL)
  {
    printf("Recieved null frame\n");
    return -EINVAL;
  }
  if (frame_len < offsetof( xbee_frame_receive_t, payload))
  {
    printf("Recieved frame too short\n");
    return -EBADMSG;
  }
  
  if (!(frame->options & XBEE_RX_OPT_BROADCAST))
  {
    printf("Recieved non-broadcast frame\n");
    return -EBADMSG;
  }

  int payload_len = frame_len - offsetof(xbee_frame_receive_t, payload);
  // if (frame->payload[payload_len-1] != '\0') {
  //   printf("Recieved message not null terminated\n");
  //   return -EBADMSG;
  // }
  
  recieved_msgs[num_msgs_rx] = malloc((payload_len) * sizeof(char));
  if (recieved_msgs[num_msgs_rx] == NULL) {
    printf("Could not allocate memory for recieved message\n");
    return -ENOMEM;
  }
  
  strncpy(recieved_msgs[num_msgs_rx], (char *) frame->payload, payload_len);
  num_msgs_rx++;
  printf("Recieved message #%d\n", num_msgs_rx);
  return EXIT_SUCCESS;
}

static int get_expected_messages(char *messages[], int max_num_messages)
{
  // Read up to max_num_messages, returns a zero on failure. Fill messages 
  // with pointers to the strings read from the file.
  FILE *msg_file = fopen(TEST_MESSAGES_SRC, "r");
  if (msg_file == NULL)
  {
    printf("Error opening %s\n", TEST_MESSAGES_SRC);
    return EXIT_FAILURE;
  }

  // Dyn allocate an array of char* with length num_msgs
  for (int i = 0; i < max_num_messages; i++)
  {
    char msg[MAX_PAYLOAD_SIZE];
    if (fgets(msg, MAX_PAYLOAD_SIZE, msg_file) == NULL)
    {
      return EXIT_FAILURE;
    }
    messages[i] = malloc(strlen(msg) * sizeof(char));
    strcpy(messages[i], msg);
  }
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