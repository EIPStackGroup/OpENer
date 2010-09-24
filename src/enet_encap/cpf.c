/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include "opener_api.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "endianconv.h"
#include "ciperror.h"
#include "cpf.h"
#include "cipconnectionmanager.h"
#include "trace.h"

/* CPF global data items */
S_CIP_CPF_Data g_stCPFDataItem;

int
notifyCPF(struct S_Encapsulation_Data * pa_stReceiveData, /* recieved encap data*/
EIP_UINT8 * pa_acReplyBuf) /* reply buffer*/
{
  int nRetVal;

  if ((nRetVal = createCPFstructure(pa_stReceiveData->m_acCurrentCommBufferPos,
      pa_stReceiveData->nData_length, &g_stCPFDataItem)) == EIP_ERROR)
    {
      OPENER_TRACE_ERR("notifyMR: error from createCPFstructure\n");
    }
  else
    {
      nRetVal = EIP_OK; /* In cases of errors we normaly need to send an error response */
      if (g_stCPFDataItem.stAddr_Item.TypeID == CIP_ITEM_ID_NULL) /* check if NullAddressItem received, otherwise it is no unconnected message and should not be here*/
        { /* found null address item*/
          if (g_stCPFDataItem.stDataI_Item.TypeID
              == CIP_ITEM_ID_UNCONNECTEDMESSAGE)
            { /* unconnected data item received*/
              nRetVal = notifyMR(g_stCPFDataItem.stDataI_Item.Data,
                  g_stCPFDataItem.stDataI_Item.Length);
              if (nRetVal != EIP_ERROR)
                {
                  nRetVal = assembleLinearMsg(&gMRResponse, &g_stCPFDataItem,
                      pa_acReplyBuf);
                }
            }
          else
            {
              /* wrong data item detected*/
              OPENER_TRACE_ERR(
                  "notifyMR: got something besides the expected CIP_ITEM_ID_UNCONNECTEDMESSAGE\n");
              pa_stReceiveData->nStatus = OPENER_ENCAP_STATUS_INCORRECT_DATA;
            }
        }
      else
        {
          OPENER_TRACE_ERR("notifyMR: got something besides the expected CIP_ITEM_ID_NULL\n");
          pa_stReceiveData->nStatus = OPENER_ENCAP_STATUS_INCORRECT_DATA;
        }
    }
  return nRetVal;
}

int
notifyConnectedCPF(struct S_Encapsulation_Data * pa_stReceiveData, /* recieved encap data*/
EIP_UINT8 * pa_acReplyBuf) /* reply buffer*/
{
  int nRetVal;
  S_CIP_ConnectionObject *pstConnectionObject;

  nRetVal = createCPFstructure(
        pa_stReceiveData->m_acCurrentCommBufferPos,
        pa_stReceiveData->nData_length, &g_stCPFDataItem);

  if (EIP_ERROR == nRetVal)
    {
      OPENER_TRACE_ERR("notifyConnectedCPF: error from createCPFstructure\n");
    }
  else
    {
      nRetVal = EIP_ERROR; /* For connected explicit messages status always has to be 0*/
      if (g_stCPFDataItem.stAddr_Item.TypeID == CIP_ITEM_ID_CONNECTIONBASED) /* check if ConnectedAddressItem received, otherwise it is no connected message and should not be here*/
        { /* ConnectedAddressItem item */
          pstConnectionObject = getConnectedObject(
              g_stCPFDataItem.stAddr_Item.Data.ConnectionIdentifier);
          if (NULL != pstConnectionObject)
            {
              /* reset the watchdog timer */
              pstConnectionObject->InnacitvityWatchdogTimer
                  = (pstConnectionObject->O_to_T_RPI / 1000) << (2
                      + pstConnectionObject->ConnectionTimeoutMultiplier);

              /*TODO check connection id  and sequence count    */
              if (g_stCPFDataItem.stDataI_Item.TypeID
                  == CIP_ITEM_ID_CONNECTIONTRANSPORTPACKET)
                { /* connected data item received*/
                  EIP_UINT8 *pnBuf = g_stCPFDataItem.stDataI_Item.Data;
                  g_stCPFDataItem.stAddr_Item.Data.SequenceNumber
                      = (EIP_UINT32) ltohs(&pnBuf);
                  nRetVal = notifyMR(pnBuf, g_stCPFDataItem.stDataI_Item.Length
                      - 2);

                  if (nRetVal != EIP_ERROR)
                    {
                      g_stCPFDataItem.stAddr_Item.Data.ConnectionIdentifier = pstConnectionObject->CIPProducedConnectionID;
                      nRetVal = assembleLinearMsg(&gMRResponse,
                          &g_stCPFDataItem, pa_acReplyBuf);
                    }
                }
              else
                {
                  /* wrong data item detected*/
                  OPENER_TRACE_ERR(
                      "notifyConnectedCPF: got something besides the expected CIP_ITEM_ID_UNCONNECTEDMESSAGE\n");
                }
            }
          else
            {
              OPENER_TRACE_ERR("notifyConnectedCPF: connection with given ID could not be found\n");
            }
        }
      else
        {
          OPENER_TRACE_ERR("notifyConnectedCPF: got something besides the expected CIP_ITEM_ID_NULL\n");
        }
    }
  return nRetVal;
}

/*   INT16 createCPFstructure(INT8 *pa_Data, INT16 pa_DataLength, S_CIP_CPF_Data *pa_CPF_data)
 *   create CPF structure out of pa_data.
 *      pa_Data		pointer to data which need to be structured.
 *      pa_DataLength	length of data in pa_Data.
 *      pa_CPF_data	pointer to structure of CPF data item.
 *  return status
 * 		0 .. success
 * 	       -1 .. error
 */
EIP_STATUS
createCPFstructure(EIP_UINT8 * pa_Data, int pa_DataLength,
    S_CIP_CPF_Data * pa_CPF_data)
{
  int len_count, i, j;

  pa_CPF_data->AddrInfo[0].TypeID = 0;
  pa_CPF_data->AddrInfo[1].TypeID = 0;

  len_count = 0;
  pa_CPF_data->ItemCount = ltohs(&pa_Data);
  len_count += 2;
  if (pa_CPF_data->ItemCount >= 1)
    {
      pa_CPF_data->stAddr_Item.TypeID = ltohs(&pa_Data);
      pa_CPF_data->stAddr_Item.Length = ltohs(&pa_Data);
      len_count += 4;
      if (pa_CPF_data->stAddr_Item.Length >= 4)
        {
          pa_CPF_data->stAddr_Item.Data.ConnectionIdentifier = ltohl(&pa_Data);
          len_count += 4;
        }
      if (pa_CPF_data->stAddr_Item.Length == 8)
        {
          pa_CPF_data->stAddr_Item.Data.SequenceNumber = ltohl(&pa_Data);
          len_count += 4;
        }
    }
  if (pa_CPF_data->ItemCount >= 2)
    {
      pa_CPF_data->stDataI_Item.TypeID = ltohs(&pa_Data);
      pa_CPF_data->stDataI_Item.Length = ltohs(&pa_Data);
      pa_CPF_data->stDataI_Item.Data = pa_Data;
      pa_Data += pa_CPF_data->stDataI_Item.Length;
      len_count += (4 + pa_CPF_data->stDataI_Item.Length);
    }
  for (j = 0; j < (pa_CPF_data->ItemCount - 2); j++) /* TODO there needs to be a limit check here???*/
    {
      pa_CPF_data->AddrInfo[j].TypeID = ltohs(&pa_Data);
      len_count += 2;
      if ((pa_CPF_data->AddrInfo[j].TypeID == CIP_ITEM_ID_SOCKADDRINFO_O_TO_T)
          || (pa_CPF_data->AddrInfo[j].TypeID
              == CIP_ITEM_ID_SOCKADDRINFO_T_TO_O))
        {
          pa_CPF_data->AddrInfo[j].Length = ltohs(&pa_Data);
          pa_CPF_data->AddrInfo[j].nsin_family = ltohs(&pa_Data);
          pa_CPF_data->AddrInfo[j].nsin_port = ltohs(&pa_Data);
          pa_CPF_data->AddrInfo[j].nsin_addr = ltohl(&pa_Data);
          for (i = 0; i < 8; i++)
            {
              pa_CPF_data->AddrInfo[j].nasin_zero[i] = *pa_Data;
              pa_Data++;
            }
          len_count += 18;
        }
      else
        { /* no sockaddr item found */
          pa_CPF_data->AddrInfo[j].TypeID = 0; /* mark as not set */
          pa_Data -= 2;
        }
    }
  /* set the addressInfoItems to not set if they werent received */
  if (pa_CPF_data->ItemCount < 4)
    {
      pa_CPF_data->AddrInfo[1].TypeID = 0;
      if (pa_CPF_data->ItemCount < 3)
        {
          pa_CPF_data->AddrInfo[0].TypeID = 0;
        }
    }
  if (len_count == pa_DataLength)
    { /* length of data is equal to length of Addr and length of Data */
      return EIP_OK;
    }
  else
    {
      OPENER_TRACE_WARN("something is wrong with the length in MR @ createCPFstructure\n");
      if (pa_CPF_data->ItemCount > 2)
        {
          /* there is an optional packet in data stream which is not sockaddr item */
          return EIP_OK;
        }
      else
        { /* something with the length was wrong */
          return EIP_ERROR;
        }
    }
}

/*   INT8 assembleLinearMsg(S_CIP_MR_Response *pa_MRResponse, S_CIP_CPF_Data *pa_CPFDataItem, INT8 *pa_msg)
 *   copy data from MRResponse struct and CPFDataItem into linear memory in pa_msg for transmission over in encapsulation.
 *      pa_MRResponse	pointer to message router response which has to be aligned into linear memory.
 *      pa_CPFDataItem	pointer to CPF structure which has to be aligned into linear memory.
 *      pa_msg		pointer to linear memory.
 *  return length of reply in pa_msg in bytes
 * 			-1 .. error
 */
int
assembleLinearMsg(S_CIP_MR_Response * pa_MRResponse,
    S_CIP_CPF_Data * pa_CPFDataItem, EIP_UINT8 * pa_msg)
{
  int i, j, size;

  size = 0;
  if (pa_MRResponse)
    {
      /* add Interface Handle and Timeout = 0 -> only for SendRRData and SendUnitData necessary */
      htoll(0, &pa_msg);
      htols(0, &pa_msg);
      size += 6;
    }

  htols(pa_CPFDataItem->ItemCount, &pa_msg); /* item count */
  size += 2;
  /* process Address Item */
  if (pa_CPFDataItem->stAddr_Item.TypeID == CIP_ITEM_ID_NULL)
    { /* null address item -> address length set to 0 */
      htols(CIP_ITEM_ID_NULL, &pa_msg);
      htols(0, &pa_msg);
      size += 4;
    }
  if (pa_CPFDataItem->stAddr_Item.TypeID == CIP_ITEM_ID_CONNECTIONBASED)
    { /* connected data item -> address length set to 4 and copy ConnectionIdentifier */
      htols(CIP_ITEM_ID_CONNECTIONBASED, &pa_msg);
      htols(4, &pa_msg);
      htoll(pa_CPFDataItem->stAddr_Item.Data.ConnectionIdentifier, &pa_msg);
      size += 8;
    }
  /* sequencenumber????? */
  if (pa_CPFDataItem->stAddr_Item.TypeID == CIP_ITEM_ID_SEQUENCEDADDRESS)
    { /* sequenced address item -> address length set to 8 and copy ConnectionIdentifier and SequenceNumber */
      htols(CIP_ITEM_ID_SEQUENCEDADDRESS, &pa_msg);
      htols(8, &pa_msg);
      htoll(pa_CPFDataItem->stAddr_Item.Data.ConnectionIdentifier, &pa_msg);
      htoll(pa_CPFDataItem->stAddr_Item.Data.SequenceNumber, &pa_msg);
      size += 12;
    }

  /* process Data Item */
  if ((pa_CPFDataItem->stDataI_Item.TypeID == CIP_ITEM_ID_UNCONNECTEDMESSAGE)
      || (pa_CPFDataItem->stDataI_Item.TypeID
          == CIP_ITEM_ID_CONNECTIONTRANSPORTPACKET))
    {
      if (pa_MRResponse)
        {
          htols(pa_CPFDataItem->stDataI_Item.TypeID, &pa_msg);

          if (pa_CPFDataItem->stDataI_Item.TypeID
              == CIP_ITEM_ID_CONNECTIONTRANSPORTPACKET)
            {
              htols((EIP_UINT16) (pa_MRResponse->DataLength + 4 + 2 + (2
                  * pa_MRResponse->SizeofAdditionalStatus)), &pa_msg);

              htols(
                  (EIP_UINT16) g_stCPFDataItem.stAddr_Item.Data.SequenceNumber,
                  &pa_msg);

              size += (4 + pa_MRResponse->DataLength + 4 + 2 + (2
                  * pa_MRResponse->SizeofAdditionalStatus));
            }
          else
            {
              htols((EIP_UINT16) (pa_MRResponse->DataLength + 4 + (2
                  * pa_MRResponse->SizeofAdditionalStatus)), &pa_msg);
              size += (4 + pa_MRResponse->DataLength + 4 + (2
                  * pa_MRResponse->SizeofAdditionalStatus));
            }

          /* write MR Response into linear memory */
          *pa_msg = pa_MRResponse->ReplyService;
          pa_msg++;
          *pa_msg = pa_MRResponse->Reserved; /* reserved = 0 */
          pa_msg++;
          *pa_msg = pa_MRResponse->GeneralStatus;
          pa_msg++;
          *pa_msg = pa_MRResponse->SizeofAdditionalStatus;
          pa_msg++;
          for (i = 0; i < pa_MRResponse->SizeofAdditionalStatus; i++)
            htols(pa_MRResponse->AdditionalStatus[i], &pa_msg);

          for (i = 0; i < pa_MRResponse->DataLength; i++)
            {
              *pa_msg = (EIP_UINT8) *(pa_MRResponse->Data + i);
              pa_msg++;
            }
        }
      else
        { /* connected IO Message to send */
          htols(pa_CPFDataItem->stDataI_Item.TypeID, &pa_msg);
          htols(pa_CPFDataItem->stDataI_Item.Length, &pa_msg);
          for (i = 0; i < pa_CPFDataItem->stDataI_Item.Length; i++)
            {
              *pa_msg = (EIP_UINT8) *(pa_CPFDataItem->stDataI_Item.Data + i);
              pa_msg++;
            }
          size += (pa_CPFDataItem->stDataI_Item.Length + 4);
        }
    }
  /* process SockAddr Info Items */
  for (j = 0; j < 2; j++)
    {
      if ((pa_CPFDataItem->AddrInfo[j].TypeID
          == CIP_ITEM_ID_SOCKADDRINFO_O_TO_T)
          || (pa_CPFDataItem->AddrInfo[j].TypeID
              == CIP_ITEM_ID_SOCKADDRINFO_T_TO_O))
        {
          htols(pa_CPFDataItem->AddrInfo[j].TypeID, &pa_msg);
          htols(pa_CPFDataItem->AddrInfo[j].Length, &pa_msg);
          htols(pa_CPFDataItem->AddrInfo[j].nsin_family, &pa_msg);
          htols(pa_CPFDataItem->AddrInfo[j].nsin_port, &pa_msg);
          htoll(pa_CPFDataItem->AddrInfo[j].nsin_addr, &pa_msg);
          for (i = 0; i < 8; i++)
            {
              *pa_msg = 0; /* sin_zero */
              pa_msg++;
            }
          size += 20;
        }
    }
  return size;
}

