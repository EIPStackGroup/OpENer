/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
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
EipUint8 g_assemblydata301[32]; /* Input */
EipUint8 g_assemblydata302[32]; /* Output */
EipUint8 g_assemblydata303[10]; /* Config */
EipUint8 g_assemblydata306[32]; /* Explicit */

extern int newfd;

EipStatus ApplicationInitialization(void) {
//  CIP_Motion_Init();

  /* create 3 assembly object instances*/
  /*INPUT*/
  CreateAssemblyObject(DEMO_APP_INPUT_ASSEMBLY_NUM, &g_assemblydata301[0],
                       sizeof(g_assemblydata301));

  /*OUTPUT*/
  CreateAssemblyObject(DEMO_APP_OUTPUT_ASSEMBLY_NUM, &g_assemblydata302[0],
                       sizeof(g_assemblydata302));

  /*CONFIG*/
  CreateAssemblyObject(DEMO_APP_CONFIG_ASSEMBLY_NUM, &g_assemblydata303[0],
                       sizeof(g_assemblydata303));

  /*Heart-beat output assembly for Input only connections */
  CreateAssemblyObject(DEMO_APP_HEARBEAT_INPUT_ONLY_ASSEMBLY_NUM, 0, 0);

  /*Heart-beat output assembly for Listen only connections */
  CreateAssemblyObject(DEMO_APP_HEARBEAT_LISTEN_ONLY_ASSEMBLY_NUM, 0, 0);

  /* assembly for explicit messaging */
  CreateAssemblyObject(DEMO_APP_EXPLICT_ASSEMBLY_NUM, &g_assemblydata306[0],
                       sizeof(g_assemblydata306));

  ConfigureExclusiveOwnerConnectionPoint(0, DEMO_APP_OUTPUT_ASSEMBLY_NUM,
                                         DEMO_APP_INPUT_ASSEMBLY_NUM,
                                         DEMO_APP_CONFIG_ASSEMBLY_NUM);
  ConfigureInputOnlyConnectionPoint(0,
                                    DEMO_APP_HEARBEAT_INPUT_ONLY_ASSEMBLY_NUM,
                                    DEMO_APP_INPUT_ASSEMBLY_NUM,
                                    DEMO_APP_CONFIG_ASSEMBLY_NUM);
  ConfigureListenOnlyConnectionPoint(0,
                                     DEMO_APP_HEARBEAT_LISTEN_ONLY_ASSEMBLY_NUM,
                                     DEMO_APP_INPUT_ASSEMBLY_NUM,
                                     DEMO_APP_CONFIG_ASSEMBLY_NUM);

  return EIP_OK;
}

void IoConnectionEvent(unsigned int pa_unOutputAssembly,
                       unsigned int pa_unInputAssembly,
                       EIOConnectionEvent pa_eIOConnectionEvent) {
  /* maintain a correct output state according to the connection state*/

  (void) pa_unOutputAssembly; /* suppress compiler warning */
  pa_unInputAssembly = pa_unInputAssembly; /* suppress compiler warning */
  pa_eIOConnectionEvent = pa_eIOConnectionEvent; /* suppress compiler warning */
}

EipStatus AfterAssemblyDataReceived(CipInstance *pa_pstInstance) {
  /*handle the data received e.g., update outputs of the device */

  if (pa_pstInstance->instance_number == DEMO_APP_OUTPUT_ASSEMBLY_NUM) {
    /* Data for the output assembly has been received.
     * Mirror it to the inputs */
    memcpy(&g_assemblydata301[0], &g_assemblydata302[0],
           sizeof(g_assemblydata301));
  } else if (pa_pstInstance->instance_number == DEMO_APP_EXPLICT_ASSEMBLY_NUM) {
    /* do something interesting with the new data from
     * the explicit set-data-attribute message */
  }

  return EIP_OK;
}

EipBool8 BeforeAssemblyDataSend(CipInstance *pa_pstInstance) {
  /*update data to be sent e.g., read inputs of the device */
  /*In this sample app we mirror the data from out to inputs on data receive
   * therefore we need nothing to do here. Just return true to inform that
   * the data is new.
   */

  if (pa_pstInstance->instance_number == DEMO_APP_EXPLICT_ASSEMBLY_NUM) {
    /* do something interesting with the existing data
     * for the explicit get-data-attribute message */
  }
  return true;
}

EipStatus ResetDevice(void) {
  /* add reset code here*/
  return EIP_OK;
}

EipStatus ResetDeviceToInitialConfiguration(void) {
  /*rest the parameters and than perform device reset*/
  return EIP_OK;
}

void *
CipCalloc(unsigned pa_nNumberOfElements, unsigned pa_nSizeOfElement) {
  return mem_calloc(pa_nNumberOfElements, pa_nSizeOfElement);
}

void CipFree(void *pa_poData) {
  mem_free(pa_poData);
}

void RunIdleChanged(EipUint32 pa_nRunIdleValue) {
  (void) pa_nRunIdleValue;
}

void HandleApplication(void) {
  /* check if application needs to trigger an connection */
}

