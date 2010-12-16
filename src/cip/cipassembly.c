/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <string.h>    /*needed for memcpy */
#include "cipassembly.h"
#include "cipcommon.h"
#include "opener_api.h"
#include "trace.h"
#include "cipconnectionmanager.h"

/*! \brief Implementation of the SetAttributeSingle CIP service for Assembly
 *          Objects.
 *  Currently only supports Attribute 3 (CIP_BYTE_ARRAY) of an Assembly
 */
EIP_STATUS
setAssemblyAttributeSingle(S_CIP_Instance *pa_pstInstance,
    S_CIP_MR_Request *pa_pstMRRequest, S_CIP_MR_Response *pa_pstMRResponse,
    EIP_UINT8 *pa_acMsg);

S_CIP_Class *
createAssemblyClass()
{
  S_CIP_Class *pstAssemblyClass;
  /* create the CIP Assembly object with zero instances */
  pstAssemblyClass = createCIPClass(CIP_ASSEMBLY_CLASS_CODE, 0, /* # class attributes*/
  0, /* 0 as the assembly object should not have a get_attribute_all service*/
  0, /* # class services*/
  1, /* # instance attributes*/
  0, /* 0 as the assembly object should not have a get_attribute_all service*/
  1, /* # instance services*/
  0, /* # instances*/
  "assembly", 2 /* Revision, according to the CIP spec currently this has to be 2 */
  );
  if (NULL != pstAssemblyClass)
    {
      insertService(pstAssemblyClass, CIP_SET_ATTRIBUTE_SINGLE,
          &setAssemblyAttributeSingle, "SetAssemblyAttributeSingle");
    }

  return pstAssemblyClass;
}

EIP_STATUS
CIP_Assembly_Init()
{ /* create the CIP Assembly object with zero instances */
  return createAssemblyClass() ? EIP_OK : EIP_ERROR;
}

void
shutdownAssemblies(void)
{
  S_CIP_Class *pstAssemblyClass = getCIPClass(CIP_ASSEMBLY_CLASS_CODE);
  S_CIP_attribute_struct *pstAttribute;
  S_CIP_Instance *pstRunner;

  if (NULL != pstAssemblyClass)
    {
      pstRunner = pstAssemblyClass->pstInstances;
      while (NULL != pstRunner)
        {
          pstAttribute = getAttribute(pstRunner, 3);
          if (NULL != pstAttribute)
            {
              IApp_CipFree(pstAttribute->pt2data);
            }
          pstRunner = pstRunner->pstNext;
        }
    }
}

S_CIP_Instance *
createAssemblyObject(EIP_UINT32 pa_nInstanceID, EIP_BYTE * pa_data,
    EIP_UINT16 pa_datalength)
{
  S_CIP_Class *pstAssemblyClass;
  S_CIP_Instance *pstAssemblyInstance;
  S_CIP_Byte_Array *stAssemblyByteArray;

  if (NULL == (pstAssemblyClass = getCIPClass(CIP_ASSEMBLY_CLASS_CODE)))
    {
      if (NULL == (pstAssemblyClass = createAssemblyClass()))
        {
          return NULL;
        }
    }

  pstAssemblyInstance = addCIPInstance(pstAssemblyClass, pa_nInstanceID); /* add instances (always succeeds (or asserts))*/

  if ((stAssemblyByteArray = (S_CIP_Byte_Array *) IApp_CipCalloc(1,
      sizeof(S_CIP_Byte_Array))) == NULL)
    {
      return NULL; /*TODO remove assembly instance in case of error*/
    }

  stAssemblyByteArray->len = pa_datalength;
  stAssemblyByteArray->Data = pa_data;
  insertAttribute(pstAssemblyInstance, 3, CIP_BYTE_ARRAY, stAssemblyByteArray);

  return pstAssemblyInstance;
}

EIP_STATUS
notifyAssemblyConnectedDataReceived(S_CIP_Instance * pa_pstInstance,
    EIP_UINT8 * pa_pnData, EIP_UINT16 pa_nDataLength)
{
  S_CIP_Byte_Array *p;

  /* empty path (path size = 0) need to be checked and taken care of in future */
  /* copy received data to Attribute 3 */
  p = (S_CIP_Byte_Array *) pa_pstInstance->pstAttributes->pt2data;
  if (p->len < pa_nDataLength)
    {
      OPENER_TRACE_ERR("too much data arrived\n");
      return EIP_ERROR; /*TODO question should we notify the application that wrong data has been recieved???*/
    }
  else
    {
      memcpy(p->Data, pa_pnData, pa_nDataLength);
      /* call the application that new data arrived */
    }

  return IApp_AfterAssemblyDataReceived(pa_pstInstance);
}

EIP_STATUS
setAssemblyAttributeSingle(S_CIP_Instance *pa_pstInstance,
    S_CIP_MR_Request *pa_pstMRRequest, S_CIP_MR_Response *pa_pstMRResponse,
    EIP_UINT8 *pa_acMsg)
{

  OPENER_TRACE_INFO(" setAttribute %d\n", pa_pstMRRequest->RequestPath.AttributNr);
  EIP_UINT8 * pa_acReqData = pa_pstMRRequest->Data;

  pa_acMsg = pa_acMsg; /* suppress compiler warning */

  pa_pstMRResponse->DataLength = 0;
  pa_pstMRResponse->ReplyService = (0x80 | pa_pstMRRequest->Service);
  pa_pstMRResponse->GeneralStatus = CIP_ERROR_ATTRIBUTE_NOT_SUPPORTED;
  pa_pstMRResponse->SizeofAdditionalStatus = 0;

  S_CIP_attribute_struct *p = getAttribute(pa_pstInstance,
      pa_pstMRRequest->RequestPath.AttributNr);

  if ((p != 0) && (3 == pa_pstMRRequest->RequestPath.AttributNr))
    {
      if (p->pt2data != 0)
        {
          S_CIP_Byte_Array * pacData = (S_CIP_Byte_Array *) p->pt2data;

          if (true == isConnectedOutputAssembly(pa_pstInstance->nInstanceNr))
            {
              OPENER_TRACE_WARN("Assembly AssemblyAttributeSingle: received data for connected output assembly\n\r");
              pa_pstMRResponse->GeneralStatus = CIP_ERROR_ATTRIBUTE_NOT_SETTABLE;
            }
          else
            {
              if (pa_pstMRRequest->DataLength < pacData->len)
                {
                  OPENER_TRACE_INFO("Assembly setAssemblyAttributeSingle: not enough data received.\r\n");
                  pa_pstMRResponse->GeneralStatus = CIP_ERROR_NOT_ENOUGH_DATA;
                }
              else
                {
                  if (pa_pstMRRequest->DataLength > pacData->len)
                    {
                      OPENER_TRACE_INFO("Assembly setAssemblyAttributeSingle: too much data received.\r\n");
                      pa_pstMRResponse->GeneralStatus = CIP_ERROR_TOO_MUCH_DATA;
                    }
                  else
                    {
                      memcpy(pacData->Data, pa_acReqData, pacData->len);

                      if (IApp_AfterAssemblyDataReceived(pa_pstInstance)
                          != EIP_OK)
                        {
                          /* punt early without updating the status... though I don't know
                           * how much this helps us here, as the attribute's data has already
                           * been overwritten.
                           *
                           * however this is the task of the application side which will
                           * take the data. In addition we have to inform the sender that the
                           * data was not ok.
                           */
                          pa_pstMRResponse->GeneralStatus
                              = CIP_ERROR_INVALID_ATTRIBUTE_VALUE;
                        }
                      else
                        {
                          pa_pstMRResponse->GeneralStatus = CIP_ERROR_SUCCESS;
                        }
                    }
                }
            }
        }
      else
        {
          /* the attribute was zero we are a heartbeat assembly */
          pa_pstMRResponse->GeneralStatus = CIP_ERROR_TOO_MUCH_DATA;
        }
    }

  return EIP_OK_SEND;
}
