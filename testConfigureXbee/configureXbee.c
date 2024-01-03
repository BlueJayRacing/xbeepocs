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
#include "xbee_baja_init.h"

// Custom frame handlers
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

// Global variable. Should define in main.c
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    {XBEE_FRAME_TRANSMIT_STATUS, 0, tx_status_handler, NULL},
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_TABLE_END};

int main(int argc, char **argv)
{
  // Test the initialization function
  xbee_dev_t my_xbee;
  int err = init_baja_xbee(&my_xbee);
  if (err)
  {
    printf("Error initializing XBee with baja settings.\n");
    return EXIT_FAILURE;
  }

  // Send a broadcast message to make sure the XBee is working
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

  // Now we'd like to wait for the XBee's frame acknowledgement
  printf("Successfully wrote frame!\n\n");
  
  printf("Ticking XBee to get TX status...\n");
  while (1)
  {
      err = xbee_dev_tick(&my_xbee);
      if (err >= 1)
      {
          printf("Read a frame from the XBee!\n");
          return EXIT_SUCCESS;
      }
      if (err < 0)
      {
          printf("ERROR: Could not tick device: %" PRIsFAR "\n", strerror(-err));
          return EXIT_FAILURE;
      }
  }

  printf("Finished running initialization test\n");
}

