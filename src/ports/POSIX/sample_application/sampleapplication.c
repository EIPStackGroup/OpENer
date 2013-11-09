/*******************************************************************************
 * Copyright (c) 2012, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "opener_api.h"
#include <string.h>
#include <stdlib.h>


#define DEMO_APP_INPUT_ASSEMBLY_NUM                100 //0x064
#define DEMO_APP_OUTPUT_ASSEMBLY_NUM               150 //0x096
#define DEMO_APP_CONFIG_ASSEMBLY_NUM               151 //0x097
#define DEMO_APP_HEARBEAT_INPUT_ONLY_ASSEMBLY_NUM  152 //0x098
#define DEMO_APP_HEARBEAT_LISTEN_ONLY_ASSEMBLY_NUM 153 //0x099
#define DEMO_APP_EXPLICT_ASSEMBLY_NUM              154 //0x09A

/* global variables for demo application (4 assembly data fields)  ************/

EIP_UINT8 g_assemblydata064[32]; /* Input */
EIP_UINT8 g_assemblydata096[32]; /* Output */
EIP_UINT8 g_assemblydata097[10]; /* Config */
EIP_UINT8 g_assemblydata09A[32]; /* Explicit */

EIP_STATUS
IApp_Init(void)
{
  /* create 3 assembly object instances*/
  /*INPUT*/
  createAssemblyObject(DEMO_APP_INPUT_ASSEMBLY_NUM, &g_assemblydata064[0],
      sizeof(g_assemblydata064));

  /*OUTPUT*/
  createAssemblyObject(DEMO_APP_OUTPUT_ASSEMBLY_NUM, &g_assemblydata096[0],
      sizeof(g_assemblydata096));

  /*CONFIG*/
  createAssemblyObject(DEMO_APP_CONFIG_ASSEMBLY_NUM, &g_assemblydata097[0],
      sizeof(g_assemblydata097));

  /*Heart-beat output assembly for Input only connections */
  createAssemblyObject(DEMO_APP_HEARBEAT_INPUT_ONLY_ASSEMBLY_NUM, 0, 0);

  /*Heart-beat output assembly for Listen only connections */
  createAssemblyObject(DEMO_APP_HEARBEAT_LISTEN_ONLY_ASSEMBLY_NUM, 0, 0);

  /* assembly for explicit messaging */
  createAssemblyObject(DEMO_APP_EXPLICT_ASSEMBLY_NUM, &g_assemblydata09A[0],
      sizeof(g_assemblydata09A));

  configureExclusiveOwnerConnectionPoint(0, DEMO_APP_OUTPUT_ASSEMBLY_NUM,
      DEMO_APP_INPUT_ASSEMBLY_NUM, DEMO_APP_CONFIG_ASSEMBLY_NUM);
  configureInputOnlyConnectionPoint(0,
      DEMO_APP_HEARBEAT_INPUT_ONLY_ASSEMBLY_NUM, DEMO_APP_INPUT_ASSEMBLY_NUM,
      DEMO_APP_CONFIG_ASSEMBLY_NUM);
  configureListenOnlyConnectionPoint(0,
      DEMO_APP_HEARBEAT_LISTEN_ONLY_ASSEMBLY_NUM, DEMO_APP_INPUT_ASSEMBLY_NUM,
      DEMO_APP_CONFIG_ASSEMBLY_NUM);

  return EIP_OK;
}

void
IApp_HandleApplication(void)
{
  /* check if application needs to trigger an connection */
}

void
IApp_IOConnectionEvent(unsigned int pa_unOutputAssembly,
    unsigned int pa_unInputAssembly, EIOConnectionEvent pa_eIOConnectionEvent)
{
  /* maintain a correct output state according to the connection state*/

  (void) pa_unOutputAssembly; /* suppress compiler warning */
  (void) pa_unInputAssembly; /* suppress compiler warning */
  (void) pa_eIOConnectionEvent; /* suppress compiler warning */
}

EIP_STATUS
IApp_AfterAssemblyDataReceived(S_CIP_Instance *pa_pstInstance)
{
  EIP_STATUS nRetVal = EIP_OK;

  /*handle the data received e.g., update outputs of the device */
  switch (pa_pstInstance->nInstanceNr)
    {
  case DEMO_APP_OUTPUT_ASSEMBLY_NUM:
    /* Data for the output assembly has been received.
     * Mirror it to the inputs */
    memcpy(&g_assemblydata064[0], &g_assemblydata096[0],
        sizeof(g_assemblydata064));
    break;
  case DEMO_APP_EXPLICT_ASSEMBLY_NUM:
    /* do something interesting with the new data from
     * the explicit set-data-attribute message */
    break;
  case DEMO_APP_CONFIG_ASSEMBLY_NUM:
    /* Add here code to handle configuration data and check if it is ok
     * The demo application does not handle config data.
     * However in order to pass the test we accept any data given.
     * EIP_ERROR
     */
    nRetVal = EIP_OK;
    break;
    }
  return nRetVal;
}

EIP_BOOL8
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
  /*rest the parameters */

  /*than perform device reset*/
  IApp_ResetDevice();
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

