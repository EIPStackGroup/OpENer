/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "networkhandler.h"
#include "opener_api.h"
#include "cipcommon.h"
#include "trace.h"

#define DEMO_APP_INPUT_ASSEMBLY_NUM                0x301
#define DEMO_APP_OUTPUT_ASSEMBLY_NUM               0x302
#define DEMO_APP_CONFIG_ASSEMBLY_NUM               0x303
#define DEMO_APP_HEARBEAT_INPUT_ONLY_ASSEMBLY_NUM  0x304
#define DEMO_APP_HEARBEAT_LISTEN_ONLY_ASSEMBLY_NUM 0x305
#define DEMO_APP_EXPLICT_ASSEMBLY_NUM              0x306


/* global variables for demo application (4 assembly data fields) */
EIP_UINT8 g_assemblydata301[32]; /* Input */
EIP_UINT8 g_assemblydata302[32]; /* Output */
EIP_UINT8 g_assemblydata303[10]; /* Config */
EIP_UINT8 g_assemblydata306[32]; /* Explicit */

extern int newfd;

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

  Start_NetworkHandler(); /* here is the select loop implemented */

  /* close remaining sessions and connections, cleanup used data */
  shutdownCIP();
  return -1;
}

EIP_STATUS
IApp_Init(void)
{
  /* create 3 assembly object instances*/
  /*INPUT*/
  createAssemblyObject(DEMO_APP_INPUT_ASSEMBLY_NUM, &g_assemblydata301[0], sizeof(g_assemblydata301));

  /*OUTPUT*/
  createAssemblyObject(DEMO_APP_OUTPUT_ASSEMBLY_NUM, &g_assemblydata302[0], sizeof(g_assemblydata302));

  /*CONFIG*/
  createAssemblyObject(DEMO_APP_CONFIG_ASSEMBLY_NUM, &g_assemblydata303[0], sizeof(g_assemblydata303));

  /*Heart-beat output assembly for Input only connections */
  createAssemblyObject(DEMO_APP_HEARBEAT_INPUT_ONLY_ASSEMBLY_NUM, 0, 0);

  /*Heart-beat output assembly for Listen only connections */
  createAssemblyObject(DEMO_APP_HEARBEAT_LISTEN_ONLY_ASSEMBLY_NUM, 0, 0);

  /* assembly for explicit messaging */
  createAssemblyObject(DEMO_APP_EXPLICT_ASSEMBLY_NUM, &g_assemblydata306[0], sizeof(g_assemblydata306));

  configureExclusiveOwnerConnectionPoint(0, DEMO_APP_OUTPUT_ASSEMBLY_NUM, DEMO_APP_INPUT_ASSEMBLY_NUM, DEMO_APP_CONFIG_ASSEMBLY_NUM);
  configureInputOnlyConnectionPoint(0, DEMO_APP_HEARBEAT_INPUT_ONLY_ASSEMBLY_NUM, DEMO_APP_INPUT_ASSEMBLY_NUM, DEMO_APP_CONFIG_ASSEMBLY_NUM);
  configureListenOnlyConnectionPoint(0, DEMO_APP_HEARBEAT_LISTEN_ONLY_ASSEMBLY_NUM, DEMO_APP_INPUT_ASSEMBLY_NUM, DEMO_APP_CONFIG_ASSEMBLY_NUM);

  return EIP_OK;
}

void
IApp_IOConnectionEvent(unsigned int pa_unOutputAssembly,
    unsigned int pa_unInputAssembly, EIOConnectionEvent pa_eIOConnectionEvent)
{
  /* maintain a correct output state according to the connection state*/

  (void) pa_unOutputAssembly; /* suppress compiler warning */
  pa_unInputAssembly = pa_unInputAssembly; /* suppress compiler warning */
  pa_eIOConnectionEvent = pa_eIOConnectionEvent; /* suppress compiler warning */
}

EIP_STATUS
IApp_AfterAssemblyDataReceived(S_CIP_Instance *pa_pstInstance)
{
  /*handle the data received e.g., update outputs of the device */

  if (pa_pstInstance->nInstanceNr == DEMO_APP_OUTPUT_ASSEMBLY_NUM)
    {
      /* Data for the output assembly has been received.
       * Mirror it to the inputs */
      memcpy(&g_assemblydata301[0], &g_assemblydata302[0], sizeof(g_assemblydata301));
    }
  else if (pa_pstInstance->nInstanceNr == DEMO_APP_EXPLICT_ASSEMBLY_NUM)
    {
      /* do something interesting with the new data from
       * the explicit set-data-attribute message */
    }

  return EIP_OK;
}

bool
IApp_BeforeAssemblyDataSend(S_CIP_Instance *pa_pstInstance)
{
  /*update data to be sent e.g., read inputs of the device */
  /*In this sample app we mirror the data from out to inputs on data receive
   * therefore we need nothing to do here. Just return true to inform that
   * the data is new.
   */

  if (pa_pstInstance->nInstanceNr == DEMO_APP_EXPLICT_ASSEMBLY_NUM)
    {
      /* do something interesting with the existing data
       * for the explicit get-data-attribute message */
    }
  return true;
}

EIP_STATUS
IApp_ResetDevice(void)
{
  /* add reset code here*/
  return EIP_OK;
}

EIP_STATUS
IApp_ResetDeviceToInitialConfiguration(void)
{
  /*rest the parameters and than perform device reset*/
  return EIP_OK;
}

void *
IApp_CipCalloc(unsigned pa_nNumberOfElements, unsigned pa_nSizeOfElement)
{
  return calloc(pa_nNumberOfElements, pa_nSizeOfElement);
}

void
IApp_CipFree(void *pa_poData)
{
  free(pa_poData);
}

void
IApp_RunIdleChanged(EIP_UINT32 pa_nRunIdleValue)
{
  (void) pa_nRunIdleValue;
}

