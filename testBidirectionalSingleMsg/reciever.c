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
char TEST_MESSAGES_SRC[] = "random_text.txt";
int const MAX_PAYLOAD_SIZE = 100;
int const NUM_EXPECTED_MESSAGES = 50;

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
    printf("Error initializing device: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }
  printf("Initialized XBee device abstraction...\n");
  xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

  // Create graceful SIGINT handler
  if (signal(SIGTERM, sigterm) == SIG_ERR || signal(SIGINT, sigterm) == SIG_ERR)
  {
    printf("Error setting signal handler\n");
    return EXIT_FAILURE;
  }

  // Read in the expected messages, store as an array of pointers to null
  // terminated strings.
  FILE *msg_file = fopen(TEST_MESSAGES_SRC, "r");
  if (msg_file == NULL)
  {
    printf("Error opening %s\n", TEST_MESSAGES_SRC);
    return EXIT_FAILURE;
  }

  // Get array of expected messages
  char* expected_msgs[NUM_EXPECTED_MESSAGES];
  int err = get_expected_messages(expected_msgs,
                                             NUM_EXPECTED_MESSAGES, msg_file);
  if (err != NUM_EXPECTED_MESSAGES)
  {
    printf("Could not read all expected messages from file source");
    return EXIT_FAILURE;
  }
  
  // Tick Xbee to search for recieved messages
  char* recieved_msgs[NUM_EXPECTED_MESSAGES];
  int n_recieved = 0;
  char payload[MAX_PAYLOAD_SIZE + 1];
  while (length(n_recieved) < NUM_EXPECTED_MESSAGES)
  {
    usleep(10000);
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

  // Clean up the device
  usleep(1000000);
  err = xbee_dev_tick(&my_xbee);
  if (err < 0)
  {
    printf("ERROR: Could not tick device: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }

  // Summary
  // TODO: Print out number expected, number recieved

  // Cleanup
  for (int i = 0; i < NUM_EXPECTED_MESSAGES, i++)
  {
    if (expected_msgs[i] != NULL) {
      free(expected_msgs[i]);
    }
    if (recieved_msgs[i]) {
      free(recieved_msgs[i]);
    }
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

static int get_expected_messages(
    char *messages[], int max_num_messages, FILE *src)
{
  // Read up to max_num_messages, returning the actual number of messages
  // read from the file. Fill messages with pointers to the strings read
  // from the file.

  // Dyn allocate an array of char* with length num_msgs
  for (int i = 0; i < max_num_messages; i++)
  {
    char msg[MAX_PAYLOAD_SIZE + 1];
    if (fgets(msg, MAX_PAYLOAD_SIZE + 1, src) == NULL)
    {
      return i;
    }
    messages[i] = malloc(strlen(msg) + 1);
    strcpy(messages[i], msg);
  }
  return max_num_messages;
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