/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#include <stdio.h>
#include <string.h>    /*needed for memcpy */
#include "cipassembly.h"
#include "cipcommon.h"
#include "opener_api.h"


S_CIP_Class *createAssemblyClass()
  { /* create the CIP Assembly object with zero instances */
    return createCIPClass(CIP_ASSEMBLY_CLASS_CODE, 0, // # class attributes
            0xffffffff, // class getAttributeAll mask
            0, // # class services
            1, // # instance attributes
            0xffffffff, // instance getAttributeAll mask
            0, // # instance services
            0, // # instances
            "assembly", 1);
  }


EIP_STATUS CIP_Assembly_Init()
  { /* create the CIP Assembly object with zero instances */
    return createAssemblyClass() ? EIP_OK : EIP_ERROR;
  }

S_CIP_Instance *createAssemblyObject(EIP_UINT8 pa_nInstanceID,
    EIP_BYTE * pa_data, EIP_UINT16 pa_datalength)
  {
    S_CIP_Class *pstAssemblyClass;
    S_CIP_Instance *pstAssemblyInstance;
    S_CIP_Byte_Array *stAssemblyByteArray;

    if (0 == (pstAssemblyClass = getCIPClass(CIP_ASSEMBLY_CLASS_CODE)))
      {
        if (0 == (pstAssemblyClass = createAssemblyClass()))
          {
            return 0;
          }
      }

    pstAssemblyInstance = addCIPInstance(pstAssemblyClass, pa_nInstanceID); // add instances (always succeeds (or asserts))

    if ((stAssemblyByteArray = IApp_CipCalloc(1, sizeof(S_CIP_Byte_Array))) == 0)
      {
        return 0; //TODO remove assembly instance in case of error
      }

    stAssemblyByteArray->len = pa_datalength;
    stAssemblyByteArray->Data = pa_data;
    insertAttribute(pstAssemblyInstance, 3, CIP_BYTE_ARRAY, stAssemblyByteArray);

    return pstAssemblyInstance;
  }

EIP_STATUS notifyAssemblyConnectedDataReceived(S_CIP_Instance * pa_pstInstance,
    EIP_UINT8 * pa_pnData, EIP_UINT16 pa_nDataLength)
  {
    S_CIP_Byte_Array *p;
    
    /* empty path (path size = 0) need to be checked and taken care of in future */
    /* copy received data to Attribute 3 */
    p = pa_pstInstance->pstAttributes->pt2data;
    if (p->len < pa_nDataLength)
      {
        if (EIP_DEBUG>EIP_VERBOSE)
          printf("too much data arrived\n");
        return EIP_ERROR; //TODO question should we notify the application that wrong data has been recieved???
      }
    else
      {
        memcpy(p->Data, pa_pnData, pa_nDataLength);
        /* call the application that new data arrived */
      }
    
    return IApp_AfterAssemblyDataReceived(pa_pstInstance);
  }
