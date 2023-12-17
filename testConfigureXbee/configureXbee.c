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

// Local Functions
xbee_serial_t init_serial();
static void sigterm(int sig);

// Global variable. Should define in main.c
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
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
  
  printf("Finished running initialization command\n");
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