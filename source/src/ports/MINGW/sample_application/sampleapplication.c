/*******************************************************************************
 * Copyright (c) 2012, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "opener_api.h"
#include "appcontype.h"
#include <string.h>
#include <stdlib.h>

#define DEMO_APP_INPUT_ASSEMBLY_NUM                100 //0x064
#define DEMO_APP_OUTPUT_ASSEMBLY_NUM               150 //0x096
#define DEMO_APP_CONFIG_ASSEMBLY_NUM               1 //0x001
#define DEMO_APP_HEARTBEAT_INPUT_ONLY_ASSEMBLY_NUM  152 //0x098
#define DEMO_APP_HEARTBEAT_LISTEN_ONLY_ASSEMBLY_NUM 153 //0x099
#define DEMO_APP_EXPLICT_ASSEMBLY_NUM              154 //0x09A

/* global variables for demo application (4 assembly data fields)  ************/

extern CipUint g_encapsulation_inactivity_timeout;

EipUint8 g_assembly_data064[40]; /* Input */
EipUint8 g_assembly_data096[40]; /* Output */
EipUint8 g_assembly_data097[10]; /* Config */
EipUint8 g_assembly_data09A[32]; /* Explicit */

EipStatus ApplicationInitialization(void) {
  /* create 3 assembly object instances*/
  /*INPUT*/
  CreateAssemblyObject( DEMO_APP_INPUT_ASSEMBLY_NUM, &g_assembly_data064[0],
                        sizeof(g_assembly_data064) );

  /*OUTPUT*/
  CreateAssemblyObject( DEMO_APP_OUTPUT_ASSEMBLY_NUM, &g_assembly_data096[0],
                        sizeof(g_assembly_data096) );

  /*CONFIG*/
  CreateAssemblyObject( DEMO_APP_CONFIG_ASSEMBLY_NUM, &g_assembly_data097[0],
                        sizeof(g_assembly_data097) );

  /*Heart-beat output assembly for Input only connections */
  CreateAssemblyObject(DEMO_APP_HEARTBEAT_INPUT_ONLY_ASSEMBLY_NUM, 0, 0);

  /*Heart-beat output assembly for Listen only connections */
  CreateAssemblyObject(DEMO_APP_HEARTBEAT_LISTEN_ONLY_ASSEMBLY_NUM, 0, 0);

  /* assembly for explicit messaging */
  CreateAssemblyObject( DEMO_APP_EXPLICT_ASSEMBLY_NUM, &g_assembly_data09A[0],
                        sizeof(g_assembly_data09A) );

  ConfigureExclusiveOwnerConnectionPoint(0, DEMO_APP_OUTPUT_ASSEMBLY_NUM,
                                         DEMO_APP_INPUT_ASSEMBLY_NUM,
                                         DEMO_APP_CONFIG_ASSEMBLY_NUM);
  ConfigureInputOnlyConnectionPoint(0,
                                    DEMO_APP_HEARTBEAT_INPUT_ONLY_ASSEMBLY_NUM,
                                    DEMO_APP_INPUT_ASSEMBLY_NUM,
                                    DEMO_APP_CONFIG_ASSEMBLY_NUM);
  ConfigureListenOnlyConnectionPoint(0,
                                     DEMO_APP_HEARTBEAT_LISTEN_ONLY_ASSEMBLY_NUM,
                                     DEMO_APP_INPUT_ASSEMBLY_NUM,
                                     DEMO_APP_CONFIG_ASSEMBLY_NUM);

  return kEipStatusOk;
}

void HandleApplication(void) {
  /* check if application needs to trigger an connection */
}

void CheckIoConnectionEvent(unsigned int pa_unOutputAssembly,
                            unsigned int pa_unInputAssembly,
                            IoConnectionEvent pa_eIOConnectionEvent) {
  /* maintain a correct output state according to the connection state*/

  (void) pa_unOutputAssembly; /* suppress compiler warning */
  (void) pa_unInputAssembly; /* suppress compiler warning */
  (void) pa_eIOConnectionEvent; /* suppress compiler warning */
}

EipStatus AfterAssemblyDataReceived(CipInstance *pa_pstInstance) {
  EipStatus nRetVal = kEipStatusOk;

  /*handle the data received e.g., update outputs of the device */
  switch (pa_pstInstance->instance_number) {
    case DEMO_APP_OUTPUT_ASSEMBLY_NUM:
      /* Data for the output assembly has been received.
       * Mirror it to the inputs */
      memcpy( &g_assembly_data064[0], &g_assembly_data096[0],
              sizeof(g_assembly_data064) );
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
      nRetVal = kEipStatusOk;
      break;
  }
  return nRetVal;
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
  CloseAllConnections();
  return kEipStatusOk;
}

EipStatus ResetDeviceToInitialConfiguration(void) {
  /*rest the parameters */
  g_encapsulation_inactivity_timeout = 120;
  /*than perform device reset*/
  ResetDevice();
  return kEipStatusOk;
}

void *
CipCalloc(size_t pa_nNumberOfElements,
          size_t pa_nSizeOfElement) {
  return calloc(pa_nNumberOfElements, pa_nSizeOfElement);
}

void CipFree(void *pa_poData) {
  free(pa_poData);
}

void RunIdleChanged(EipUint32 pa_nRunIdleValue) {
  (void) pa_nRunIdleValue;
}

