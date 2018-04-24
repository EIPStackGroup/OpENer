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
int main(int argc,
         char *arg[]) {
  EipUint16 nUniqueConnectionID;

  if (argc != 2) {
    printf("Wrong number of command line parameters!\n");
    printf("The correct command line parameters are:\n");
    printf(
      "./OpENer index\n");
    printf(
      "    e.g. ./OpENer index\n");
    exit(0);
  } else {
    DoublyLinkedListInitialize(&connection_list,
                               CipConnectionObjectListArrayAllocator,
                               CipConnectionObjectListArrayFree);
    /* fetch Internet address info from the platform */
    ConfigureDomainName(atoi(arg[1]) );
    ConfigureHostName(atoi(arg[1]) );
    ConfigureIpMacAddress(atoi(arg[1]) );
  }

  /*for a real device the serial number should be unique per device */
  SetDeviceSerialNumber(123456789);

  /* nUniqueConnectionID should be sufficiently random or incremented and stored
   *  in non-volatile memory each time the device boots.
   */
  nUniqueConnectionID = rand();

  /* Setup the CIP Layer */
  CipStackInit(nUniqueConnectionID);

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
  OPENER_TRACE_STATE("got signal HUP\n");
  g_end_stack = 1;
}
