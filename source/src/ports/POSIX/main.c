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
#include "doublylinkedlist.h"
#include "cipconnectionobject.h"

/******************************************************************************/
/** @brief Signal handler function for ending stack execution
 *
 * @param signal the signal we received
 */
void LeaveStack(int signal);

/*****************************************************************************/
/** @brief Flag indicating if the stack should end its execution
 */
int g_end_stack = 0;

/******************************************************************************/
int main(int argc,
         char *arg[]) {
  EipUint16 unique_connection_id;

  if (argc != 2) {
    printf("Wrong number of command line parameters!\n");
    printf("The correct command line parameters are:\n");
    printf("./OpENer interfacename\n");
    printf("    e.g. ./OpENer eth1\n");
    exit(0);
  } else {
    DoublyLinkedListInitialize(&connection_list,
                               CipConnectionObjectListArrayAllocator,
                               CipConnectionObjectListArrayFree);
    /* fetch Internet address info from the platform */
    ConfigureNetworkInterface(arg[1]);
    ConfigureDomainName();
    ConfigureHostName();

    ConfigureMacAddress(arg[1]);
  }

  /*for a real device the serial number should be unique per device */
  SetDeviceSerialNumber(123456789);

  /* nUniqueConnectionID should be sufficiently random or incremented and stored
   *  in non-volatile memory each time the device boots.
   */
  unique_connection_id = rand();

  /* Setup the CIP Layer */
  CipStackInit(unique_connection_id);

  /* Setup Network Handles */
  if ( kEipStatusOk == NetworkHandlerInitialize() ) {
    g_end_stack = 0;
#ifndef WIN32
    /* register for closing signals so that we can trigger the stack to end */
    signal(SIGHUP, LeaveStack);
#endif

    /* The event loop. Put other processing you need done continually in here */
    while (1 != g_end_stack) {
      if ( kEipStatusOk != NetworkHandlerProcessOnce() ) {
        OPENER_TRACE_ERR("Error in NetworkHandler loop! Exiting OpENer!\n");
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

void LeaveStack(int signal) {
  (void) signal; /* kill unused parameter warning */
  OPENER_TRACE_STATE("got signal HUP\n");
  g_end_stack = 1;
}
