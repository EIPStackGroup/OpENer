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
  EipUint8 acMyMACAddress[6];
  EipUint16 nUniqueConnectionID;

  if (argc != 12) {
    printf("Wrong number of command line parameters!\n");
    printf("The correct command line parameters are:\n");
    printf(
      "./OpENer ipaddress subnetmask gateway domainname hostaddress macaddress\n");
    printf(
      "    e.g. ./OpENer 192.168.0.2 255.255.255.0 192.168.0.1 test.com testdevice 00 15 C5 BF D0 87\n");
    exit(0);
  } else {
    /* fetch Internet address info from the platform */
    ConfigureNetworkInterface(arg[1], arg[2], arg[3]);
    ConfigureDomainName(arg[4]);
    ConfigureHostName(arg[5]);

    acMyMACAddress[0] = (EipUint8) strtoul(arg[6], NULL, 16);
    acMyMACAddress[1] = (EipUint8) strtoul(arg[7], NULL, 16);
    acMyMACAddress[2] = (EipUint8) strtoul(arg[8], NULL, 16);
    acMyMACAddress[3] = (EipUint8) strtoul(arg[9], NULL, 16);
    acMyMACAddress[4] = (EipUint8) strtoul(arg[10], NULL, 16);
    acMyMACAddress[5] = (EipUint8) strtoul(arg[11], NULL, 16);
    ConfigureMacAddress(acMyMACAddress);
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
