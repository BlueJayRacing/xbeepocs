#ifndef XBEE_INIT_FUNCS_H
#define XBEE_INIT_FUNCS_H

#include "xbee/device.h"

int init_baja_xbee(xbee_dev_t *xbee, const xbee_dispatch_table_entry_t* xbee_frame_handlers);

#endif
