/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "networkhandler.h"
#include "opener_api.h"
#include "cipcommon.h"
#include "trace.h"

/* global variables for demo application (3 assembly data fields) */
EIP_UINT8 g_assemblydata[32]; /* Input */
EIP_UINT8 g_assemblydata2[32]; /* Output */
EIP_UINT8 g_assemblydata3[10]; /* Config */

extern int newfd;

int main(int argc, char *arg[])
  {
    EIP_UINT8 acMyMACAddress[6];

    if (argc != 12)
      { printf("Wrong number of command line parameters!\n");
        printf("The correct command line parameters are:\n");
        printf("./opener ipaddress subnetmask gateway domainname hostaddress macaddress\n");
        printf("    e.g. ./opener 192.168.0.2 255.255.255.0 192.168.0.1 test.com testdevice 00 15 C5 BF D0 87\n");      
        exit(0);
      }
    else
      {
        /* fetch Internet address info from the platform */
        configureNetworkInterface(arg[1], arg[2], arg[3]);
        configureDomainName(arg[4]);
        configureHostName(arg[5]);

        acMyMACAddress[0] = (EIP_UINT8)strtoul(arg[6], NULL, 16);
        acMyMACAddress[1] = (EIP_UINT8)strtoul(arg[7], NULL, 16);
        acMyMACAddress[2] = (EIP_UINT8)strtoul(arg[8], NULL, 16);
        acMyMACAddress[3] = (EIP_UINT8)strtoul(arg[9], NULL, 16);
        acMyMACAddress[4] = (EIP_UINT8)strtoul(arg[10], NULL, 16);
        acMyMACAddress[5] = (EIP_UINT8)strtoul(arg[11], NULL, 16);
        configureMACAddress(acMyMACAddress);
      }
    
    /*for a real device the serial number should be unique per device */
    setDeviceSerialNumber(123456789);

    /* Setup the CIP Layer */
    CIP_Init();

    Start_NetworkHandler(); /* here is the select loop implemented */
    return -1;
  }

EIP_STATUS IApp_Init(void)
  {
    /* create 3 assembly object instances*/
    /*INPUT*/
    createAssemblyObject(1, &g_assemblydata[0], sizeof(g_assemblydata));

    /*OUTPUT*/
    createAssemblyObject(2, &g_assemblydata2[0], sizeof(g_assemblydata2));

    /*CONFIG*/
    createAssemblyObject(3, &g_assemblydata3[0], sizeof(g_assemblydata3));

    /*Heartbeat output assembly for Listen only connections */
    createAssemblyObject(4, 0, 0);

    return EIP_OK;
  }

EIP_STATUS IApp_AfterAssemblyDataReceived(S_CIP_Instance *pa_pstInstance)
  {
    /*handle the data received e.g., update outputs of the device */

    return EIP_OK;
  }

bool IApp_BeforeAssemblyDataSend(S_CIP_Instance *pa_pstInstance)
  {
    /*update data to be sent e.g., read inputs of the device */
    return true;
  }

EIP_STATUS IApp_ResetDevice(void)
  {
    /* add reset code here*/
    return EIP_OK;
  }

EIP_STATUS IApp_ResetDeviceToInitialConfiguration(void)
  {
    /*rest the parameters and than perform device reset*/
    return EIP_OK;
  }

void *IApp_CipCalloc(unsigned pa_nNumberOfElements, unsigned pa_nSizeOfElement)
  {
    return calloc(pa_nNumberOfElements, pa_nSizeOfElement);
  }

void IApp_RunIdleChanged(EIP_UINT32 pa_nRunIdleValue)
  {
    (void)pa_nRunIdleValue;
  }

