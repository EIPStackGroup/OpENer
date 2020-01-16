/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "generic_networkhandler.h"
#include "opener_api.h"
#include "cipcommon.h"
#include "trace.h"
#include "networkconfig.h"
#include "nvdata.h"

extern int newfd;

/******************************************************************************/
/*!\brief Signal handler function for ending stack execution
 *
 * @param pa_nSig the signal we received
 */
void
LeaveStack(int pa_nSig);

/*****************************************************************************/
/*! \brief Flag indicating if the stack should end its execution
 */
int g_end_stack = 0;

/******************************************************************************/
int main(int argc, char *arg[]) {

  if (argc != 2) {
    printf("Wrong number of command line parameters!\n");
    printf("The correct command line parameters are:\n");
    printf(
      "./OpENer index\n");
    printf(
      "    e.g. ./OpENer index\n");
    exit(0);
  }
  DoublyLinkedListInitialize(&connection_list,
                             CipConnectionObjectListArrayAllocator,
                             CipConnectionObjectListArrayFree);
  /* Fetch MAC address from the platform. This tests also if the interface is present. */
  uint8_t iface_mac[6];
  if (kEipStatusError == IfaceGetMacAddress(arg[1], iface_mac)) {
    printf("Network interface %s not found.\n", arg[1]);
    exit(EXIT_FAILURE);
  }

  /*for a real device the serial number should be unique per device */
  SetDeviceSerialNumber(123456789);

  /* unique_connection_id should be sufficiently random or incremented and stored
   * in non-volatile memory each time the device boots.
   */
  EipUint16 unique_connection_id = rand();

  /* Setup the CIP Layer */
  CipStackInit(unique_connection_id);

  /* The CIP objects are now created and initialized with their default values.
   *  Now any NV data values are loaded to change the data to the stored
   *  configuration.
   */
  if (kEipStatusError == NvdataLoad()) {
    OPENER_TRACE_WARN("Loading of some NV data failed. Maybe the first start?\n");
  }

  /* Setup Network Handles */
  if ( kEipStatusOk == NetworkHandlerInitialize() ) {
    g_end_stack = 0;
#ifndef WIN32
    /* register for closing signals so that we can trigger the stack to end */
    signal(SIGHUP, LeaveStack);
    signal(SIGINT, LeaveStack); /* needed to be able to abort with ^C */
#endif

    /* The event loop. Put other processing you need done continually in here */
    while (1 != g_end_stack) {
      if ( kEipStatusOk != NetworkHandlerProcessOnce() ) {
        break;
      }
    }

    /* clean up network state */
    NetworkHandlerFinish();
  }
  /* close remaining sessions and connections, cleanup used data */
  ShutdownCipStack();

  return -1;
}

void LeaveStack(int pa_nSig) {
  (void) pa_nSig; /* kill unused parameter warning */
  OPENER_TRACE_STATE("got signal %d\n",pa_nSig);
  g_end_stack = 1;
}
