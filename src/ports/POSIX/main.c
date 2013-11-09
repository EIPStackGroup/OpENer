/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "networkhandler.h"
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
leaveStack(int pa_nSig);

/*****************************************************************************/
/*! \brief Flag indicating if the stack should end its execution
 */
int g_nEndStack = 0;

/******************************************************************************/
int
main(int argc, char *arg[])
{
  EIP_UINT8 acMyMACAddress[6];
  EIP_UINT16 nUniqueConnectionID;

  if (argc != 12)
    {
      printf("Wrong number of command line parameters!\n");
      printf("The correct command line parameters are:\n");
      printf(
          "./opener ipaddress subnetmask gateway domainname hostaddress macaddress\n");
      printf(
          "    e.g. ./opener 192.168.0.2 255.255.255.0 192.168.0.1 test.com testdevice 00 15 C5 BF D0 87\n");
      exit(0);
    }
  else
    {
      /* fetch Internet address info from the platform */
      configureNetworkInterface(arg[1], arg[2], arg[3]);
      configureDomainName(arg[4]);
      configureHostName(arg[5]);

      acMyMACAddress[0] = (EIP_UINT8) strtoul(arg[6], NULL, 16);
      acMyMACAddress[1] = (EIP_UINT8) strtoul(arg[7], NULL, 16);
      acMyMACAddress[2] = (EIP_UINT8) strtoul(arg[8], NULL, 16);
      acMyMACAddress[3] = (EIP_UINT8) strtoul(arg[9], NULL, 16);
      acMyMACAddress[4] = (EIP_UINT8) strtoul(arg[10], NULL, 16);
      acMyMACAddress[5] = (EIP_UINT8) strtoul(arg[11], NULL, 16);
      configureMACAddress(acMyMACAddress);
    }

  /*for a real device the serial number should be unique per device */
  setDeviceSerialNumber(123456789);

  /* nUniqueConnectionID should be sufficiently random or incremented and stored
   *  in non-volatile memory each time the device boots.
   */
  nUniqueConnectionID = rand();

  /* Setup the CIP Layer */
  CIP_Init(nUniqueConnectionID);

  /* Setup Network Handles */
  if (EIP_OK == NetworkHandler_Init())
    {
      g_nEndStack = 0;
#ifndef WIN32
      /* register for closing signals so that we can trigger the stack to end */
      signal(SIGHUP, leaveStack);
#endif

      /* The event loop. Put other processing you need done continually in here */
      while (1 != g_nEndStack)
        {
          if( EIP_OK != NetworkHandler_ProcessOnce())
            {
              break;
            }
        }

      /* clean up network state */
      NetworkHandler_Finish();
    }
  /* close remaining sessions and connections, cleanup used data */
  shutdownCIP();

  return -1;
}

void
leaveStack(int pa_nSig)
{
  (void) pa_nSig; /* kill unused parameter warning */
  OPENER_TRACE_STATE("got signal HUP\n");
  g_nEndStack = 1;
}
