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
#include "encap.h"
#include "cipidentity.h"
#include "trace.h"
#include "cipclass3connection.h"
#include "cipioconnection.h"
#include "cipassembly.h"
#include "cpf.h"
#include "appcontype.h"
#include "encap.h"

/* values needed from cipidentiy */
extern EIP_UINT16 VendorID;
extern EIP_UINT16 DeviceType;
extern EIP_UINT16 ProductCode;
extern S_CIP_Revision Revison;

#define CIP_CONN_PATH_INVALID 1
#define CIP_CONN_PATH_CONFIGURATION 2
#define CIP_CONN_PATH_CONSUMPTION 3
#define CIP_CONN_PATH_CONFIGURATION_AND_CONSUMPTION 4
#define CIP_CONN_PATH_PRODUCTION 5
#define CIP_CONN_PATH_CONFIGURATION_AND_PRODUCTION 6
#define CIP_CONN_PATH_CONSUMTION_AND_PRODUCTION 7
#define CIP_CONN_PATH_CONFIGURATION_AND_CONSUMPTION_AND_PRODUCTION 8

#define CIP_CONN_TYPE_MASK 0x6000   /*Bit 13&14 true*/

#define CIP_CONN_PIT_NETWORK_SEGEMENT_ID  0x43  /* identifier indicating a production inhibit time network segment */

#define FORWARD_OPEN_HEADER_LENGTH 36         /* the length in bytes of the forward open command specific data till the start of the connection path (including con path size)*/
#define EQLOGICALPATH(x,y) (((x)&0xfc)==(y))

static const int scg_nNumConnectableObjects = 2
    + OPENER_CIP_NUM_APPLICATION_SPECIFIC_CONNECTABLE_OBJECTS;

typedef struct
{
  EIP_UINT32 m_nClassID;
  TConnOpenFunc m_pfOpenFunc;
} TConnMgmHandling;

/* global variables private */
/*!List holding information on the object classes and open/close function
 * pointers to which connections may be established.
 */
TConnMgmHandling g_astConnMgmList[2
    + OPENER_CIP_NUM_APPLICATION_SPECIFIC_CONNECTABLE_OBJECTS];

/*! List holding all currently active connections*/
/*@null@*/S_CIP_ConnectionObject *g_pstActiveConnectionList = NULL; 

/*! buffer connection object needed for forward open */
S_CIP_ConnectionObject g_stDummyConnectionObject;

/*! value holds the connection ID's "incarnation ID" in the upper 16 bits     */

EIP_UINT32 g_nIncarnationID;

/* private functions */
EIP_STATUS
ForwardOpen(S_CIP_Instance * pa_pstInstance, S_CIP_MR_Request * pa_MRRequest,
    S_CIP_MR_Response * pa_MRResponse);
EIP_STATUS
ForwardClose(S_CIP_Instance * pa_pstInstance, S_CIP_MR_Request * pa_MRRequest,
    S_CIP_MR_Response * pa_MRResponse);

EIP_STATUS
GetConnectionOwner(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse);

EIP_STATUS
assembleFWDOpenResponse(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_MR_Response * pa_MRResponse, EIP_UINT8 pa_nGeneralStatus,
    EIP_UINT16 pa_nExtendedStatus);

EIP_STATUS
assembleFWDCloseResponse(EIP_UINT16 pa_ConnectionSerialNr,
    EIP_UINT16 pa_OriginatorVendorID, EIP_UINT32 pa_OriginatorSerialNr,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse,
    EIP_UINT16 pa_nExtErrorCode);

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
 */EIP_UINT8
parseConnectionPath(S_CIP_ConnectionObject *pa_pstConnObj,
    S_CIP_MR_Request *pa_MRRequest, EIP_UINT16 *pa_pnExtendedError);

TConnMgmHandling *
getConnMgmEntry(EIP_UINT32 pa_nClassId);

void
initializeConnectionManagerData();

unsigned int
GETPADDEDLOGICALPATH(unsigned char **x)
{
  unsigned int tmp;

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
 */EIP_UINT32
getConnectionId()
{
  static EIP_UINT32 nConnectionId = 18;
  nConnectionId++;
  return (g_nIncarnationID | (nConnectionId & 0x0000FFFF));
}

EIP_STATUS
Connection_Manager_Init(EIP_UINT16 pa_nUniqueConnID)
{
  S_CIP_Class *pstConnectionManager;

  initializeConnectionManagerData();

  pstConnectionManager = createCIPClass(CIP_CONNECTION_MANAGER_CLASS_CODE, /* class ID*/
  0, /* # of class attributes */
  0xffffffff, /* class getAttributeAll mask */
  0, /* # of class services */
  0, /* # of instance attributes */
  0xffffffff, /* instance getAttributeAll mask */
  3, /* # of instance services */
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

  g_nIncarnationID = ((EIP_UINT32) pa_nUniqueConnID) << 16;

  addConnectableObject(CIP_MESSAGE_ROUTER_CLASS_CODE,
      establishClass3Connection);
  addConnectableObject(CIP_ASSEMBLY_CLASS_CODE, establishIOConnction);

  return EIP_OK;
}

EIP_STATUS
handleReceivedConnectedData(EIP_UINT8 * pa_pnData, int pa_nDataLength,
    struct sockaddr_in *pa_pstFromAddr)
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
          || (g_stCPFDataItem.stAddr_Item.TypeID == CIP_ITEM_ID_SEQUENCEDADDRESS))
        { /* found connected address item or found sequenced address item -> for now the sequence number will be ignored */
          if (g_stCPFDataItem.stDataI_Item.TypeID
              == CIP_ITEM_ID_CONNECTIONTRANSPORTPACKET)
            { /* connected data item received */
              pstConnectionObject = getConnectedObject(
                  g_stCPFDataItem.stAddr_Item.Data.ConnectionIdentifier);
              if (pstConnectionObject == 0)
                return EIP_ERROR;

              /* only handle the data if it is coming from the originator */
              if (pstConnectionObject->m_stOriginatorAddr.sin_addr.s_addr
                  == pa_pstFromAddr->sin_addr.s_addr)
                {

                  if (SEQ_GEQ32(g_stCPFDataItem.stAddr_Item.Data.SequenceNumber, pstConnectionObject->EIPSequenceCountConsuming))
                    {
                      /* reset the watchdog timer */
                      pstConnectionObject->InnacitvityWatchdogTimer =
                          (pstConnectionObject->O_to_T_RPI / 1000)
                              << (2
                                  + pstConnectionObject->ConnectionTimeoutMultiplier);

                      /* only inform assembly object if the sequence counter is greater or equal */
                      pstConnectionObject->EIPSequenceCountConsuming =
                          g_stCPFDataItem.stAddr_Item.Data.SequenceNumber;

                      if (NULL != pstConnectionObject->m_pfReceiveDataFunc)
                        {
                          return pstConnectionObject->m_pfReceiveDataFunc(
                              pstConnectionObject,
                              g_stCPFDataItem.stDataI_Item.Data,
                              g_stCPFDataItem.stDataI_Item.Length);
                        }
                    }
                }
              else
                {
                  OPENER_TRACE_WARN("Connected Message Data Received with wrong address information\n");
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
    S_CIP_MR_Response *pa_MRResponse)
{
  EIP_UINT16 nConnectionStatus = CIP_CON_MGR_SUCCESS;
  EIP_UINT32 tmp;
  TConnMgmHandling *pstConnMgmEntry;

  (void) pa_pstInstance; /*suppress compiler warning */

  /*first check if we have already a connection with the given params */
  g_stDummyConnectionObject.Priority_Timetick = *pa_MRRequest->Data++;
  g_stDummyConnectionObject.Timeoutticks = *pa_MRRequest->Data++;
  /* O_to_T Conn ID */
  g_stDummyConnectionObject.CIPConsumedConnectionID = ltohl(
      &pa_MRRequest->Data);
  /* T_to_O Conn ID */
  g_stDummyConnectionObject.CIPProducedConnectionID = ltohl(
      &pa_MRRequest->Data);
  g_stDummyConnectionObject.ConnectionSerialNumber = ltohs(&pa_MRRequest->Data);
  g_stDummyConnectionObject.OriginatorVendorID = ltohs(&pa_MRRequest->Data);
  g_stDummyConnectionObject.OriginatorSerialNumber = ltohl(&pa_MRRequest->Data);

  if ((NULL != checkForExistingConnection(&g_stDummyConnectionObject)))
    {
      /* TODO this test is  incorrect, see CIP spec 3-5.5.2 re: duplicate forward open
       it should probably be testing the connection type fields
       TODO think on how a reconfigurationrequest could be handled correctly */
      if ((0 == g_stDummyConnectionObject.CIPConsumedConnectionID)
          && (0 == g_stDummyConnectionObject.CIPProducedConnectionID))
        {
          /*TODO implement reconfiguration of connection*/

          OPENER_TRACE_ERR("this looks like a duplicate forward open -- I can't handle this yet, sending a CIP_CON_MGR_ERROR_CONNECTION_IN_USE response\n");
        }
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          CIP_ERROR_CONNECTION_FAILURE, CIP_CON_MGR_ERROR_CONNECTION_IN_USE);
    }
  /* keep it to none existent till the setup is done this eases error handling and
   * the state changes within the forward open request can not be detected from
   * the application or from outside (reason we are single threaded)*/
  g_stDummyConnectionObject.State = CONN_STATE_NONEXISTENT;
  g_stDummyConnectionObject.SequenceCountProducing = 0; /* set the sequence count to zero */

  g_stDummyConnectionObject.ConnectionTimeoutMultiplier = *pa_MRRequest->Data++;
  pa_MRRequest->Data += 3; /* reserved */
  /* the requested packet interval parameter needs to be a multiple of TIMERTICK from the header file */
  OPENER_TRACE_INFO("ForwardOpen: ConConnID %lu, ProdConnID %lu, ConnSerNo %u\n",
      g_stDummyConnectionObject.CIPConsumedConnectionID,
      g_stDummyConnectionObject.CIPProducedConnectionID,
      g_stDummyConnectionObject.ConnectionSerialNumber);

  g_stDummyConnectionObject.O_to_T_RPI = ltohl(&pa_MRRequest->Data);

  g_stDummyConnectionObject.O_to_T_NetworkConnectionParameter = ltohs(
      &pa_MRRequest->Data);
  g_stDummyConnectionObject.T_to_O_RPI = ltohl(&pa_MRRequest->Data);

  tmp = g_stDummyConnectionObject.T_to_O_RPI % (OPENER_TIMER_TICK * 1000);
  if (tmp > 0)
    {
      g_stDummyConnectionObject.T_to_O_RPI =
          (EIP_UINT32) (g_stDummyConnectionObject.T_to_O_RPI
              / (OPENER_TIMER_TICK * 1000)) * (OPENER_TIMER_TICK * 1000)
              + (OPENER_TIMER_TICK * 1000);
    }

  g_stDummyConnectionObject.T_to_O_NetworkConnectionParameter = ltohs(
      &pa_MRRequest->Data);

  /*check if Network connection paramters are ok */
  if ((CIP_CONN_TYPE_MASK
      == (g_stDummyConnectionObject.O_to_T_NetworkConnectionParameter
          & CIP_CONN_TYPE_MASK))
      || (CIP_CONN_TYPE_MASK
          == (g_stDummyConnectionObject.T_to_O_NetworkConnectionParameter
              & CIP_CONN_TYPE_MASK)))
    {
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          CIP_ERROR_CONNECTION_FAILURE,
          CIP_CON_MGR_ERROR_INVALID_CONNECTION_TYPE);
    }

  g_stDummyConnectionObject.TransportTypeClassTrigger = *pa_MRRequest->Data++;
  /*check if the trigger type value is ok */
  if (0x40 & g_stDummyConnectionObject.TransportTypeClassTrigger)
    {
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          CIP_ERROR_CONNECTION_FAILURE,
          CIP_CON_MGR_ERROR_TRANSPORT_TRIGGER_NOT_SUPPORTED);
    }

  tmp = parseConnectionPath(&g_stDummyConnectionObject, pa_MRRequest,
      &nConnectionStatus);
  if (EIP_OK != tmp)
    {
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          tmp, nConnectionStatus);
    }

  /*parsing is now finished all data is available and check now establish the connection */
  pstConnMgmEntry = getConnMgmEntry(
      g_stDummyConnectionObject.ConnectionPath.ClassID);
  if (NULL != pstConnMgmEntry)
    {
      tmp = pstConnMgmEntry->m_pfOpenFunc(&g_stDummyConnectionObject,
          &nConnectionStatus);
    }
  else
    {
      tmp = EIP_ERROR;
      nConnectionStatus = CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
    }

  if (EIP_OK != tmp)
    {
      OPENER_TRACE_INFO("connection manager: connect failed\n");
      /* in case of error the dummy objects holds all necessary information */
      return assembleFWDOpenResponse(&g_stDummyConnectionObject, pa_MRResponse,
          tmp, nConnectionStatus);
    }
  else
    {
      OPENER_TRACE_INFO("connection manager: connect succeeded\n");
      /* in case of success the g_pstActiveConnectionList points to the new connection */
      return assembleFWDOpenResponse(g_pstActiveConnectionList, pa_MRResponse,
          CIP_ERROR_SUCCESS, 0);
    }
}

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

  pa_pstConnObj->WatchdogTimeoutAction = enWatchdogAutoDelete; /* the default for all connections on EIP*/

  pa_pstConnObj->ExpectedPacketRate = 0; /* default value */

  if ((pa_pstConnObj->TransportTypeClassTrigger & 0x80) == 0x00)
    { /* Client Type Connection requested */
      pa_pstConnObj->ExpectedPacketRate =
          (EIP_UINT16) ((pa_pstConnObj->T_to_O_RPI) / 1000);
      /* As soon as we are ready we should produce the connection. With the 0 here we will produce with the next timer tick
       * which should be sufficient. */
      pa_pstConnObj->TransmissionTriggerTimer = 0;
    }
  else
    {
      /* Server Type Connection requested */
      pa_pstConnObj->ExpectedPacketRate =
          (EIP_UINT16) ((pa_pstConnObj->O_to_T_RPI) / 1000);
    }

  pa_pstConnObj->m_nProductionInhibitTimer =
      pa_pstConnObj->m_unProductionInhibitTime = 0;

  /*setup the preconsuption timer: max(ConnectionTimeoutMultiplier * EpectetedPacketRate, 10s) */
  pa_pstConnObj->InnacitvityWatchdogTimer =
      ((((pa_pstConnObj->O_to_T_RPI) / 1000)
          << (2 + pa_pstConnObj->ConnectionTimeoutMultiplier)) > 10000) ? (((pa_pstConnObj->O_to_T_RPI)
          / 1000) << (2 + pa_pstConnObj->ConnectionTimeoutMultiplier)) :
          10000;

  pa_pstConnObj->ConsumedConnectionSize =
      pa_pstConnObj->O_to_T_NetworkConnectionParameter & 0x01FF;

  pa_pstConnObj->ProducedConnectionSize =
      pa_pstConnObj->T_to_O_NetworkConnectionParameter & 0x01FF;

}

EIP_STATUS
ForwardClose(S_CIP_Instance *pa_pstInstance, S_CIP_MR_Request * pa_MRRequest,
    S_CIP_MR_Response * pa_MRResponse)
{
  EIP_UINT16 ConnectionSerialNr, OriginatorVendorID;
  EIP_UINT32 OriginatorSerialNr;
  S_CIP_ConnectionObject *pstRunner;
  EIP_UINT16 nConnectionStatus;

  /*Suppress compiler warning*/
  (void) pa_pstInstance;

  /* check connection serial number && Vendor ID && OriginatorSerialNr if connection is established */
  nConnectionStatus =
      CIP_CON_MGR_ERROR_CONNECTION_NOT_FOUND_AT_TARGET_APPLICATION;
  pstRunner = g_pstActiveConnectionList;

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
      if ((pstRunner->State == CONN_STATE_ESTABLISHED)
          || (pstRunner->State == CONN_STATE_TIMEDOUT))
        {
          if ((pstRunner->ConnectionSerialNumber == ConnectionSerialNr)
              && (pstRunner->OriginatorVendorID == OriginatorVendorID)
              && (pstRunner->OriginatorSerialNumber == OriginatorSerialNr))
            {
              /* found the corresponding connection object -> close it */OPENER_ASSERT(
                  NULL != pstRunner->m_pfCloseFunc);
              pstRunner->m_pfCloseFunc(pstRunner);
              nConnectionStatus = CIP_CON_MGR_SUCCESS;
              break;
            }
        }
      pstRunner = pstRunner->m_pstNext;
    }

  return assembleFWDCloseResponse(ConnectionSerialNr, OriginatorVendorID,
      OriginatorSerialNr, pa_MRRequest, pa_MRResponse, nConnectionStatus);
}

EIP_STATUS
GetConnectionOwner(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_MRRequest, S_CIP_MR_Response * pa_MRResponse)
{
  /* suppress compiler warnings */
  (void) pa_pstInstance;
  (void) pa_MRRequest;
  (void) pa_MRResponse;

  return EIP_OK;
}

EIP_STATUS
manageConnections(void)
{
  EIP_STATUS res;
  S_CIP_ConnectionObject *pstRunner;

  /*Inform application that it can execute                                    */
  IApp_HandleApplication();
  manageEncapsulationMessages();

  pstRunner = g_pstActiveConnectionList;
  while (NULL != pstRunner)
    {
      if (pstRunner->State == CONN_STATE_ESTABLISHED)
        {
          if ((0 != pstRunner->p_stConsumingInstance) || /* we have a consuming connection check innacitivitywatchdog timer */
          (pstRunner->TransportTypeClassTrigger & 0x80)) /* all sever connections have to maintain an innacitivitywatchdog timer */
            {
              pstRunner->InnacitvityWatchdogTimer -= OPENER_TIMER_TICK;
              if (pstRunner->InnacitvityWatchdogTimer <= 0)
                {
                  /* we have a timed out connection perform watchdog time out action*/
                  OPENER_TRACE_INFO(">>>>>>>>>>Connection timed out\n");OPENER_ASSERT(
                      NULL != pstRunner->m_pfTimeOutFunc);
                  pstRunner->m_pfTimeOutFunc(pstRunner);
                }
            }
          /* only if the connection has not timed out check if data is to be send */
          if (CONN_STATE_ESTABLISHED == pstRunner->State)
            {
              /* client connection */
              if ((pstRunner->ExpectedPacketRate != 0)
                  && (EIP_INVALID_SOCKET != pstRunner->sockfd[PRODUCING])) /* only produce for the master connection */
                {
                  if (CIP_CONN_CYCLIC_CONNECTION
                      != (pstRunner->TransportTypeClassTrigger
                          & CIP_CONN_PRODUCTION_TRIGGER_MASK))
                    {
                      /* non cyclic connections have to decrement production inhibit timer */
                      if (0 <= pstRunner->m_nProductionInhibitTimer)
                        {
                          pstRunner->m_nProductionInhibitTimer -=
                              OPENER_TIMER_TICK;
                        }
                    }
                  pstRunner->TransmissionTriggerTimer -= OPENER_TIMER_TICK;
                  if (pstRunner->TransmissionTriggerTimer <= 0)
                    { /* need to send package */
                      OPENER_ASSERT(NULL != pstRunner->m_pfSendDataFunc);
                      res = pstRunner->m_pfSendDataFunc(pstRunner);
                      if (res == EIP_ERROR)
                        {
                          OPENER_TRACE_ERR(
                              "sending of UDP data in manage Connection failed\n");
                        }
                      /* reload the timer value */
                      pstRunner->TransmissionTriggerTimer =
                          pstRunner->ExpectedPacketRate;
                      if (CIP_CONN_CYCLIC_CONNECTION
                          != (pstRunner->TransportTypeClassTrigger
                              & CIP_CONN_PRODUCTION_TRIGGER_MASK))
                        {
                          /* non cyclic connections have to reload the production inhibit timer */
                          pstRunner->m_nProductionInhibitTimer =
                              pstRunner->m_unProductionInhibitTime;
                        }
                    }
                }
            }
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return EIP_OK;
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
    EIP_UINT16 pa_nExtendedStatus)
{
  /* write reply information in CPF struct dependent of pa_status */
  S_CIP_CPF_Data *pa_CPF_data = &g_stCPFDataItem;
  EIP_BYTE *paMsg = pa_MRResponse->Data;
  pa_CPF_data->ItemCount = 2;
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

      htoll(pa_pstConnObj->CIPConsumedConnectionID, &paMsg);
      htoll(pa_pstConnObj->CIPProducedConnectionID, &paMsg);
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

  htols(pa_pstConnObj->ConnectionSerialNumber, &paMsg);
  htols(pa_pstConnObj->OriginatorVendorID, &paMsg);
  htoll(pa_pstConnObj->OriginatorSerialNumber, &paMsg);

  if (CIP_ERROR_SUCCESS == pa_nGeneralStatus)
    {
      /* set the actual packet rate to requested packet rate */
      htoll(pa_pstConnObj->O_to_T_RPI, &paMsg);
      htoll(pa_pstConnObj->T_to_O_RPI, &paMsg);
    }

  *paMsg = 0; /* remaining path size -  for routing devices relevant */
  paMsg++;
  *paMsg = 0; /* reserved */
  paMsg++;

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
    EIP_UINT16 pa_nExtErrorCode)
{
  /* write reply information in CPF struct dependent of pa_status */
  S_CIP_CPF_Data *pa_CPF_data = &g_stCPFDataItem;
  EIP_BYTE *paMsg = pa_MRResponse->Data;
  pa_CPF_data->ItemCount = 2;
  pa_CPF_data->stDataI_Item.TypeID = CIP_ITEM_ID_UNCONNECTEDMESSAGE;
  pa_CPF_data->stAddr_Item.TypeID = CIP_ITEM_ID_NULL;
  pa_CPF_data->stAddr_Item.Length = 0;

  htols(pa_ConnectionSerialNr, &paMsg);
  htols(pa_OriginatorVendorID, &paMsg);
  htoll(pa_OriginatorSerialNr, &paMsg);

  pa_MRResponse->ReplyService = (0x80 | pa_MRRequest->Service);
  pa_MRResponse->DataLength = 10; /* if there is no application specific data */

  if (CIP_CON_MGR_SUCCESS == pa_nExtErrorCode)
    {
      *paMsg = 0; /* no application data */
      pa_MRResponse->GeneralStatus = CIP_ERROR_SUCCESS;
      pa_MRResponse->SizeofAdditionalStatus = 0;
    }
  else
    {
      *paMsg = *pa_MRRequest->Data; /* remaining path size */
      pa_MRResponse->GeneralStatus = CIP_ERROR_CONNECTION_FAILURE;
      pa_MRResponse->AdditionalStatus[0] = pa_nExtErrorCode;
      pa_MRResponse->SizeofAdditionalStatus = 1;
    }

  paMsg++;
  *paMsg = 0; /* reserved */
  paMsg++;

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
                      if ((pa_pstKeyData->MajorRevision != Revison.MajorRevision)
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

  /* with 256 we mark that we haven't got a PIT segment */
  pa_pstConnObj->m_unProductionInhibitTime = 256;

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

          if (EIP_OK
              != checkElectronicKeyData(pa_pstConnObj->ElectronicKey.KeyFormat,
                  &(pa_pstConnObj->ElectronicKey.KeyData), pa_pnExtendedError))
            {
              return CIP_ERROR_CONNECTION_FAILURE;
            }
        }
      else
        {
          OPENER_TRACE_INFO("no key\n");
        }

      if (CIP_CONN_CYCLIC_CONNECTION
          != (pa_pstConnObj->TransportTypeClassTrigger
              & CIP_CONN_PRODUCTION_TRIGGER_MASK))
        {
          /*non cyclic connections may have a production inhibit */
          if (CIP_CONN_PIT_NETWORK_SEGEMENT_ID == *pnMsg)
            {
              pa_pstConnObj->m_unProductionInhibitTime = pnMsg[1];
              pnMsg += 2;
              nRemainingPathSize -= 1;
            }
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
                  *pa_pnExtendedError =
                      CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
                }
              else
                {
                  *pa_pnExtendedError =
                      CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
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
          pa_pstConnObj->ConnectionPath.ConnectionPoint[2] =
              GETPADDEDLOGICALPATH(&pnMsg);
          OPENER_TRACE_INFO("Configuration instance id %ld\n", pa_pstConnObj->ConnectionPath.ConnectionPoint[2]);
          if (NULL
              == getCIPInstance(pstClass,
                  pa_pstConnObj->ConnectionPath.ConnectionPoint[2]))
            {
              /*according to the test tool we should respond with this extended error code */
              *pa_pnExtendedError =
                  CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
              return CIP_ERROR_CONNECTION_FAILURE;
            }
          /* 1 or 2 16Bit words for the configuration instance part of the path  */
          nRemainingPathSize -=
              (pa_pstConnObj->ConnectionPath.ConnectionPoint[2] > 0xFF) ? 2 : 1;
        }
      else
        {
          OPENER_TRACE_INFO("no config data\n");
        }

      if (0x03 == (pa_pstConnObj->TransportTypeClassTrigger & 0x03))
        {
          /*we have Class 3 connection*/
          if (nRemainingPathSize > 0)
            {
              OPENER_TRACE_WARN(
                  "Too much data in connection path for class 3 connection\n");
              *pa_pnExtendedError =
                  CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
              return CIP_ERROR_CONNECTION_FAILURE;
            }

          /* connection end point has to be the message router instance 1 */
          if ((pa_pstConnObj->ConnectionPath.ClassID
              != CIP_MESSAGE_ROUTER_CLASS_CODE)
              || (pa_pstConnObj->ConnectionPath.ConnectionPoint[2] != 1))
            {
              *pa_pnExtendedError = CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
              return CIP_ERROR_CONNECTION_FAILURE;
            }
          pa_pstConnObj->ConnectionPath.ConnectionPoint[0] =
              pa_pstConnObj->ConnectionPath.ConnectionPoint[2];
        }
      else
        { /* we have an IO connection */
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
                  pa_pstConnObj->ConnectionPath.ConnectionPoint[i] =
                      GETPADDEDLOGICALPATH(&pnMsg);
                  OPENER_TRACE_INFO("connection point %lu\n",
                      pa_pstConnObj->ConnectionPath.ConnectionPoint[i]);
                  if (0
                      == getCIPInstance(pstClass,
                          pa_pstConnObj->ConnectionPath.ConnectionPoint[i]))
                    {
                      *pa_pnExtendedError =
                          CIP_CON_MGR_ERROR_INVALID_CONNECTION_POINT;
                      return CIP_ERROR_CONNECTION_FAILURE;
                    }
                  /* 1 or 2 16Bit word for the connection point part of the path */
                  nRemainingPathSize -=
                      (pa_pstConnObj->ConnectionPath.ConnectionPoint[i] > 0xFF) ? 2 :
                          1;
                }
              else
                {
                  *pa_pnExtendedError =
                      CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH;
                  return CIP_ERROR_CONNECTION_FAILURE;
                }
            }

          g_unConfigDataLen = 0;
          g_pnConfigDataBuffer = NULL;

          while (nRemainingPathSize > 0)
            { /* have something left in the path should be configuration data */

              switch (*pnMsg)
                {
              case 0x80:
                /* we have a simple data segment */
                g_unConfigDataLen = pnMsg[1] * 2; /*data segments store length 16-bit word wise */
                g_pnConfigDataBuffer = &(pnMsg[2]);
                nRemainingPathSize -= (g_unConfigDataLen + 2);
                pnMsg += (g_unConfigDataLen + 2);
                break;
                /*TODO do we have to handle ANSI extended symbol data segments too? */
              case CIP_CONN_PIT_NETWORK_SEGEMENT_ID:
                if (CIP_CONN_CYCLIC_CONNECTION
                    != (pa_pstConnObj->TransportTypeClassTrigger
                        & CIP_CONN_PRODUCTION_TRIGGER_MASK))
                  {
                    /* only non cyclic connections may have a production inhibit */
                    pa_pstConnObj->m_unProductionInhibitTime = pnMsg[1];
                    pnMsg += 2;
                    nRemainingPathSize -= 2;
                  }
                else
                  {
                    *pa_pnExtendedError = pa_pstConnObj->ConnectionPathSize
                        - nRemainingPathSize; /*offset in 16Bit words where within the connection path the error happend*/
                    return 0x04; /*status code for invalid segment type*/
                  }
                break;
              default:
                OPENER_TRACE_WARN(
                    "No data segment identifier found for the configuration data\n");
                *pa_pnExtendedError = pa_pstConnObj->ConnectionPathSize
                    - nRemainingPathSize; /*offset in 16Bit words where within the connection path the error happend*/
                return 0x04; /*status code for invalid segment type*/
                break;
                }
            }
        }
    }

  /*save back the current position in the stream allowing followers to parse anything thats still there*/
  pa_MRRequest->Data = pnMsg;
  return EIP_OK;
}

void
closeConnection(S_CIP_ConnectionObject *pa_pstConnObj)
{
  pa_pstConnObj->State = CONN_STATE_NONEXISTENT;
  if (0x03 != (pa_pstConnObj->TransportTypeClassTrigger & 0x03))
    {
      /* only close the udp connection for not class 3 connections */
      IApp_CloseSocket(pa_pstConnObj->sockfd[CONSUMING]);
      pa_pstConnObj->sockfd[CONSUMING] = EIP_INVALID_SOCKET;
      IApp_CloseSocket(pa_pstConnObj->sockfd[PRODUCING]);
      pa_pstConnObj->sockfd[PRODUCING] = EIP_INVALID_SOCKET;
    }
  removeFromActiveConnections(pa_pstConnObj);
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
  g_pstActiveConnectionList->State = CONN_STATE_ESTABLISHED;
}

void
removeFromActiveConnections(S_CIP_ConnectionObject *pa_pstConn)
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
  pa_pstConn->State = CONN_STATE_NONEXISTENT;
}

EIP_BOOL8
isConnectedOutputAssembly(EIP_UINT32 pa_nInstanceNr)
{
  EIP_BOOL8 bRetVal = false;

  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if (pa_nInstanceNr == pstRunner->ConnectionPath.ConnectionPoint[0])
        {
          bRetVal = true;
          break;
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return bRetVal;
}

EIP_STATUS
addConnectableObject(EIP_UINT32 pa_nClassId, TConnOpenFunc pa_pfOpenFunc)
{
  int i;
  EIP_STATUS nRetVal;
  nRetVal = EIP_ERROR;

  /*parsing is now finished all data is available and check now establish the connection */
  for (i = 0; i < scg_nNumConnectableObjects; ++i)
    {
      if ((0 == g_astConnMgmList[i].m_nClassID)
          || (pa_nClassId == g_astConnMgmList[i].m_nClassID))
        {
          g_astConnMgmList[i].m_nClassID = pa_nClassId;
          g_astConnMgmList[i].m_pfOpenFunc = pa_pfOpenFunc;
          return EIP_OK;
        }
    }

  return EIP_ERROR;
}

TConnMgmHandling *
getConnMgmEntry(EIP_UINT32 pa_nClassId)
{
  int i;
  TConnMgmHandling *pstRetVal;

  pstRetVal = NULL;

  for (i = 0; i < scg_nNumConnectableObjects; ++i)
    {
      if (pa_nClassId == g_astConnMgmList[i].m_nClassID)
        {
          pstRetVal = &(g_astConnMgmList[i]);
          break;
        }
    }
  return pstRetVal;
}

EIP_STATUS
triggerConnections(unsigned int pa_unOutputAssembly,
    unsigned int pa_unInputAssembly)
{
  EIP_STATUS nRetVal = EIP_ERROR;

  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;
  while (NULL != pstRunner)
    {
      if ((pa_unOutputAssembly == pstRunner->ConnectionPath.ConnectionPoint[0])
          && (pa_unInputAssembly == pstRunner->ConnectionPath.ConnectionPoint[1]))
        {
          if (CIP_CONN_APLICATION_TRIGGERED_CONNECTION
              == (pstRunner->TransportTypeClassTrigger
                  & CIP_CONN_PRODUCTION_TRIGGER_MASK))
            {
              /* produce at the next allowed occurrence */
              pstRunner->TransmissionTriggerTimer =
                  pstRunner->m_nProductionInhibitTimer;
              nRetVal = EIP_OK;
            }
          break;
        }
    }
  return nRetVal;
}

void
initializeConnectionManagerData()
{
  memset(g_astConnMgmList, 0,
      scg_nNumConnectableObjects * sizeof(TConnMgmHandling));
  initializeClass3ConnectionData();
  initializeIOConnectionData();
}
