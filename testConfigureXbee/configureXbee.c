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
#include "xbee_init_funcs.h"

// Global variable. Should define in main.c
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_TABLE_END};

int main(int argc, char **argv)
{
  
  xbee_dev_t my_xbee;
  int err = init_baja_xbee();
  if (err)
  {
    printf("Error initializing XBee with baja settings.\n");
    return EXIT_FAILURE;
  }
  printf("Finished running initialization test\n");
  

}

