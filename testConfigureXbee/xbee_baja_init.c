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

/** Write Baja XBee standard firmware settins to the XBee.
 *  to the Baja standard. This function will return none
 *  zero if the XBee does not conform to the standard.
 */
int _write_baja_settings(xbee_dev_t *xbee)
{
  int err;
  // TODO: set channel mask (CM), which doesn't have a 32-bit value?

  
  // A struct for storing the command title and it's Baja value
  // Create an array of integer-valued commands to send to the XBee
  // TODO: move this into settings header
  struct cmd {
    char* name;
    int value;
  };
  struct cmd baja_cmds[] = {
    {"HP", XBEE_BAJA_HP},
    {"BD", XBEE_BAJA_BD},
    {"TX", XBEE_BAJA_TX},
    {"BR", XBEE_BAJA_BR},
    {"AP", XBEE_BAJA_AP},
    {"ID", XBEE_BAJA_ID},
    {"MT", XBEE_BAJA_MT},
    {"NP", XBEE_BAJA_NP},
  };

  for (int i = 0; i < sizeof baja_cmds / sizeof baja_cmds[0]; i++) {
    do {
      err = xbee_cmd_simple(xbee, baja_cmds[i].name, baja_cmds[i].value);
    } while (err == -EBUSY);
    if (err == -EINVAL) {
      printf("Error sending %s command. Invalid parameter\n", baja_cmds[i].name);
      return EXIT_FAILURE;
    }
    printf("Set %s to %d\n", baja_cmds[i].name, baja_cmds[i].value);
  }
  xbee_cmd_execute(xbee, "WR", NULL, 0);
  return EXIT_SUCCESS;
}


/**
 * Initialize an XBee with the Baja standard settings
*/
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

  err = _write_baja_settings(xbee);
  if (err)
  {
    printf("Xbee settings were not verified\n");
    return EXIT_FAILURE;
  }
  xbee_dev_tick(xbee);
  printf("XBee settings conform to Baja standards\n\n");
  return EXIT_SUCCESS;
}