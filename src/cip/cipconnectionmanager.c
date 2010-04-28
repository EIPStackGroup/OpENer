/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <string.h>

#include "opener_user_conf.h"
#include "cipconnectionmanager.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "opener_api.h"
#include "cpf.h"
#include "cipassembly.h"
#include "encap.h"
#include "cipidentity.h"
#include "trace.h"
#include "appcontype.h"

/* values needed from cipidentiy */
extern EIP_UINT16 VendorID;
extern EIP_UINT16 DeviceType;
extern EIP_UINT16 ProductCode;
extern S_CIP_Revision Revison;

/* values need from tcpipinterface */
extern EIP_UINT32 g_nMultiCastAddress;

/* Connection Manager Error codes */
#define CIP_CON_MGR_SUCCESS 0x00
#define CIP_CON_MGR_ERROR_CONNECTION_IN_USE 0x0100
#define CIP_CON_MGR_ERROR_TRANSPORT_TRIGGER_NOT_SUPPORTED 0x0103
#define CIP_CON_MGR_ERROR_OWNERSHIP_CONFLICT 0x0106
#define CIP_CON_MGR_ERROR_CONNECTION_NOT_FOUND_AT_TARGET_APPLICATION 0x0107
#define CIP_CON_MGR_ERROR_INVALID_CONNECTION_TYPE 0x0108
#define CIP_CON_MGR_ERROR_INVALID_CONNECTION_SIZE 0x0109  
#define CIP_CON_MGR_ERROR_NO_MORE_CONNECTIONS_AVAILABLE 0x0113
#define CIP_CON_MGR_ERROR_VENDERID_OR_PRODUCTCODE_ERROR 0x0114
#define CIP_CON_MGR_ERROR_VENDERID_OR_PRODUCT_TYPE_ERROR 0x0115
#define CIP_CON_MGR_ERROR_REVISION_MISMATCH 0x0116
#define CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT 0x0117
#define CIP_CON_MGR_ERROR_INVALID_CONFIGURATION_FORMAT 0x0118
#define CIP_CON_MGR_ERROR_PARAMETER_ERROR_IN_UNCONNECTED_SEND_SERVICE 0x0205
#define CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH 0x0315

#define CIP_CONN_PATH_INVALID 1
#define CIP_CONN_PATH_CONFIGURATION 2
#define CIP_CONN_PATH_CONSUMPTION 3
#define CIP_CONN_PATH_CONFIGURATION_AND_CONSUMPTION 4
#define CIP_CONN_PATH_PRODUCTION 5
#define CIP_CONN_PATH_CONFIGURATION_AND_PRODUCTION 6
#define CIP_CONN_PATH_CONSUMTION_AND_PRODUCTION 7
#define CIP_CONN_PATH_CONFIGURATION_AND_CONSUMPTION_AND_PRODUCTION 8

#define CIP_CONN_TYPE_MASK 0x6000   /*Bit 13&14 true*/

#define FORWARD_OPEN_HEADER_LENGTH 36         /* the length in bytes of the forward open command specific data till the start of the connection path (including con path size)*/
#define EQLOGICALPATH(x,y) (((x)&0xfc)==(y))

/*macros for comparing sequence numbers according to CIP spec vol 2 3-4.2*/
#define SEQ_LEQ32(a, b) ((int)((a) - (b)) <= 0)
#define SEQ_GEQ32(a, b) ((int)((a) - (b)) >= 0)

/* similar macros for comparing 16 bit sequence numbers */
#define SEQ_LEQ16(a, b) ((short)((a) - (b)) <= 0)
#define SEQ_GEQ16(a, b) ((short)((a) - (b)) >= 0) 

/*The port to be used per default for I/O messages on UDP.*/
#define OPENER_EIP_IO_UDP_PORT   0x08AE

/* global variables private */

/*! buffer for holding the run idle information.*/
EIP_UINT32 g_nRunIdleState;

/*! List holding all currently active connections*/
S_CIP_ConnectionObject *g_pstActiveConnectionList = NULL;

/*! buffer connection object needed for forward open */
S_CIP_ConnectionObject g_stDummyConnectionObject;

/*!Array of the available explicit connections */
S_CIP_ConnectionObject
    g_astExplicitConnections[OPENER_CIP_NUM_EXPLICIT_CONNS];

/* buffers for the config data coming with a forward open request.
 */
EIP_UINT8 *g_pnConfigDataBuffer;
unsigned int g_unConfigDataLen;

/*! value holds the connection ID's "incarnation ID" in the upper 16 bits */
EIP_UINT32 g_nIncarnationID;

/* private functions */
EIP_STATUS
ForwardOpen(S_CIP_Instance * pa_pstInstance, S_CIP_MR_Request * pa_MRRequest,
    S_CIP_MR_Response * pa_MRResponse, EIP_UINT8 * pa_msg);
EIP_STATUS
ForwardClose(S_CIP_Instance * pa_pstInstance, S_CIP_MR_Request * pa_MRRequest,
    S_CIP_MR_Response * pa_MRResponse, EIP_UINT8 * pa_msg);
EIP_STATUS
UnconnectedSend(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse,
    EIP_UINT8 * pa_msg); /* for originating and routing devices */
EIP_STATUS
GetConnectionOwner(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse,
    EIP_UINT8 * pa_msg);

EIP_STATUS
assembleFWDOpenResponse(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_MR_Response * pa_MRResponse, EIP_UINT8 pa_nGeneralStatus,
    EIP_UINT16 pa_nExtendedStatus, EIP_UINT8 * pa_msg);

EIP_STATUS
assembleFWDCloseResponse(EIP_UINT16 pa_ConnectionSerialNr,
    EIP_UINT16 pa_OriginatorVendorID, EIP_UINT32 pa_OriginatorSerialNr,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse,
    EIP_UINT16 pa_nExtErrorCode, EIP_UINT8 * pa_msg);

S_CIP_ConnectionObject *
getFreeExplicitConnection(void);

void
generalConnectionConfiguration(S_CIP_ConnectionObject *pa_pstConnObj);

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
    S_CIP_CPF_Data *pa_CPF_data, EIP_UINT16 *pa_pnExtendedError);

/** \brief check if the data given in the connection object match with an already established connection
 * 
 * The comparison is done according to the definitions in the CIP specification Section 3-5.5.2:
 * The following elements have to be equal: Vendor ID, Connection Serial Number, Originator Serial Number
 * @param pa_pstConnObj connection object containing the comparison elements from the forward open request
 * @return 
 *    - NULL if no equal established connection exists
 *    - pointer to the equal connection object
 */
S_CIP_ConnectionObject *
checkForExistingConnection(S_CIP_ConnectionObject *pa_pstConnObj);

/** \brief Compare the electronic key received with a forward open request with the device's data.
 * 
 * @param pa_nKeyFormat format identifier given in the forward open request
 * @param pa_pstKeyData pointer to the electronic key data recieved in the forward open request
 * @param pa_pnExtStatus the extended error code in case an error happend
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
EIP_STATUS
checkElectronicKeyData(EIP_UINT8 pa_nKeyFormat, S_CIP_KeyData *pa_pstKeyData,
    EIP_UINT16 *pa_pnExtStatus);

/** \brief Parse the connection path of a forward open request
 * 
 * This function will take the connection object and the recieved data stream and parse the connection path.
 * @param pa_pstConnObj pointer to the connection object structure for which the connection should 
 *                      be established
 * @param pa_MRRequest pointer to the received request structre. The position of the data stream pointer has to be at the connection length entry
 * @param pa_pnExtendedError the extended error code in case an error happend
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
EIP_UINT8
parseConnectionPath(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_MR_Request *pa_MRRequest, EIP_UINT16 *pa_pnExtendedError);

/** \brief Check if Class3 connection is available and if yes setup all data.
 *
 * This function can be called after all data has been parsed from the forward open request
 * @param pa_pstConnObj pointer to the connection object structure holding the parsed data from the forward open request
 * @param pa_pnExtendedError the extended error code in case an error happened
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
int
establishClass3Connection(S_CIP_ConnectionObject *pa_pstConnObj,
    EIP_UINT16 *pa_pnExtendedError);

/** \brief Setup all data in order to establish an IO connection
 * 
 * This function can be called after all data has been parsed from the forward open request
 * @param pa_pstConnObjData pointer to the connection object structure holding the parsed data from the forward open request
 * @param pa_pnExtendedError the extended error code in case an error happened
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
int
establishIOConnction(S_CIP_ConnectionObject *pa_pstConnObjData,
    EIP_UINT16 *pa_pnExtendedError);

/* Regularly close the IO connection. If it is an exclusive owner or input only
 * connection and in charge of the connection a new owner will be searched
 */
void
closeIOConnection(S_CIP_ConnectionObject *pa_pstConnObjData);

/*!  Send the data from the produced CIP Object of the connection via the socket of the connection object
 *   on UDP.
 *      @param pa_pstConnection  pointer to the connection object
 *      @return status  EIP_OK .. success
 *                     EIP_ERROR .. error
 */
EIP_STATUS
sendConnectedData(S_CIP_ConnectionObject *pa_pstConnection);

void
addNewActiveConnection(S_CIP_ConnectionObject *pa_pstConn);

void
removeFromeActiveConnections(S_CIP_ConnectionObject *pa_pstConn);

void
handleIOConnectionTimeOut(S_CIP_ConnectionObject *pa_pstConn);

EIP_UINT16
handleConfigData(S_CIP_Class *pa_pstAssemblyClass,
    S_CIP_ConnectionObject *pa_pstIOConnObj);

int
GETPADDEDLOGICALPATH(unsigned char **x)
{
  int tmp;

  tmp = *(*x)++;
  if ((tmp & 3) == 0)
    {
      tmp = *(*x)++;
    }
  else if ((tmp & 3) == 1)
    {
      (*x)++; /* skip pad */
      tmp = *(*x)++;
      tmp |= *(*x)++ << 8;
    }
  else
    {
      OPENER_TRACE_ERR("illegal logical path segment\n");
    }
  return tmp;
}

/*! \brief Generate a new connection Id utilizing the Incarnation Id as
 * descibed in the EIP specs.
 *
 * A unique connectionID is formed from the boot-time-specified "incarnation ID"
 * and the per-new-connection-incremented connection number/counter.
 * @return new connection id
 */
EIP_UINT32 getConnectionId()
{
  static EIP_UINT32 nConnectionId = 18;
  nConnectionId++;
  return (g_nIncarnationID | (nConnectionId & 0x0000FFFF));
}

EIP_STATUS
Connection_Manager_Init(EIP_UINT16 pa_nUniqueConnID)
{
  S_CIP_Class *pstConnectionManager;

  pstConnectionManager = createCIPClass(CIP_CONNECTION_MANAGER_CLASS_CODE, /* class ID*/
  0, /* # of class attributes */
  0xffffffff, /* class getAttributeAll mask */
  0, /* # of class services */
  0, /* # of instance attributes */
  0xffffffff, /* instance getAttributeAll mask */
  4, /* # of instance services */
  1, /* # of instances */
  "connection manager", /* class name */
  1); /* revision */
  if (pstConnectionManager == 0)
    return EIP_ERROR;

  insertService(pstConnectionManager, CIP_FORWARD_OPEN, &ForwardOpen,
      "ForwardOpen");
  insertService(pstConnectionManager, CIP_FORWARD_CLOSE, &ForwardClose,
      "ForwardClose");
  insertService(pstConnectionManager, CIP_GET_CONNECTION_OWNER,
      &GetConnectionOwner, "GetConnectionOwner");
  insertService(pstConnectionManager, CIP_UNCONNECTED_SEND, &UnconnectedSend,
      "UnconnectedSend");

  g_nIncarnationID = ((EIP_UINT32) pa_nUniqueConnID) << 16;

  return EIP_OK;
}

EIP_STATUS
handleReceivedConnectedData(EIP_UINT8 * pa_pnData, int pa_nDataLength)
{
  S_CIP_ConnectionObject *pstConnectionObject;

  if ((createCPFstructure(pa_pnData, pa_nDataLength, &g_stCPFDataItem)) == -1)
    { /* error from createCPFstructure */
      return EIP_ERROR;
    }
  else
    {
      /* check if connected address item or sequenced address item  received, otherwise it is no connected message and should not be here */
      if ((g_stCPFDataItem.stAddr_Item.TypeID == CIP_ITEM_ID_CONNECTIONBASED)
          || (g_stCPFDataItem.stAddr_Item.TypeID
              == CIP_ITEM_ID_SEQUENCEDADDRESS))
        { /* found connected address item or found sequenced address item -> for now the sequence number will be ignored */
          if (g_stCPFDataItem.stDataI_Item.TypeID
              == CIP_ITEM_ID_CONNECTIONTRANSPORTPACKET)
            { /* connected data item received */
              pstConnectionObject = getConnectedObject(
                  g_stCPFDataItem.stAddr_Item.Data.ConnectionIdentifier);
              if (pstConnectionObject == 0)
                return EIP_ERROR;

              if (SEQ_GEQ32(g_stCPFDataItem.stAddr_Item.Data.SequenceNumber, pstConnectionObject->EIPSequenceCountConsuming))
                {
                  /* reset the watchdog timer */
                  pstConnectionObject->InnacitvityWatchdogTimer
                      = (pstConnectionObject->O_to_T_RPI / 1000) << (2
                          + pstConnectionObject->ConnectionTimeoutMultiplier);

                  /* only inform assembly object if the sequence counter is greater or equal */
                  pstConnectionObject->EIPSequenceCountConsuming
                      = g_stCPFDataItem.stAddr_Item.Data.SequenceNumber;
                  /* check class 1 sequence number*/
                  if ((pstConnectionObject->TransportTypeTrigger & 0x0F) == 1)
                    {
                      EIP_UINT16 nSequenceBuf = ltohs(
                          &(g_stCPFDataItem.stDataI_Item.Data));
                      if (SEQ_LEQ16(nSequenceBuf, pstConnectionObject->SequenceCountConsuming))
                        {
                          return EIP_OK; /* no new data for the assembly */
                        }
                      pstConnectionObject->SequenceCountConsuming
                          = nSequenceBuf;
                      g_stCPFDataItem.stDataI_Item.Length -= 2;
                    }

                  if (g_stCPFDataItem.stDataI_Item.Length > 0)
                    {
                      /* we have no heartbeat connection */
                      if (OPENER_CONSUMED_DATA_HAS_RUN_IDLE_HEADER)
                        {
                          EIP_UINT32 nRunIdleBuf = ltohl(
                              &(g_stCPFDataItem.stDataI_Item.Data));
                          if (g_nRunIdleState != nRunIdleBuf)
                            {
                              IApp_RunIdleChanged(nRunIdleBuf);
                            }
                          g_nRunIdleState = nRunIdleBuf;
                          g_stCPFDataItem.stDataI_Item.Length -= 4;
                        }

                      if (notifyAssemblyConnectedDataReceived(
                          pstConnectionObject->p_stConsumingInstance,
                          g_stCPFDataItem.stDataI_Item.Data,
                          g_stCPFDataItem.stDataI_Item.Length) != 0)
                        return EIP_ERROR;
                    }
                }

            }
        }
    }
  return EIP_OK;
}

/*   EIP_STATUS ForwardOpen(S_CIP_Instance *pa_pstInstance, S_CIP_MR_Request *pa_MRRequest, S_CIP_MR_Response *pa_MRResponse, S_CIP_CPF_Data *pa_CPF_data, INT8 *pa_msg)
 *   check if resources for new connection available, generate ForwardOpen Reply message.
 *      pa_pstInstance	pointer to CIP object instance
 *      pa_MRRequest		pointer to Message Router Request.
 *      pa_MRResponse		pointer to Message Router Response.
 *      pa_CPF_data		received CPF Data Item
 *      pa_msg			pointer to memory where reply will be stored
 *  return length of reply
 * 		>0 .. success, 0 .. no reply to send back
 *      	-1 .. error
 */
EIP_STATUS
ForwardOpen(S_CIP_Instance *pa_pstInstance, S_CIP_MR_Request *pa_MRRequest,
    S_CIP_MR_Response *pa_MRResponse, EIP_UINT8 *pa_msg)
{
  EIP_UINT16 nConnectionStatus = CIP_CON_MGR_SUCCESS;
  EIP_UINT32 tmp;

  (void) pa_pstInstance; /*suppress compiler warning */

  /*first check if we have already a connection with the given params */
  g_stDummyConnectionObject.Priority_Timetick = *pa_MRRequest->Data++;
  g_stDummyConnectionObject.Timeoutticks = *pa_MRRequest->Data++;
  /* O_to_T Conn ID */
  g_stDummyConnectionObject.CIPConsumedConnectionID
      = ltohl(&pa_MRRequest->Data);
  /* T_to_O Conn ID */
  g_stDummyConnectionObject.CIPProducedConnectionID
      = ltohl(&pa_MRRequest->Data);
  g_stDummyConnectionObject.ConnectionSerialNumber = ltohs(&pa_MRRequest->Data);
  g_stDummyConnectionObject.OriginatorVendorID = ltohs(&pa_MRRequest->Data);
  g_stDummyConnectionObject.OriginatorSerialNumber = ltohl(&pa_MRRequest->Data);

  if ((NULL != checkForExistingConnection(&g_stDummyConnectionObject)))
    {
      /* TODO this test is  incorrect, see CIP spec 3-5.5.2 re: duplicate forward open
       it should probably be testing the connection type fields
       TODO think on how a reconfigurationrequest could be handled correctly */
      if ((0 == g_stDummyConnectionObject.CIPConsumedConnectionID) && (0
          == g_stDummyConnectionObject.CIPProducedConnectionID))
        {
          /*TODO implement reconfiguration of connection*/

          OPENER_TRACE_ERR("this looks like a duplicate forward open -- I can't handle this yet, sending a CIP_CON_MGR_ERROR_CONNECTION_IN_USE response\n");
        }
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          CIP_ERROR_CONNECTION_FAILURE, CIP_CON_MGR_ERROR_CONNECTION_IN_USE,
          pa_msg);
    }
  /* set state to configuring */
  g_stDummyConnectionObject.State = CONN_STATE_CONFIGURING;
  g_stDummyConnectionObject.SequenceCountProducing = 0; /* set the sequence count to zero */

  g_stDummyConnectionObject.ConnectionTimeoutMultiplier = *pa_MRRequest->Data++;
  pa_MRRequest->Data += 3; /* reserved */
  /* the requested packet interval parameter needs to be a multiple of TIMERTICK from the header file */
  OPENER_TRACE_INFO("ForwardOpen: ConConnID %lu, ProdConnID %lu, ConnSerNo %u\n",
      g_stDummyConnectionObject.CIPConsumedConnectionID,
      g_stDummyConnectionObject.CIPProducedConnectionID,
      g_stDummyConnectionObject.ConnectionSerialNumber);

  g_stDummyConnectionObject.O_to_T_RPI = ltohl(&pa_MRRequest->Data);

  tmp = g_stDummyConnectionObject.T_to_O_RPI % (OPENER_TIMER_TICK * 1000);
  if (tmp > 0)
    {
      g_stDummyConnectionObject.T_to_O_RPI
          = (EIP_UINT32) (g_stDummyConnectionObject.T_to_O_RPI
              / (OPENER_TIMER_TICK * 1000)) * (OPENER_TIMER_TICK * 1000)
              + (OPENER_TIMER_TICK * 1000);
    }

  g_stDummyConnectionObject.O_to_T_NetworkConnectionParameter = ltohs(
      &pa_MRRequest->Data);
  g_stDummyConnectionObject.T_to_O_RPI = ltohl(&pa_MRRequest->Data);

  g_stDummyConnectionObject.T_to_O_NetworkConnectionParameter = ltohs(
      &pa_MRRequest->Data);

  /*check if Network connection paramters are ok */
  if ((CIP_CONN_TYPE_MASK
      == (g_stDummyConnectionObject.O_to_T_NetworkConnectionParameter
          & CIP_CONN_TYPE_MASK)) || (CIP_CONN_TYPE_MASK
      == (g_stDummyConnectionObject.T_to_O_NetworkConnectionParameter
          & CIP_CONN_TYPE_MASK)))
    {
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          CIP_ERROR_CONNECTION_FAILURE,
          CIP_CON_MGR_ERROR_INVALID_CONNECTION_TYPE, pa_msg);
    }

  g_stDummyConnectionObject.TransportTypeTrigger = *pa_MRRequest->Data++;
  /*check if the trigger type value is ok */
  if (0x40 & g_stDummyConnectionObject.TransportTypeTrigger)
    {
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          CIP_ERROR_CONNECTION_FAILURE,
          CIP_CON_MGR_ERROR_TRANSPORT_TRIGGER_NOT_SUPPORTED, pa_msg);
    }

  tmp = parseConnectionPath(&g_stDummyConnectionObject, pa_MRRequest,
      &nConnectionStatus);
  if (EIP_OK != tmp)
    {
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          tmp, nConnectionStatus, pa_msg);
    }

  /*parsing is now finished all data is available and check now establish the connection */
  if (0x03 == (g_stDummyConnectionObject.TransportTypeTrigger & 0x03))
    {
      /*if we are here all values are checked and correct
       buffer the T_to_O connection ID as we have to take the one given from the callee */
      tmp = establishClass3Connection(&g_stDummyConnectionObject,
          &nConnectionStatus);
    }
  else
    {
      tmp
          = establishIOConnction(&g_stDummyConnectionObject, &nConnectionStatus);
    }

  if (EIP_OK != tmp)
    {
      OPENER_TRACE_INFO("connection manager: connect failed\n");
      /* in case of error the dummy objects holds all necessary information */
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          tmp, nConnectionStatus, pa_msg);
    }
  else
    {
      OPENER_TRACE_INFO("connection manager: connect succeeded\n");
      /* in case of success the g_pstActiveConnectionList points to the new connection */
      g_pstActiveConnectionList->State = CONN_STATE_ESTABLISHED;
      return assembleFWDOpenResponse(g_pstActiveConnectionList, pa_MRResponse,
          CIP_ERROR_SUCCESS, 0, pa_msg);
    }
}

/*   EIP_STATUS OpenPointToPointConnection(S_CIP_CPF_Data *pa_CPF_data, S_CIP_CM_Object *pa_pstCMObject, INT8 pa_direction, int pa_index)
 *   open a Point2Point connection dependent on pa_direction.
 *      pa_pstCMObject	pointer to registered Object in ConnectionManager.
 *      pa_index	index of the connection object
 *  return status
 * 		 0 .. success
 *      	-1 .. error
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
      OPENER_TRACE_ERR("cannot create UDP socket in OpenPointToPointConnection\n");
      return EIP_ERROR;
    }

  pa_pstConnObj->m_stOriginatorAddr = addr;   /* store the address of the originator for packet scanning */
  addr.sin_addr.s_addr = INADDR_ANY;          /* restore the address */
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
    S_CIP_CPF_Data *pa_CPF_data, EIP_UINT16 *pa_pnExtendedError)
{
  int newfd;
  in_port_t nPort = OPENER_EIP_IO_UDP_PORT; /* the default port to be used if no port information is part of the forward open request */

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
      OPENER_TRACE_ERR("cannot create UDP socket in OpenPointToPointConnection\n");
      *pa_pnExtendedError = 0x0315; /* miscellaneous*/
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
 *      pa_CPF_data	received CPF Data Item.
 *      pa_pstCMObject	pointer to registered Object in ConnectionManager.
 *      pa_direction	flag to indicate if consuming or producing.
 *      pa_index	index of the connection object
 *  return status
 * 		 0 .. success
 *      	-1 .. error
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
      addr.sin_addr.s_addr = g_nMultiCastAddress;  /* restore the multicast address */
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

/*   void ConnectionObjectGeneralConfiguration(int pa_index)
 *   generate ConnectionID and set configuration parameter in connection object.
 *      pa_index	index of the connection object
 */
void
generalConnectionConfiguration(S_CIP_ConnectionObject *pa_pstConnObj)
{
  if (CIP_POINT_TO_POINT_CONNECTION
      == (pa_pstConnObj->O_to_T_NetworkConnectionParameter
          & CIP_POINT_TO_POINT_CONNECTION))
    {
      /* if we have a point to point connection for the O to T direction
       * the target shall choose the connection ID.
       */
      pa_pstConnObj->CIPConsumedConnectionID = getConnectionId();
    }

  if (CIP_MULTICAST_CONNECTION
      == (pa_pstConnObj->T_to_O_NetworkConnectionParameter
          & CIP_MULTICAST_CONNECTION))
    {
      /* if we have a multi-cast connection for the T to O direction the
       * target shall choose the connection ID.
       */
      pa_pstConnObj->CIPProducedConnectionID = getConnectionId();
    }

  pa_pstConnObj->EIPSequenceCountProducing = 0;
  pa_pstConnObj->SequenceCountProducing = 0;
  pa_pstConnObj->EIPSequenceCountConsuming = 0;
  pa_pstConnObj->SequenceCountConsuming = 0;

  pa_pstConnObj->TransportClassTrigger = pa_pstConnObj->TransportTypeTrigger;

  pa_pstConnObj->WatchdogTimeoutAction = enWatchdogAutoDelete; /* the default for all connections on EIP*/

  pa_pstConnObj->ExpectedPacketRate = 0; /* default value */

  if ((pa_pstConnObj->TransportClassTrigger & 0x80) == 0x00)
    { /* Client Type Connection requested */
      pa_pstConnObj->ExpectedPacketRate
          = (EIP_UINT16) ((pa_pstConnObj->T_to_O_RPI) / 1000);
      /* As soon as we are ready we should produce the connection. With the 0 here we will produce with the next timer tick
       * which should be sufficient. */
      pa_pstConnObj->TransmissionTriggerTimer = 0;
    }
  else
    {
      /* Server Type Connection requested */
      pa_pstConnObj->ExpectedPacketRate
          = (EIP_UINT16) ((pa_pstConnObj->O_to_T_RPI) / 1000);
    }

  /*setup the preconsuption timer: max(ConnectionTimeoutMultiplier * EpectetedPacketRate, 10s) */
  pa_pstConnObj->InnacitvityWatchdogTimer
      = ((((pa_pstConnObj->O_to_T_RPI) / 1000) << (2
          + pa_pstConnObj->ConnectionTimeoutMultiplier)) > 10000) ? (((pa_pstConnObj->O_to_T_RPI)
          / 1000) << (2 + pa_pstConnObj->ConnectionTimeoutMultiplier))
          : 10000;

  pa_pstConnObj->ConsumedConnectionSize
      = pa_pstConnObj->O_to_T_NetworkConnectionParameter & 0x01FF;

  pa_pstConnObj->ProducedConnectionSize
      = pa_pstConnObj->T_to_O_NetworkConnectionParameter & 0x01FF;

}

EIP_STATUS
ForwardClose(S_CIP_Instance *pa_pstInstance, S_CIP_MR_Request * pa_MRRequest,
    S_CIP_MR_Response * pa_MRResponse, EIP_UINT8 * pa_msg)
{
  /* check connection serial number && Vendor ID && OriginatorSerialNr if connection is established */
  EIP_UINT16 nConnectionStatus =
      CIP_CON_MGR_ERROR_CONNECTION_NOT_FOUND_AT_TARGET_APPLICATION;
  EIP_UINT16 ConnectionSerialNr, OriginatorVendorID;
  EIP_UINT32 OriginatorSerialNr;
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  /*Suppress compiler warning*/
  (void) pa_pstInstance;

  /* set AddressInfo Items to invalid TypeID to prevent assembleLinearMsg to read them */
  g_stCPFDataItem.AddrInfo[0].TypeID = 0;
  g_stCPFDataItem.AddrInfo[1].TypeID = 0;

  pa_MRRequest->Data += 2; /* ignore Priority/Time_tick and Time-out_ticks */
  ConnectionSerialNr = ltohs(&pa_MRRequest->Data);
  OriginatorVendorID = ltohs(&pa_MRRequest->Data);
  OriginatorSerialNr = ltohl(&pa_MRRequest->Data);

  OPENER_TRACE_INFO("ForwardClose: ConnSerNo %d\n", ConnectionSerialNr);

  while (NULL != pstRunner)
    {
      /* this check should not be necessary as only established connections should be in the active connection list */
      if ((pstRunner->State == CONN_STATE_ESTABLISHED) || (pstRunner->State
          == CONN_STATE_TIMEDOUT))
        {
          if ((pstRunner->ConnectionSerialNumber == ConnectionSerialNr)
              && (pstRunner->OriginatorVendorID == OriginatorVendorID)
              && (pstRunner->OriginatorSerialNumber == OriginatorSerialNr))
            { /* found the corresponding connection object -> close it */
              if (enConnTypeExplicit != pstRunner->m_eInstanceType)
                {
                  closeIOConnection(pstRunner);
                }
              else
                {
                  closeConnection(pstRunner);
                }
              nConnectionStatus = CIP_CON_MGR_SUCCESS;
              break;
            }
        }
      pstRunner = pstRunner->m_pstNext;
    }

  return assembleFWDCloseResponse(ConnectionSerialNr, OriginatorVendorID,
      OriginatorSerialNr, pa_MRRequest, pa_MRResponse, nConnectionStatus,
      pa_msg);
}

EIP_STATUS
UnconnectedSend(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse,
    EIP_UINT8 * pa_msg)
{
  EIP_UINT8 *p;
  S_CIP_UnconnectedSend_Param_Struct data;
  S_Data_Item stDataItem;

  /*Suppress compiler warning*/
  (void) pa_pstInstance;
  (void) pa_MRResponse;
  (void) pa_msg;

  p = pa_MRRequest->Data;
  data.Priority = *p++;
  data.Timeout_Ticks = *p++;
  data.Message_Request_Size = ltohs(&p);

  if (data.Message_Request_Size != 0)
    {
      stDataItem.TypeID = g_stCPFDataItem.stDataI_Item.TypeID;
      stDataItem.Length = data.Message_Request_Size;
      stDataItem.Data = p;
    }

  p = pa_MRRequest->Data + 4 + data.Message_Request_Size;
  /* check for padding */
  if (data.Message_Request_Size % 2 != 0)
    p++;

  data.Route_Path.PathSize = *p++;
  data.Reserved = *p++;
  if (data.Route_Path.PathSize == 1)
    {
      data.Route_Path.Port = *p++;
      data.Route_Path.Address = *p;
    }
  else
    {
      /*TODO: other packet received */
      OPENER_TRACE_WARN("Warning: Route path data of unconnected send currently not handled\n");
    }
  /*TODO correctly handle the path, currently we just ignore it and forward to the message router which should be ok for non routing devices*/
  return notifyMR(stDataItem.Data, data.Message_Request_Size);
}

EIP_STATUS
GetConnectionOwner(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse,
    EIP_UINT8 * pa_msg)
{
  /* suppress compiler warnings */
  (void) pa_pstInstance;
  (void) pa_MRRequest;
  (void) pa_MRResponse;
  (void) pa_msg;

  return EIP_OK;
}

EIP_STATUS
manageConnections(void)
{
  EIP_STATUS res;

  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if (pstRunner->State == CONN_STATE_ESTABLISHED)
        {
          if ((0 != pstRunner->p_stConsumingInstance) || /* we have a consuming connection check innacitivitywatchdog timer */
          (pstRunner->TransportClassTrigger & 0x80)) /* all sever connections have to maintain an innacitivitywatchdog timer */
            {
              pstRunner->InnacitvityWatchdogTimer -= OPENER_TIMER_TICK;
              if (pstRunner->InnacitvityWatchdogTimer <= 0)
                {
                  /* we have a timed out connection perform watchdog time out action*/
                  OPENER_TRACE_INFO(">>>>>>>>>>Connection timed out\n");
                  if (enConnTypeExplicit == pstRunner->m_eInstanceType)
                    { /* explicit connection have to be closed */
                      closeConnection(pstRunner);
                    }
                  else
                    {
                      handleIOConnectionTimeOut(pstRunner);
                    }
                }
            }
          /* only if the connection has not timed out check if data is to be send */
          if (CONN_STATE_ESTABLISHED == pstRunner->State)
            {
              /* client connection */
              if ((0 == (pstRunner->TransportClassTrigger & 0x70)) && /* cyclic connection */
              (pstRunner->ExpectedPacketRate != 0) && (EIP_INVALID_SOCKET
                  != pstRunner->sockfd[PRODUCING])) /* only produce for the master connection */
                {
                  pstRunner->TransmissionTriggerTimer -= OPENER_TIMER_TICK;
                  if (pstRunner->TransmissionTriggerTimer <= 0)
                    { /* need to send package */
                      /* send() */
                      res = sendConnectedData(pstRunner);
                      if (res == EIP_ERROR)
                        {
                          OPENER_TRACE_ERR(
                              "sending of UDP data in manage Connection failed\n");
                        }
                      /* reload the timer value */
                      pstRunner->TransmissionTriggerTimer
                          = pstRunner->ExpectedPacketRate;
                    }
                }
            }
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return EIP_OK;
}

S_CIP_ConnectionObject *
getFreeExplicitConnection(void)
{
  int i;
  for (i = 0; i < OPENER_CIP_NUM_EXPLICIT_CONNS; i++)
    {
      if (g_astExplicitConnections[i].State == CONN_STATE_NONEXISTENT)
        return &(g_astExplicitConnections[i]);
    }
  return NULL;
}

/*   INT8 assembleFWDOpenResponse(S_CIP_ConnectionObject *pa_pstConnObj, S_CIP_MR_Response * pa_MRResponse, EIP_UINT8 pa_nGeneralStatus, EIP_UINT16 pa_nExtendedStatus,
 void * deleteMeSomeday, EIP_UINT8 * pa_msg)
 *   create FWDOpen response dependent on status.
 *      pa_pstConnObj pointer to connection Object
 *      pa_MRResponse	pointer to message router response
 *      pa_nGeneralStatus the general status of the response
 *      pa_nExtendedStatus extended status in the case of an error otherwise 0
 *      pa_CPF_data	pointer to CPF Data Item
 *      pa_msg		pointer to memory where reply has to be stored
 *  return status
 * 			0 .. no reply need to be sent back
 * 			1 .. need to send reply
 * 		  -1 .. error
 */
EIP_STATUS
assembleFWDOpenResponse(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_MR_Response * pa_MRResponse, EIP_UINT8 pa_nGeneralStatus,
    EIP_UINT16 pa_nExtendedStatus, EIP_UINT8 * pa_msg)
{
  /* write reply information in CPF struct dependent of pa_status */
  S_CIP_CPF_Data *pa_CPF_data = &g_stCPFDataItem;
  pa_CPF_data->ItemCount = 2;
  pa_MRResponse->Data = pa_msg;
  pa_CPF_data->stDataI_Item.TypeID = CIP_ITEM_ID_UNCONNECTEDMESSAGE;
  pa_CPF_data->stAddr_Item.TypeID = CIP_ITEM_ID_NULL;
  pa_CPF_data->stAddr_Item.Length = 0;

  pa_MRResponse->ReplyService = (0x80 | CIP_FORWARD_OPEN);
  pa_MRResponse->GeneralStatus = pa_nGeneralStatus;

  if (CIP_ERROR_SUCCESS == pa_nGeneralStatus)
    {
      OPENER_TRACE_INFO("assembleFWDOpenResponse: sending success response\n");
      pa_MRResponse->DataLength = 26; /* if there is no application specific data */
      pa_MRResponse->SizeofAdditionalStatus = 0;

      if (pa_CPF_data->AddrInfo[0].TypeID != 0)
        {
          pa_CPF_data->ItemCount = 3;
          if (pa_CPF_data->AddrInfo[1].TypeID != 0)
            {
              pa_CPF_data->ItemCount = 4; /* there are two sockaddrinfo items to add */
            }
        }

      htoll(pa_pstConnObj->CIPConsumedConnectionID, &pa_msg);
      htoll(pa_pstConnObj->CIPProducedConnectionID, &pa_msg);
    }
  else
    {
      /* we have an connection creation error */
      OPENER_TRACE_INFO("assembleFWDOpenResponse: sending error response\n");
      pa_pstConnObj->State = CONN_STATE_NONEXISTENT;
      pa_MRResponse->DataLength = 10;
      pa_MRResponse->SizeofAdditionalStatus = 1;
      pa_MRResponse->AdditionalStatus[0] = pa_nExtendedStatus;
    }

  htols(pa_pstConnObj->ConnectionSerialNumber, &pa_msg);
  htols(pa_pstConnObj->OriginatorVendorID, &pa_msg);
  htoll(pa_pstConnObj->OriginatorSerialNumber, &pa_msg);

  if (CIP_ERROR_SUCCESS == pa_nGeneralStatus)
    {
      /* set the actual packet rate to requested packet rate */
      htoll(pa_pstConnObj->O_to_T_RPI, &pa_msg);
      htoll(pa_pstConnObj->T_to_O_RPI, &pa_msg);
    }

  *pa_msg = 0; /* remaining path size -  for routing devices relevant */
  pa_msg++;
  *pa_msg = 0; /* reserved */
  pa_msg++;

  return EIP_OK_SEND; /* send reply */
}

/*   INT8 assembleFWDCloseResponse(UINT16 pa_ConnectionSerialNr, UINT16 pa_OriginatorVendorID, UINT32 pa_OriginatorSerialNr, S_CIP_MR_Request *pa_MRRequest, S_CIP_MR_Response *pa_MRResponse, S_CIP_CPF_Data *pa_CPF_data, INT8 pa_status, INT8 *pa_msg)
 *   create FWDClose response dependent on status.
 *      pa_ConnectionSerialNr	requested ConnectionSerialNr
 *      pa_OriginatorVendorID	requested OriginatorVendorID
 *      pa_OriginatorSerialNr	requested OriginalSerialNr
 *      pa_MRRequest		pointer to message router request
 *      pa_MRResponse		pointer to message router response
 *      pa_CPF_data		pointer to CPF Data Item
 *      pa_status		status of FWDClose
 *      pa_msg			pointer to memory where reply has to be stored
 *  return status
 * 			0 .. no reply need to ne sent back
 * 			1 .. need to send reply
 * 		       -1 .. error
 */
EIP_STATUS
assembleFWDCloseResponse(EIP_UINT16 pa_ConnectionSerialNr,
    EIP_UINT16 pa_OriginatorVendorID, EIP_UINT32 pa_OriginatorSerialNr,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse,
    EIP_UINT16 pa_nExtErrorCode, EIP_UINT8 * pa_msg)
{
  /* write reply information in CPF struct dependent of pa_status */
  S_CIP_CPF_Data *pa_CPF_data = &g_stCPFDataItem;
  pa_CPF_data->ItemCount = 2;
  pa_MRResponse->Data = pa_msg;
  pa_CPF_data->stDataI_Item.TypeID = CIP_ITEM_ID_UNCONNECTEDMESSAGE;
  pa_CPF_data->stAddr_Item.TypeID = CIP_ITEM_ID_NULL;
  pa_CPF_data->stAddr_Item.Length = 0;

  htols(pa_ConnectionSerialNr, &pa_msg);
  htols(pa_OriginatorVendorID, &pa_msg);
  htoll(pa_OriginatorSerialNr, &pa_msg);

  pa_MRResponse->ReplyService = (0x80 | pa_MRRequest->Service);
  pa_MRResponse->DataLength = 10; /* if there is no application specific data */

  if (CIP_CON_MGR_SUCCESS == pa_nExtErrorCode)
    {
      *pa_msg = 0; /* no application data */
      pa_MRResponse->GeneralStatus = CIP_ERROR_SUCCESS;
      pa_MRResponse->SizeofAdditionalStatus = 0;
    }
  else
    {
      *pa_msg = *pa_MRRequest->Data; /* remaining path size */
      pa_MRResponse->GeneralStatus = CIP_ERROR_CONNECTION_FAILURE;
      pa_MRResponse->AdditionalStatus[0] = pa_nExtErrorCode;
      pa_MRResponse->SizeofAdditionalStatus = 1;
    }

  pa_msg++;
  *pa_msg = 0; /* reserved */
  pa_msg++;

  return EIP_OK_SEND;
}

S_CIP_ConnectionObject *
getConnectedObject(EIP_UINT32 ConnectionID)
{
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if (pstRunner->State == CONN_STATE_ESTABLISHED)
        {
          if (pstRunner->CIPConsumedConnectionID == ConnectionID)
            return pstRunner;
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return NULL;
}

S_CIP_ConnectionObject *
checkForExistingConnection(S_CIP_ConnectionObject *pa_pstConnObj)
{
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if (pstRunner->State == CONN_STATE_ESTABLISHED)
        {
          if ((pa_pstConnObj->ConnectionSerialNumber
              == pstRunner->ConnectionSerialNumber)
              && (pa_pstConnObj->OriginatorVendorID
                  == pstRunner->OriginatorVendorID)
              && (pa_pstConnObj->OriginatorSerialNumber
                  == pstRunner->OriginatorSerialNumber))
            {
              return pstRunner;
            }
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return NULL;
}

EIP_STATUS
checkElectronicKeyData(EIP_UINT8 pa_nKeyFormat, S_CIP_KeyData *pa_pstKeyData,
    EIP_UINT16 *pa_pnExtStatus)
{
  EIP_BYTE nCompatiblityMode = pa_pstKeyData->MajorRevision & 0x80;

  pa_pstKeyData->MajorRevision &= 0x7F;
  *pa_pnExtStatus = CIP_CON_MGR_SUCCESS;
  if (4 != pa_nKeyFormat)
    {
      *pa_pnExtStatus = CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
      return EIP_ERROR;
    }

  if (((pa_pstKeyData->VendorID != VendorID) && (pa_pstKeyData->VendorID != 0))
      || ((pa_pstKeyData->ProductCode != ProductCode)
          && (pa_pstKeyData->ProductCode != 0)))
    {
      *pa_pnExtStatus = CIP_CON_MGR_ERROR_VENDERID_OR_PRODUCTCODE_ERROR;
    }
  else
    {
      if ((pa_pstKeyData->DeviceType != DeviceType)
          && (pa_pstKeyData->DeviceType != 0))
        {
          *pa_pnExtStatus = CIP_CON_MGR_ERROR_VENDERID_OR_PRODUCT_TYPE_ERROR;
        }
      else
        {
          if (pa_pstKeyData->MajorRevision != 0)
            { /* 0 means accept any revision combination */
              if (pa_pstKeyData->MinorRevision == 0)
                {
                  pa_pstKeyData->MinorRevision = Revison.MinorRevision;
                }

              if (!((pa_pstKeyData->MajorRevision == Revison.MajorRevision)
                  && (pa_pstKeyData->MinorRevision == Revison.MinorRevision)))
                {
                  /* we have no exact match */
                  if (!nCompatiblityMode)
                    {
                      *pa_pnExtStatus = CIP_CON_MGR_ERROR_REVISION_MISMATCH;
                    }
                  else
                    {
                      if ((pa_pstKeyData->MajorRevision
                          != Revison.MajorRevision)
                          || ((pa_pstKeyData->MajorRevision
                              == Revison.MajorRevision)
                              && (pa_pstKeyData->MinorRevision
                                  > Revison.MinorRevision)))
                        {
                          /*TODO check if we accept also greater minor revision depends on the product. Maybe should be configurable */
                          *pa_pnExtStatus = CIP_CON_MGR_ERROR_REVISION_MISMATCH;
                        }
                    }
                }

            }
        }
    }
  return (*pa_pnExtStatus == CIP_CON_MGR_SUCCESS) ? EIP_OK : EIP_ERROR;
}

EIP_UINT8
parseConnectionPath(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_MR_Request *pa_MRRequest, EIP_UINT16 *pa_pnExtendedError)
{
  int i;
  EIP_UINT8 *pnMsg = pa_MRRequest->Data;
  int nRemainingPathSize = pa_pstConnObj->ConnectionPathSize = *pnMsg++; /* length in words */
  S_CIP_Class *pstClass = NULL;
  int O2TConnectionType, T2OConnectionType, imax;
      

  if ((FORWARD_OPEN_HEADER_LENGTH + nRemainingPathSize * 2)
      < pa_MRRequest->DataLength)
    {
      /* the received packet is larger than the data in the path */
      *pa_pnExtendedError = 0;
      return CIP_ERROR_TOO_MUCH_DATA;
    }

  if ((FORWARD_OPEN_HEADER_LENGTH + nRemainingPathSize * 2)
      > pa_MRRequest->DataLength)
    {
      /*there is not enough data in received packet */
      *pa_pnExtendedError = 0;
      return CIP_ERROR_NOT_ENOUGH_DATA;
    }

  if (nRemainingPathSize > 0)
    {
      /* first electronic key */
      if (*pnMsg == 0x34)
        {
          if (nRemainingPathSize < 5)
            {
              /*there is not enough data for holding the electronic key segement*/
              *pa_pnExtendedError = 0;
              return CIP_ERROR_NOT_ENOUGH_DATA;
            }

          /* logical electronic key found */
          pa_pstConnObj->ElectronicKey.SegmentType = 0x34;
          pnMsg++;
          pa_pstConnObj->ElectronicKey.KeyFormat = *pnMsg++;
          pa_pstConnObj->ElectronicKey.KeyData.VendorID = ltohs(&pnMsg);
          pa_pstConnObj->ElectronicKey.KeyData.DeviceType = ltohs(&pnMsg);
          pa_pstConnObj->ElectronicKey.KeyData.ProductCode = ltohs(&pnMsg);
          pa_pstConnObj->ElectronicKey.KeyData.MajorRevision = *pnMsg++;
          pa_pstConnObj->ElectronicKey.KeyData.MinorRevision = *pnMsg++;
          nRemainingPathSize -= 5; /*length of the electronic key*/
          OPENER_TRACE_INFO(
              "key: ven ID %d, dev type %d, prod code %d, major %d, minor %d\n",
              pa_pstConnObj->ElectronicKey.KeyData.VendorID,
              pa_pstConnObj->ElectronicKey.KeyData.DeviceType,
              pa_pstConnObj->ElectronicKey.KeyData.ProductCode,
              pa_pstConnObj->ElectronicKey.KeyData.MajorRevision,
              pa_pstConnObj->ElectronicKey.KeyData.MinorRevision);

          if (EIP_OK != checkElectronicKeyData(
              pa_pstConnObj->ElectronicKey.KeyFormat,
              &(pa_pstConnObj->ElectronicKey.KeyData), pa_pnExtendedError))
            {
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }
      else
        {
          OPENER_TRACE_INFO("no key\n");
        }

      if (EQLOGICALPATH(*pnMsg,0x20))
        { /* classID */
          pa_pstConnObj->ConnectionPath.ClassID = GETPADDEDLOGICALPATH(&pnMsg);
          pstClass = getCIPClass(pa_pstConnObj->ConnectionPath.ClassID);
          if (0 == pstClass)
            {
              OPENER_TRACE_ERR("classid %lx not found\n",
                  pa_pstConnObj->ConnectionPath.ClassID);
              if (pa_pstConnObj->ConnectionPath.ClassID >= 0xC8) /*reserved range of class ids */

                {
                  *pa_pnExtendedError
                      = CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
                }
              else
                {
                  *pa_pnExtendedError
                      = CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
                }
              return CIP_ERROR_CONNECTION_FAILURE;
            }OPENER_TRACE_INFO("classid %lx (%s)\n",
              pa_pstConnObj->ConnectionPath.ClassID, pstClass->acName);
        }
      else
        {
          *pa_pnExtendedError = CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
          return CIP_ERROR_CONNECTION_FAILURE;
        }
      nRemainingPathSize -= 1; /* 1 16Bit word for the class part of the path */

      if (EQLOGICALPATH(*pnMsg,0x24))
        { /* store the configuration ID for later checking in the application connection types */
          pa_pstConnObj->ConnectionPath.ConnectionPoint[2]
              = GETPADDEDLOGICALPATH(&pnMsg);
          OPENER_TRACE_INFO("Configuration instance id %ld\n", pa_pstConnObj->ConnectionPath.ConnectionPoint[2]);
          if (NULL == getCIPInstance(pstClass,
              pa_pstConnObj->ConnectionPath.ConnectionPoint[2]))
            {
              /*according to the test tool we should respond with this extended error code */
              *pa_pnExtendedError
                  = CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }
      else
        {
          *pa_pnExtendedError = CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
          return CIP_ERROR_CONNECTION_FAILURE;
        }

      /* 1 or 2 16Bit words for the configuration instance part of the path  */
      nRemainingPathSize -= (pa_pstConnObj->ConnectionPath.ConnectionPoint[2] > 0xFF ) ? 2 : 1;

      if (0x03 == (pa_pstConnObj->TransportTypeTrigger & 0x03))
        { /* we have Class 3 connection, connection end point has to be the message router instance 1 */
          if ((pa_pstConnObj->ConnectionPath.ClassID
              != CIP_MESSAGE_ROUTER_CLASS_CODE)
              || (pa_pstConnObj->ConnectionPath.ConnectionPoint[2] != 1))
            {
              *pa_pnExtendedError = CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
              return CIP_ERROR_CONNECTION_FAILURE;
            }
          pa_pstConnObj->ConnectionPath.ConnectionPoint[0]
              = pa_pstConnObj->ConnectionPath.ConnectionPoint[2];
        }
      else
        { /* we have an IO connection */
          if (pa_pstConnObj->ConnectionPath.ClassID != CIP_ASSEMBLY_CLASS_CODE)
            { /* we currently only support IO connections to assembly instances */
              *pa_pnExtendedError = CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
              return CIP_ERROR_CONNECTION_FAILURE;
            }

          O2TConnectionType = (pa_pstConnObj->O_to_T_NetworkConnectionParameter
              & 0x6000) >> 13;
          T2OConnectionType = (pa_pstConnObj->T_to_O_NetworkConnectionParameter
              & 0x6000) >> 13;

          pa_pstConnObj->ConnectionPath.ConnectionPoint[1] = 0; /* set not available path to Invalid */

          if (O2TConnectionType == 0)
            {
              if (T2OConnectionType == 0)
                { /* configuration only connection */
                  imax = 0;
                  OPENER_TRACE_WARN("assembly: type invalid\n");
                }
              else
                { /* 1 path -> path is for production */
                  OPENER_TRACE_INFO("assembly: type produce\n");
                  imax = 1;
                }
            }
          else
            {
              if (T2OConnectionType == 0)
                { /* 1 path -> path is for consumption */
                  OPENER_TRACE_INFO("assembly: type consume\n");
                  imax = 1;
                }
              else
                { /* 2 pathes -> 1st for production 2nd for consumption */
                  OPENER_TRACE_INFO("assembly: type bidirectional\n");
                  imax = 2;
                }
            }

          for (i = 0; i < imax; i++) /* process up to 2 encoded paths */
            {
              if (EQLOGICALPATH(*pnMsg,0x24) || EQLOGICALPATH(*pnMsg,0x2C)) /* Connection Point interpreted as InstanceNr -> only in Assembly Objects */
                { /* InstanceNR */
                  pa_pstConnObj->ConnectionPath.ConnectionPoint[i]
                      = GETPADDEDLOGICALPATH(&pnMsg);
                  OPENER_TRACE_INFO("connection point %lu\n",
                      pa_pstConnObj->ConnectionPath.ConnectionPoint[i]);
                  if (0 == getCIPInstance(pstClass,
                      pa_pstConnObj->ConnectionPath.ConnectionPoint[i]))
                    {
                      *pa_pnExtendedError
                          = CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
                      return CIP_ERROR_CONNECTION_FAILURE;
                    }
                  /* 1 or 2 16Bit word for the connection point part of the path */
                  nRemainingPathSize -= (pa_pstConnObj->ConnectionPath.ConnectionPoint[i] > 0xFF ) ? 2 : 1;
                }
              else
                {
                  *pa_pnExtendedError
                      = CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
                  return CIP_ERROR_CONNECTION_FAILURE;
                }
            }

          g_unConfigDataLen = 0;
          g_pnConfigDataBuffer = NULL;

          if (nRemainingPathSize > 0)
            { /* have something left in the path should be configuration data */
              if (0x80 == *pnMsg)
                { /* we have a simple data segment
                 TODO do we have to handle ANSI extended symbol data segments too? */
                  g_unConfigDataLen = pnMsg[1] * 2; /*data segments store length 16-bit word wise */
                  g_pnConfigDataBuffer = &(pnMsg[2]);
                }
              else
                {
                  OPENER_TRACE_WARN(
                      "No data segment identifier found for the configuration data\n");
                  *pa_pnExtendedError = pa_pstConnObj->ConnectionPathSize
                      - nRemainingPathSize; /*offset in 16Bit words where within the connection path the error happend*/
                  return 0x04; /*status code for invalid segment type*/
                }
            }
        }
    }

  /*save back the current position in the stream allowing followers to parse anything thats still there*/
  pa_MRRequest->Data = pnMsg;
  return EIP_OK;
}

int
establishClass3Connection(S_CIP_ConnectionObject *pa_pstConnObj,
    EIP_UINT16 *pa_pnExtendedError)
{
  int nRetVal = EIP_OK;
  EIP_UINT32 nTmp;

  S_CIP_ConnectionObject *pstExplConn = getFreeExplicitConnection();

  if (NULL == pstExplConn)
    {
      nRetVal = CIP_ERROR_CONNECTION_FAILURE;
      *pa_pnExtendedError = CIP_CON_MGR_ERROR_NO_MORE_CONNECTIONS_AVAILABLE;
    }
  else
    {
      copyConnectionData(pstExplConn, pa_pstConnObj);
      nTmp = pstExplConn->CIPProducedConnectionID;
      generalConnectionConfiguration(pstExplConn);
      pstExplConn->CIPProducedConnectionID = nTmp;
      pstExplConn->m_eInstanceType = enConnTypeExplicit;
      pstExplConn->sockfd[0] = pstExplConn->sockfd[1] = EIP_INVALID_SOCKET;
      addNewActiveConnection(pstExplConn);
    }
  return nRetVal;
}

int
establishIOConnction(S_CIP_ConnectionObject *pa_pstConnObjData,
    EIP_UINT16 *pa_pnExtendedError)
{
  int O2TConnectionType, T2OConnectionType;
  S_CIP_attribute_struct *pstAttribute;
  /* currently we allow I/O connections only to assembly objects */
  S_CIP_Class *pstAssemblyClass = getCIPClass(CIP_ASSEMBLY_CLASS_CODE); /* we don't need to check for zero as this is handled in the connection path parsing */
  S_CIP_Instance *pstInstance = NULL;

  S_CIP_ConnectionObject *pstIOConnObj = getIOConnectionForConnectionData(
      pa_pstConnObjData, pa_pnExtendedError);
  /*get pointer to the cpf data, currently we have just one global instance of the struct. This may change in the future*/
  S_CIP_CPF_Data *pstCPF_data = &g_stCPFDataItem;

  if (NULL == pstIOConnObj)
    {
      return CIP_ERROR_CONNECTION_FAILURE;
    }

  O2TConnectionType
      = (pstIOConnObj->O_to_T_NetworkConnectionParameter & 0x6000) >> 13;
  T2OConnectionType
      = (pstIOConnObj->T_to_O_NetworkConnectionParameter & 0x6000) >> 13;

  generalConnectionConfiguration(pstIOConnObj);


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

              if ((pstIOConnObj->TransportTypeTrigger & 0x0F) == 1)
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
              if ((pstIOConnObj->TransportTypeTrigger & 0x0F) == 1)
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

      /* open a connection "point to point" or "multicast" based on the ConnectionParameter */
      if (O2TConnectionType == 1) /* Multicast consuming */
        {
          if (OpenMulticastConnection(CONSUMING, pstIOConnObj, pstCPF_data)
              == EIP_ERROR)
            {
              OPENER_TRACE_ERR("error in OpenMulticast Connection\n");
              *pa_pnExtendedError = 0; /*TODO find out the correct extended error code*/
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }
      else if (O2TConnectionType == 2) /* Point to Point consuming */
        {
          if (OpenConsumingPointToPointConnection(pstIOConnObj, pstCPF_data)
              == EIP_ERROR)
            {
              OPENER_TRACE_ERR("error in PointToPoint consuming connection\n");
              *pa_pnExtendedError = 0; /*TODO find out the correct extended error code*/
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }

      if (T2OConnectionType == 1) /* Multicast producing */
        {
          if (openProducingMulticastConnection(pstIOConnObj, pstCPF_data)
              == EIP_ERROR)
            {
              OPENER_TRACE_ERR("error in OpenMulticast Connection\n");
              *pa_pnExtendedError = 0; /*TODO find out the correct extended error code*/
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }
      else if (T2OConnectionType == 2) /* Point to Point producing */
        {

          if (OpenProducingPointToPointConnection(pstIOConnObj, pstCPF_data,
              pa_pnExtendedError) != EIP_OK)
            {
              OPENER_TRACE_ERR("error in PointToPoint producing connection\n");
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }

    }
  addNewActiveConnection(pstIOConnObj);
  IApp_IOConnectionEvent(pstIOConnObj->ConnectionPath.ConnectionPoint[0],
      pstIOConnObj->ConnectionPath.ConnectionPoint[1], enOpened);
  return EIP_OK;
}

void
closeConnection(S_CIP_ConnectionObject *pa_pstConnObj)
{
  pa_pstConnObj->State = CONN_STATE_NONEXISTENT;
  if (0x03 != (pa_pstConnObj->TransportTypeTrigger & 0x03))
    {
      /* only close the udp connection for not class 3 connections */
      IApp_CloseSocket(pa_pstConnObj->sockfd[CONSUMING]);
      pa_pstConnObj->sockfd[CONSUMING] = EIP_INVALID_SOCKET;
      IApp_CloseSocket(pa_pstConnObj->sockfd[PRODUCING]);
      pa_pstConnObj->sockfd[CONSUMING] = EIP_INVALID_SOCKET;
    }
  removeFromeActiveConnections(pa_pstConnObj);
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
  if ((pa_pstConnection->TransportClassTrigger & 0x0F) != 0)
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
  if ((pa_pstConnection->TransportTypeTrigger & 0x0F) == 1)
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

void
copyConnectionData(S_CIP_ConnectionObject *pa_pstDst,
    S_CIP_ConnectionObject *pa_pstSrc)
{
  memcpy(pa_pstDst, pa_pstSrc, sizeof(S_CIP_ConnectionObject));
}

void
addNewActiveConnection(S_CIP_ConnectionObject *pa_pstConn)
{
  pa_pstConn->m_pstFirst = NULL;
  pa_pstConn->m_pstNext = g_pstActiveConnectionList;
  if (NULL != g_pstActiveConnectionList)
    {
      g_pstActiveConnectionList->m_pstFirst = pa_pstConn;
    }
  g_pstActiveConnectionList = pa_pstConn;
}

void
removeFromeActiveConnections(S_CIP_ConnectionObject *pa_pstConn)
{
  if (NULL != pa_pstConn->m_pstFirst)
    {
      pa_pstConn->m_pstFirst->m_pstNext = pa_pstConn->m_pstNext;
    }
  else
    {
      g_pstActiveConnectionList = pa_pstConn->m_pstNext;
    }
  if (NULL != pa_pstConn->m_pstNext)
    {
      pa_pstConn->m_pstNext->m_pstFirst = pa_pstConn->m_pstFirst;
    }
  pa_pstConn->m_pstFirst = NULL;
  pa_pstConn->m_pstNext = NULL;
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
  closeConnection(pa_pstConnObjData);
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

  closeConnection(pa_pstConn);
}

EIP_UINT16
handleConfigData(S_CIP_Class *pa_pstAssemblyClass,
    S_CIP_ConnectionObject *pa_pstIOConnObj)
{
  EIP_UINT16 unRetVal = 0;
  S_CIP_Instance *pstConfigInstance = getCIPInstance(pa_pstAssemblyClass,
      pa_pstIOConnObj->ConnectionPath.ConnectionPoint[2]);

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
  return unRetVal;
}
