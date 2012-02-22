/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include "cipioconnection.h"
#include "cipconnectionmanager.h"
#include "cipassembly.h"
#include "cipcommon.h"
#include "appcontype.h"
#include "cpf.h"
#include "trace.h"
#include "endianconv.h"
#include <string.h>

/*The port to be used per default for I/O messages on UDP.*/
#define OPENER_EIP_IO_UDP_PORT   0x08AE

/* values needed from tcp/ip interface */
extern EIP_UINT32 g_nMultiCastAddress;

/* producing multicast connection have to consider the rules that apply for
 * application connection types.
 */
EIP_STATUS
openProducingMulticastConnection(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_CPF_Data *pa_CPF_data);

EIP_STATUS
OpenMulticastConnection(int pa_direction,
    S_CIP_ConnectionObject *pa_pstConnObj, S_CIP_CPF_Data *pa_CPF_data);

EIP_STATUS
OpenConsumingPointToPointConnection(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_CPF_Data *pa_CPF_data);

int
OpenProducingPointToPointConnection(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_CPF_Data *pa_CPF_data);

EIP_UINT16
handleConfigData(S_CIP_Class *pa_pstAssemblyClass,
    S_CIP_ConnectionObject *pa_pstIOConnObj);

/* Regularly close the IO connection. If it is an exclusive owner or input only
 * connection and in charge of the connection a new owner will be searched
 */
void
closeIOConnection(S_CIP_ConnectionObject *pa_pstConnObjData);

void
handleIOConnectionTimeOut(S_CIP_ConnectionObject *pa_pstConn);

/*!  Send the data from the produced CIP Object of the connection via the socket of the connection object
 *   on UDP.
 *      @param pa_pstConnection  pointer to the connection object
 *      @return status  EIP_OK .. success
 *                     EIP_ERROR .. error
 */
EIP_STATUS
sendConnectedData(S_CIP_ConnectionObject *pa_pstConnection);

EIP_STATUS
handleReceivedIOConnData(struct CIP_ConnectionObject *pa_pstConnection,
    EIP_UINT8 * pa_pnData, EIP_UINT16 pa_nDataLength);

/**** Global variables ****/

/* buffers for the config data coming with a forward open request.
 */
EIP_UINT8 *g_pnConfigDataBuffer = NULL;
unsigned int g_unConfigDataLen = 0;

/*! buffer for holding the run idle information.                             */

EIP_UINT32 g_nRunIdleState;

/**** Implementation ****/

int
establishIOConnction(struct CIP_ConnectionObject *pa_pstConnObjData,
    EIP_UINT16 *pa_pnExtendedError)
{
  int O2TConnectionType, T2OConnectionType;
  int nRetVal = EIP_OK;
  S_CIP_attribute_struct *pstAttribute;
  /* currently we allow I/O connections only to assembly objects */
  S_CIP_Class *pstAssemblyClass = getCIPClass(CIP_ASSEMBLY_CLASS_CODE); /* we don't need to check for zero as this is handled in the connection path parsing */
  S_CIP_Instance *pstInstance = NULL;

  S_CIP_ConnectionObject *pstIOConnObj = getIOConnectionForConnectionData(
      pa_pstConnObjData, pa_pnExtendedError);

  if (NULL == pstIOConnObj)
    {
      return CIP_ERROR_CONNECTION_FAILURE;
    }

  //TODO add check for transport type trigger

  if (CIP_CONN_CYCLIC_CONNECTION != (pstIOConnObj->TransportTypeClassTrigger
      & CIP_CONN_PRODUCTION_TRIGGER_MASK))
    {
      if (256 == pstIOConnObj->m_unProductionInhibitTime)
        {
          /* there was no PIT segment in the connection path set PIT to one fourth of RPI */
          pstIOConnObj->m_unProductionInhibitTime
              = ((EIP_UINT16) (pstIOConnObj->T_to_O_RPI) / 4000);
        }
      else
        {
          /* if production inhibit time has been provided it needs to be smaller than the RPI */
          if (pstIOConnObj->m_unProductionInhibitTime
              > ((EIP_UINT16) ((pstIOConnObj->T_to_O_RPI) / 1000)))
            {
              /* see section C-1.4.3.3 */
              *pa_pnExtendedError = 0x111;
              return 0x01;
            }
        }
    }
  /* set the connection call backs */
  pstIOConnObj->m_pfCloseFunc = closeIOConnection;
  pstIOConnObj->m_pfTimeOutFunc = handleIOConnectionTimeOut;
  pstIOConnObj->m_pfSendDataFunc = sendConnectedData;
  pstIOConnObj->m_pfReceiveDataFunc = handleReceivedIOConnData;

  generalConnectionConfiguration(pstIOConnObj);

  O2TConnectionType
      = (pstIOConnObj->O_to_T_NetworkConnectionParameter & 0x6000) >> 13;
  T2OConnectionType
      = (pstIOConnObj->T_to_O_NetworkConnectionParameter & 0x6000) >> 13;

  if ((O2TConnectionType == 0) && (T2OConnectionType == 0))
    { /* this indicates an re-configuration of the connection currently not supported and we should not come here as this is handled in the forwardopen function*/

    }
  else
    {
      int nProducingIndex = 0;
      int nDataSize;
      if ((O2TConnectionType != 0) && (T2OConnectionType != 0))
        { /* we have a producing and consuming connection*/
          nProducingIndex = 1;
        }

      pstIOConnObj->p_stConsumingInstance = 0;
      pstIOConnObj->ConsumedConnectionPathLength = 0;
      pstIOConnObj->p_stProducingInstance = 0;
      pstIOConnObj->ProducedConnectionPathLength = 0;

      if (O2TConnectionType != 0)
        { /*setup consumer side*/
          if (0 != (pstInstance = getCIPInstance(pstAssemblyClass,
              pstIOConnObj->ConnectionPath.ConnectionPoint[0])))
            { /* consuming Connection Point is present */
              pstIOConnObj->p_stConsumingInstance = pstInstance;

              pstIOConnObj->ConsumedConnectionPathLength = 6;
              pstIOConnObj->ConsumedConnectionPath.PathSize = 6;
              pstIOConnObj->ConsumedConnectionPath.ClassID
                  = pstIOConnObj->ConnectionPath.ClassID;
              pstIOConnObj->ConsumedConnectionPath.InstanceNr
                  = pstIOConnObj->ConnectionPath.ConnectionPoint[0];
              pstIOConnObj->ConsumedConnectionPath.AttributNr = 3;

              pstAttribute = getAttribute(pstInstance, 3);
              OPENER_ASSERT(pstAttribute != NULL); /* an assembly object should always have an attribute 3 */
              nDataSize = pstIOConnObj->ConsumedConnectionSize;

              if ((pstIOConnObj->TransportTypeClassTrigger & 0x0F) == 1)
                {
                  /* class 1 connection */
                  nDataSize -= 2; /* remove 16-bit sequence count length */
                }
              if ((OPENER_CONSUMED_DATA_HAS_RUN_IDLE_HEADER) && (nDataSize > 0))
                { /* we only have an run idle header if it is not an hearbeat connection */
                  nDataSize -= 4; /* remove the 4 bytes needed for run/idle header */
                }

              if (((S_CIP_Byte_Array *) pstAttribute->pt2data)->len
                  != nDataSize)
                {
                  /*wrong connection size */
                  *pa_pnExtendedError
                      = CIP_CON_MGR_ERROR_INVALID_CONNECTION_SIZE;
                  return CIP_ERROR_CONNECTION_FAILURE;
                }
            }
          else
            {
              *pa_pnExtendedError = CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }

      if (T2OConnectionType != 0)
        { /*setup producer side*/
          if (0 != (pstInstance = getCIPInstance(pstAssemblyClass,
              pstIOConnObj->ConnectionPath.ConnectionPoint[nProducingIndex])))
            {
              pstIOConnObj->p_stProducingInstance = pstInstance;

              pstIOConnObj->ProducedConnectionPathLength = 6;
              pstIOConnObj->ProducedConnectionPath.PathSize = 6;
              pstIOConnObj->ProducedConnectionPath.ClassID
                  = pstIOConnObj->ConnectionPath.ClassID;
              pstIOConnObj->ProducedConnectionPath.InstanceNr
                  = pstIOConnObj->ConnectionPath.ConnectionPoint[nProducingIndex];
              pstIOConnObj->ProducedConnectionPath.AttributNr = 3;

              pstAttribute = getAttribute(pstInstance, 3);
              OPENER_ASSERT(pstAttribute != NULL); /* an assembly object should always have an attribute 3 */
              nDataSize = pstIOConnObj->ProducedConnectionSize;
              if ((pstIOConnObj->TransportTypeClassTrigger & 0x0F) == 1)
                {
                  /* class 1 connection */
                  nDataSize -= 2; /* remove 16-bit sequence count length */
                }
              if (((S_CIP_Byte_Array *) pstAttribute->pt2data)->len
                  != nDataSize)
                {
                  /*wrong connection size*/
                  *pa_pnExtendedError
                      = CIP_CON_MGR_ERROR_INVALID_CONNECTION_SIZE;
                  return CIP_ERROR_CONNECTION_FAILURE;
                }

            }
          else
            {
              *pa_pnExtendedError = CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }

      if (NULL != g_pnConfigDataBuffer)
        { /* config data has been sent with this forward open request */
          *pa_pnExtendedError
              = handleConfigData(pstAssemblyClass, pstIOConnObj);
          if (0 != *pa_pnExtendedError)
            {
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }

      nRetVal = openCommunicationChannels(pstIOConnObj);
      if (EIP_OK != nRetVal)
        {
          *pa_pnExtendedError = 0; /*TODO find out the correct extended error code*/
          return nRetVal;
        }
    }

  addNewActiveConnection(pstIOConnObj);
  IApp_IOConnectionEvent(pstIOConnObj->ConnectionPath.ConnectionPoint[0],
      pstIOConnObj->ConnectionPath.ConnectionPoint[1], enOpened);
  return nRetVal;
}

/*   EIP_STATUS OpenPointToPointConnection(S_CIP_CPF_Data *pa_CPF_data, S_CIP_CM_Object *pa_pstCMObject, INT8 pa_direction, int pa_index)
 *   open a Point2Point connection dependent on pa_direction.
 *      pa_pstCMObject  pointer to registered Object in ConnectionManager.
 *      pa_index        index of the connection object
 *  return status
 *               0 .. success
 *              -1 .. error
 */

EIP_STATUS
OpenConsumingPointToPointConnection(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_CPF_Data *pa_CPF_data)
{
  /*static EIP_UINT16 nUDPPort = 2222; TODO think on improving the udp port assigment for point to point connections */
  int j;
  struct sockaddr_in addr;
  int newfd;

  j = 0;
  if (pa_CPF_data->AddrInfo[0].TypeID == 0)
    { /* it is not used yet */
      j = 0;
    }
  else if (pa_CPF_data->AddrInfo[1].TypeID == 0)
    {
      j = 1;
    }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  /*addr.in_port = htons(nUDPPort++);*/
  addr.sin_port = htons(OPENER_EIP_IO_UDP_PORT);

  newfd = IApp_CreateUDPSocket(CONSUMING, &addr); /* the address is only needed for bind used if consuming */
  if (newfd == EIP_INVALID_SOCKET)
    {
      OPENER_TRACE_ERR(
          "cannot create UDP socket in OpenPointToPointConnection\n");
      return EIP_ERROR;
    }

  pa_pstConnObj->m_stOriginatorAddr = addr; /* store the address of the originator for packet scanning */
  addr.sin_addr.s_addr = INADDR_ANY; /* restore the address */
  pa_pstConnObj->sockfd[CONSUMING] = newfd;

  pa_CPF_data->AddrInfo[j].Length = 16;
  pa_CPF_data->AddrInfo[j].TypeID = CIP_ITEM_ID_SOCKADDRINFO_O_TO_T;

  pa_CPF_data->AddrInfo[j].nsin_port = addr.sin_port;
  /*TODO should we add our own address here? */
  pa_CPF_data->AddrInfo[j].nsin_addr = addr.sin_addr.s_addr;
  memset(pa_CPF_data->AddrInfo[j].nasin_zero, 0, 8);
  pa_CPF_data->AddrInfo[j].nsin_family = htons(AF_INET);

  return EIP_OK;
}

int
OpenProducingPointToPointConnection(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_CPF_Data *pa_CPF_data)
{
  int newfd;
  in_port_t nPort = htons(OPENER_EIP_IO_UDP_PORT); /* the default port to be used if no port information is part of the forward open request */

  if (CIP_ITEM_ID_SOCKADDRINFO_T_TO_O == pa_CPF_data->AddrInfo[0].TypeID)
    {
      nPort = pa_CPF_data->AddrInfo[0].nsin_port;
    }
  else
    {
      if (CIP_ITEM_ID_SOCKADDRINFO_T_TO_O == pa_CPF_data->AddrInfo[1].TypeID)
        {
          nPort = pa_CPF_data->AddrInfo[1].nsin_port;
        }
    }

  pa_pstConnObj->remote_addr.sin_family = AF_INET;
  pa_pstConnObj->remote_addr.sin_addr.s_addr = 0; /* we don't know the address of the originate will be set in the IApp_CreateUDPSocket */
  pa_pstConnObj->remote_addr.sin_port = nPort;

  newfd = IApp_CreateUDPSocket(PRODUCING, &pa_pstConnObj->remote_addr); /* the address is only needed for bind used if consuming */
  if (newfd == EIP_INVALID_SOCKET)
    {
      OPENER_TRACE_ERR(
          "cannot create UDP socket in OpenPointToPointConnection\n");
      //*pa_pnExtendedError = 0x0315; /* miscellaneous*/
      return CIP_ERROR_CONNECTION_FAILURE;
    }
  pa_pstConnObj->sockfd[PRODUCING] = newfd;

  return EIP_OK;
}

EIP_STATUS
openProducingMulticastConnection(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_CPF_Data *pa_CPF_data)
{
  S_CIP_ConnectionObject *pstExistingConn = getExistingProdMulticastConnection(
      pa_pstConnObj->ConnectionPath.ConnectionPoint[1]);
  int j;

  if (NULL == pstExistingConn)
    { /* we are the first connection producing for the given Input Assembly */
      return OpenMulticastConnection(PRODUCING, pa_pstConnObj, pa_CPF_data);
    }
  else
    {
      /* we need to infrom our originator on the correct connection id */
      pa_pstConnObj->CIPProducedConnectionID
          = pstExistingConn->CIPProducedConnectionID;
    }

  /* we have a connection reuse the data and the socket */

  j = 0; /* allocate an unused sockaddr struct to use */
  if (g_stCPFDataItem.AddrInfo[0].TypeID == 0)
    { /* it is not used yet */
      j = 0;
    }
  else if (g_stCPFDataItem.AddrInfo[1].TypeID == 0)
    {
      j = 1;
    }

  if (enConnTypeIOExclusiveOwner == pa_pstConnObj->m_eInstanceType)
    {
      /* eclusive owners take the socket and further manage the connection
       * especially in the case of time outs.
       */
      pa_pstConnObj->sockfd[PRODUCING] = pstExistingConn->sockfd[PRODUCING];
      pstExistingConn->sockfd[PRODUCING] = EIP_INVALID_SOCKET;
    }
  else
    { /* this connection will not produce the data */
      pa_pstConnObj->sockfd[PRODUCING] = EIP_INVALID_SOCKET;
    }

  pa_CPF_data->AddrInfo[j].Length = 16;
  pa_CPF_data->AddrInfo[j].TypeID = CIP_ITEM_ID_SOCKADDRINFO_T_TO_O;
  pa_pstConnObj->remote_addr.sin_family = AF_INET;
  pa_pstConnObj->remote_addr.sin_port = pa_CPF_data->AddrInfo[j].nsin_port
      = htons(OPENER_EIP_IO_UDP_PORT);
  pa_pstConnObj->remote_addr.sin_addr.s_addr
      = pa_CPF_data->AddrInfo[j].nsin_addr = g_nMultiCastAddress;
  memset(pa_CPF_data->AddrInfo[j].nasin_zero, 0, 8);
  pa_CPF_data->AddrInfo[j].nsin_family = htons(AF_INET);

  return EIP_OK;
}

/*   INT8 OpenMulticastConnection(S_CIP_CPF_Data *pa_CPF_data, S_CIP_CM_Object *pa_pstCMObject, INT8 pa_direction, int pa_index)
 *   open a Multicast connection dependent on pa_direction.
 *      pa_CPF_data     received CPF Data Item.
 *      pa_pstCMObject  pointer to registered Object in ConnectionManager.
 *      pa_direction    flag to indicate if consuming or producing.
 *      pa_index        index of the connection object
 *  return status
 *               0 .. success
 *              -1 .. error
 */
EIP_STATUS
OpenMulticastConnection(int pa_direction,
    S_CIP_ConnectionObject *pa_pstConnObj, S_CIP_CPF_Data *pa_CPF_data)
{
  int j;
  struct sockaddr_in addr;
  int newfd;

  j = 0; /* allocate an unused sockaddr struct to use */
  if (g_stCPFDataItem.AddrInfo[0].TypeID == 0)
    { /* it is not used yet */
      j = 0;
    }
  else if (g_stCPFDataItem.AddrInfo[1].TypeID == 0)
    {
      j = 1;
    }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = g_nMultiCastAddress;
  addr.sin_port = htons(OPENER_EIP_IO_UDP_PORT);

  newfd = IApp_CreateUDPSocket(pa_direction, &addr); /* the address is only needed for bind used if consuming */
  if (newfd == EIP_INVALID_SOCKET)
    {
      OPENER_TRACE_ERR("cannot create UDP socket in OpenMulticastConnection\n");
      return EIP_ERROR;
    }
  pa_pstConnObj->sockfd[pa_direction] = newfd;

  pa_CPF_data->AddrInfo[j].Length = 16;
  if (pa_direction == CONSUMING)
    {
      pa_CPF_data->AddrInfo[j].TypeID = CIP_ITEM_ID_SOCKADDRINFO_O_TO_T;
      pa_pstConnObj->m_stOriginatorAddr = addr;
      addr.sin_addr.s_addr = g_nMultiCastAddress; /* restore the multicast address */
    }
  else
    {
      pa_CPF_data->AddrInfo[j].TypeID = CIP_ITEM_ID_SOCKADDRINFO_T_TO_O;
      pa_pstConnObj->remote_addr = addr;
    }
  pa_CPF_data->AddrInfo[j].nsin_port = addr.sin_port;
  pa_CPF_data->AddrInfo[j].nsin_addr = addr.sin_addr.s_addr;
  memset(pa_CPF_data->AddrInfo[j].nasin_zero, 0, 8);
  pa_CPF_data->AddrInfo[j].nsin_family = htons(AF_INET);

  return EIP_OK;
}

EIP_UINT16
handleConfigData(S_CIP_Class *pa_pstAssemblyClass,
    S_CIP_ConnectionObject *pa_pstIOConnObj)
{
  EIP_UINT16 unRetVal = 0;
  S_CIP_Instance *pstConfigInstance = getCIPInstance(pa_pstAssemblyClass,
      pa_pstIOConnObj->ConnectionPath.ConnectionPoint[2]);

  if (0 != g_unConfigDataLen)
    {
      if (connectionWithSameConfigPointExists(
          pa_pstIOConnObj->ConnectionPath.ConnectionPoint[2]))
        { /* there is a connected connection with the same config point
         * we have to have the same data as already present in the config point*/
          S_CIP_Byte_Array *p = (S_CIP_Byte_Array *) getAttribute(
              pstConfigInstance, 3)->pt2data;
          if (p->len != g_unConfigDataLen)
            {
              unRetVal = CIP_CON_MGR_ERROR_OWNERSHIP_CONFLICT;
            }
          else
            {
              /*FIXME check if this is correct */
              if (memcmp(p->Data, g_pnConfigDataBuffer, g_unConfigDataLen))
                {
                  unRetVal = CIP_CON_MGR_ERROR_OWNERSHIP_CONFLICT;
                }
            }
        }
      else
        {
          /*put the data on the configuration assembly object with the current
           design this can be done rather efficiently */
          if (EIP_OK != notifyAssemblyConnectedDataReceived(pstConfigInstance,
              g_pnConfigDataBuffer, g_unConfigDataLen))
            {
              OPENER_TRACE_WARN("Configuration data was invalid\n");
              unRetVal = CIP_CON_MGR_ERROR_INVALID_CONFIGURATION_FORMAT;
            }
        }
    }
  return unRetVal;
}

void
closeIOConnection(S_CIP_ConnectionObject *pa_pstConnObjData)
{
  S_CIP_ConnectionObject *pstNextNonCtrlMasterCon;

  IApp_IOConnectionEvent(pa_pstConnObjData->ConnectionPath.ConnectionPoint[0],
      pa_pstConnObjData->ConnectionPath.ConnectionPoint[1], enClosed);

  if ((enConnTypeIOExclusiveOwner == pa_pstConnObjData->m_eInstanceType)
      || (enConnTypeIOInputOnly == pa_pstConnObjData->m_eInstanceType))
    {
      if ((CIP_MULTICAST_CONNECTION
          == (pa_pstConnObjData->T_to_O_NetworkConnectionParameter
              & CIP_MULTICAST_CONNECTION)) && (EIP_INVALID_SOCKET
          != pa_pstConnObjData->sockfd[PRODUCING]))
        {
          pstNextNonCtrlMasterCon = getNextNonCtrlMasterCon(
              pa_pstConnObjData->ConnectionPath.ConnectionPoint[1]);
          if (NULL != pstNextNonCtrlMasterCon)
            {
              pstNextNonCtrlMasterCon->sockfd[PRODUCING]
                  = pa_pstConnObjData->sockfd[PRODUCING];
              pa_pstConnObjData->sockfd[PRODUCING] = EIP_INVALID_SOCKET;
              pstNextNonCtrlMasterCon->TransmissionTriggerTimer
                  = pa_pstConnObjData->TransmissionTriggerTimer;
            }
          else
            { /* this was the last master connection close all listen only connections listening on the port */
              closeAllConnsForInputWithSameType(
                  pa_pstConnObjData->ConnectionPath.ConnectionPoint[1],
                  enConnTypeIOListenOnly);
            }
        }
    }

  closeCommChannelsAndRemoveFromActiveConnsList(pa_pstConnObjData);
}

void
handleIOConnectionTimeOut(S_CIP_ConnectionObject *pa_pstConn)
{
  S_CIP_ConnectionObject *pstNextNonCtrlMasterCon;
  IApp_IOConnectionEvent(pa_pstConn->ConnectionPath.ConnectionPoint[0],
      pa_pstConn->ConnectionPath.ConnectionPoint[1], enTimedOut);

  if (CIP_MULTICAST_CONNECTION
      == (pa_pstConn->T_to_O_NetworkConnectionParameter
          & CIP_MULTICAST_CONNECTION))
    {
      switch (pa_pstConn->m_eInstanceType)
        {
      case enConnTypeIOExclusiveOwner:
        closeAllConnsForInputWithSameType(
            pa_pstConn->ConnectionPath.ConnectionPoint[1],
            enConnTypeIOInputOnly);
        closeAllConnsForInputWithSameType(
            pa_pstConn->ConnectionPath.ConnectionPoint[1],
            enConnTypeIOListenOnly);
        break;
      case enConnTypeIOInputOnly:
        if (EIP_INVALID_SOCKET != pa_pstConn->sockfd[PRODUCING])
          { /* we are the controlling input only connection find a new controller*/
            pstNextNonCtrlMasterCon = getNextNonCtrlMasterCon(
                pa_pstConn->ConnectionPath.ConnectionPoint[1]);
            if (NULL != pstNextNonCtrlMasterCon)
              {
                pstNextNonCtrlMasterCon->sockfd[PRODUCING]
                    = pa_pstConn->sockfd[PRODUCING];
                pa_pstConn->sockfd[PRODUCING] = EIP_INVALID_SOCKET;
                pstNextNonCtrlMasterCon->TransmissionTriggerTimer
                    = pa_pstConn->TransmissionTriggerTimer;
              }
            else
              { /* this was the last master connection close all listen only connections listening on the port */
                closeAllConnsForInputWithSameType(
                    pa_pstConn->ConnectionPath.ConnectionPoint[1],
                    enConnTypeIOListenOnly);
              }
          }
        break;
      default:
        break;
        }
    }

  OPENER_ASSERT(NULL != pa_pstConn->m_pfCloseFunc);
  pa_pstConn->m_pfCloseFunc(pa_pstConn);
}

EIP_STATUS
sendConnectedData(S_CIP_ConnectionObject *pa_pstConnection)
{
  S_CIP_CPF_Data *pCPFDataItem;
  S_CIP_Byte_Array *p;
  EIP_UINT16 replylength;
  EIP_UINT8 *pnBuf;
  int i;

  /* TODO think of adding an own send buffer to each connection object in order to preset up the whole message on connection opening and just change the variable data items e.g., sequence number */

  pCPFDataItem = &g_stCPFDataItem; /* TODO think on adding a CPF data item to the S_CIP_ConnectionObject in order to remove the code here or even better allocate memory in the connection object for storing the message to send and just change the application data*/

  pa_pstConnection->EIPSequenceCountProducing++;

  /* assembleCPFData */
  pCPFDataItem->ItemCount = 2;
  if ((pa_pstConnection->TransportTypeClassTrigger & 0x0F) != 0)
    { /* use Sequenced Address Items if not Connection Class 0 */
      pCPFDataItem->stAddr_Item.TypeID = CIP_ITEM_ID_SEQUENCEDADDRESS;
      pCPFDataItem->stAddr_Item.Length = 8;
      pCPFDataItem->stAddr_Item.Data.SequenceNumber
          = pa_pstConnection->EIPSequenceCountProducing;
    }
  else
    {
      pCPFDataItem->stAddr_Item.TypeID = CIP_ITEM_ID_CONNECTIONBASED;
      pCPFDataItem->stAddr_Item.Length = 4;

    }
  pCPFDataItem->stAddr_Item.Data.ConnectionIdentifier
      = pa_pstConnection->CIPProducedConnectionID;

  pCPFDataItem->stDataI_Item.TypeID = CIP_ITEM_ID_CONNECTIONTRANSPORTPACKET;

  p
      = (S_CIP_Byte_Array *) pa_pstConnection->p_stProducingInstance->pstAttributes->pt2data;
  pCPFDataItem->stDataI_Item.Length = 0;

  /* notify the application that data will be sent immediately after the call */
  if (IApp_BeforeAssemblyDataSend(pa_pstConnection->p_stProducingInstance))
    {
      /* the data has changed increase sequence counter */
      pa_pstConnection->SequenceCountProducing++;
    }

  /* set AddressInfo Items to invalid Type */
  pCPFDataItem->AddrInfo[0].TypeID = 0;
  pCPFDataItem->AddrInfo[1].TypeID = 0;

  replylength = assembleLinearMsg(0, pCPFDataItem,
      &g_acMessageDataReplyBuffer[0]);

  pnBuf = &g_acMessageDataReplyBuffer[replylength - 2];
  pCPFDataItem->stDataI_Item.Length = p->len;
  if ((pa_pstConnection->TransportTypeClassTrigger & 0x0F) == 1)
    {
      pCPFDataItem->stDataI_Item.Length += 2;
      htols(pCPFDataItem->stDataI_Item.Length, &pnBuf);
      htols(pa_pstConnection->SequenceCountProducing, &pnBuf);
    }
  else
    {
      htols(pCPFDataItem->stDataI_Item.Length, &pnBuf);
    }

  for (i = 0; i < p->len; i++)
    {
      *pnBuf = (EIP_UINT8) *(p->Data + i);
      pnBuf++;
    }

  replylength += pCPFDataItem->stDataI_Item.Length;

  return IApp_SendUDPData(&pa_pstConnection->remote_addr,
      pa_pstConnection->sockfd[PRODUCING], &g_acMessageDataReplyBuffer[0],
      replylength);
}

EIP_STATUS
handleReceivedIOConnData(struct CIP_ConnectionObject *pa_pstConnection,
    EIP_UINT8 * pa_pnData, EIP_UINT16 pa_nDataLength)
{

  /* check class 1 sequence number*/
  if ((pa_pstConnection->TransportTypeClassTrigger & 0x0F) == 1)
    {
      EIP_UINT16 nSequenceBuf = ltohs(&(pa_pnData));
      if (SEQ_LEQ16(nSequenceBuf, pa_pstConnection->SequenceCountConsuming))
        {
          return EIP_OK; /* no new data for the assembly */
        }
      pa_pstConnection->SequenceCountConsuming = nSequenceBuf;
      pa_nDataLength -= 2;
    }

  if (pa_nDataLength > 0)
    {
      /* we have no heartbeat connection */
      if (OPENER_CONSUMED_DATA_HAS_RUN_IDLE_HEADER)
        {
          EIP_UINT32 nRunIdleBuf = ltohl(&(pa_pnData));
          if (g_nRunIdleState != nRunIdleBuf)
            {
              IApp_RunIdleChanged(nRunIdleBuf);
            }
          g_nRunIdleState = nRunIdleBuf;
          pa_nDataLength -= 4;
        }

      if (notifyAssemblyConnectedDataReceived(
          pa_pstConnection->p_stConsumingInstance, pa_pnData, pa_nDataLength)
          != 0)
        {
          return EIP_ERROR;
        }
    }
  return EIP_OK;
}

int
openCommunicationChannels(struct CIP_ConnectionObject *pa_pstIOConnObj)
{
  int O2TConnectionType, T2OConnectionType;
  int nRetVal = EIP_OK;
  /*get pointer to the cpf data, currently we have just one global instance of the struct. This may change in the future*/
  S_CIP_CPF_Data *pstCPF_data = &g_stCPFDataItem;

  O2TConnectionType = (pa_pstIOConnObj->O_to_T_NetworkConnectionParameter
      & 0x6000) >> 13;
  T2OConnectionType = (pa_pstIOConnObj->T_to_O_NetworkConnectionParameter
      & 0x6000) >> 13;

  /* open a connection "point to point" or "multicast" based on the ConnectionParameter */
  if (O2TConnectionType == 1) /* Multicast consuming */
    {
      if (OpenMulticastConnection(CONSUMING, pa_pstIOConnObj, pstCPF_data)
          == EIP_ERROR)
        {
          OPENER_TRACE_ERR("error in OpenMulticast Connection\n");
          return CIP_ERROR_CONNECTION_FAILURE;
        }
    }
  else if (O2TConnectionType == 2) /* Point to Point consuming */
    {
      if (OpenConsumingPointToPointConnection(pa_pstIOConnObj, pstCPF_data)
          == EIP_ERROR)
        {
          OPENER_TRACE_ERR("error in PointToPoint consuming connection\n");
          return CIP_ERROR_CONNECTION_FAILURE;
        }
    }

  if (T2OConnectionType == 1) /* Multicast producing */
    {
      if (openProducingMulticastConnection(pa_pstIOConnObj, pstCPF_data)
          == EIP_ERROR)
        {
          OPENER_TRACE_ERR("error in OpenMulticast Connection\n");
          return CIP_ERROR_CONNECTION_FAILURE;
        }
    }
  else if (T2OConnectionType == 2) /* Point to Point producing */
    {

      if (OpenProducingPointToPointConnection(pa_pstIOConnObj, pstCPF_data)
          != EIP_OK)
        {
          OPENER_TRACE_ERR("error in PointToPoint producing connection\n");
          return CIP_ERROR_CONNECTION_FAILURE;
        }
    }

  return nRetVal;
}

void
closeCommChannelsAndRemoveFromActiveConnsList(
    struct CIP_ConnectionObject *pa_pstConnObjData)
{
  IApp_CloseSocket(pa_pstConnObjData->sockfd[CONSUMING]);
  pa_pstConnObjData->sockfd[CONSUMING] = EIP_INVALID_SOCKET;
  IApp_CloseSocket(pa_pstConnObjData->sockfd[PRODUCING]);
  pa_pstConnObjData->sockfd[PRODUCING] = EIP_INVALID_SOCKET;

  removeFromActiveConnections(pa_pstConnObjData);
}
