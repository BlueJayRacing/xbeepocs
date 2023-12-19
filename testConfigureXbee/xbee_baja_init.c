// This file should contain functions which allow main.c to
// create an xbee_dev_t object in one line. This will require
// some functions for
// 1. Initializing the serial port
// 2. Initializing the xbee_dev_t object
// 3. Initializing the AT layer
// 4. Confirming that the XBee has the correct setting

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/wpan.h"
#include "xbee_baja_config.h"
#include "serial_port_config.h"
#include "xbee_baja_init.h"
#include "platform_config.h"

xbee_serial_t _init_serial()
{
  // We want to start with a clean slate
  xbee_serial_t serial;
  memset(&serial, 0, sizeof serial);

  // Set the baudrate and device ID.
  serial.baudrate = XBEE_BAJA_BD;
  strncpy(serial.device, SERIAL_DEVICE_ID, (sizeof serial.device));
  return serial;
}

int _queue_cmd_requests(xbee_dev_t *xbee, int16_t *request) {
  // Queue up the AT commands to verify the settings. Return EXIT_FAILURE
  // if commands could not be queued up
  
  // while (1)
  // {
  //   xbee_cmd_set_command(request, "AF"); // change the cmd handle to create new command
  //   xbee_cmd_set_param_bytes(request, "???", length - 6); // Set handle paramteres
  //   error = xbee_cmd_send(request);
  //   if (error != 0)
  //   {
  //     printf("Error %d setting %s\n", error, "AF");
  //   }
  // }
  
  return 0;
}

int _verify_baja_settings(xbee_dev_t *xbee)
{
  /** Verify an XBee has firmware settings that conform
   *  to the Baja standard. This function will return none
   *  zero if the XBee does not conform to the standard.
   */
  int16_t* request;
  *request = xbee_cmd_create(xbee, "AF");
  if (*request >= 0)
  {
    xbee_cmd_set_flags(*request, XBEE_CMD_FLAG_QUEUE_CHANGE | XBEE_CMD_FLAG_REUSE_HANDLE);
    xbee_cmd_send(*request);
    
    // Qeueue the commands
    int err = _uncleanly_verify_settings(xbee, *request);
    if (err == EXIT_FAILURE) {
      // Do NOT write the queue of commands
      xbee_cmd_release_handle(*request);
      return EXIT_FAILURE; 
    } 
  }

  xbee_cmd_execute(xbee, "WR", NULL, 0);
  xbee_cmd_release_handle(*request);
  return EXIT_SUCCESS;
}

int init_baja_xbee(xbee_dev_t *xbee)
{
  xbee_serial_t serial = _init_serial();

  // Dump state to stdout for debug
  int err = xbee_dev_init(xbee, &serial, NULL, NULL);
  if (err)
  {
    printf("Error initializing abstraction: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }
  printf("Initialized XBee device abstraction.\n");

  // Need to initialize AT layer so we can transmit
  err = xbee_cmd_init_device(xbee);
  if (err)
  {
    printf("Error initializing AT layer: %" PRIsFAR "\n", strerror(-err));
    return EXIT_FAILURE;
  }
  do
  {
    xbee_dev_tick(xbee);
    err = xbee_cmd_query_status(xbee);
  } while (err == -EBUSY);
  if (err)
  {
    printf("Error %d waiting for AT init to complete.\n", err);
  }

  printf("Initialized XBee AT layer\n");
  xbee_dev_dump_settings(xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);
  printf("Veryfing XBee settings conform to Baja standard...\n");

  err = _safely_verify_baja_settings(xbee);
  if (err)
  {
    printf("Xbee settings were not verified\n");
    return EXIT_FAILURE;
  }

  printf("XBee settings conform to Baja standards\n\n");
  return EXIT_SUCCESS;
}