/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef CIPCONNECTIONMANAGER_H_
#define CIPCONNECTIONMANAGER_H_

#include "opener_user_conf.h"
#include "opener_api.h"
#include "typedefs.h"
#include "ciptypes.h"

#define CONSUMING 0           /* these are used as array indexes, watch out if changing these values */
#define PRODUCING 1

#define CIP_POINT_TO_POINT_CONNECTION 0x4000
#define CIP_MULTICAST_CONNECTION      0x2000


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
#define CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH 0x0117
#define CIP_CON_MGR_INVALID_OR_INCONSISTENT_CONFIGURATION_APPLICATION_PATH 0x0117
#define CIP_CON_MGR_ERROR_INVALID_CONFIGURATION_FORMAT 0x0118
#define CIP_CON_MGR_NON_LISTEN_ONLY_CONNECTION_NOT_OPENED 0x0119
#define CIP_CON_MGR_ERROR_PARAMETER_ERROR_IN_UNCONNECTED_SEND_SERVICE 0x0205
#define CIP_CON_MGR_ERROR_INVALID_SEGMENT_TYPE_IN_PATH 0x0315
#define CIP_CON_MGR_TARGET_OBJECT_OUT_OF_CONNECTIONS 0x011A

#define CIP_CONN_PRODUCTION_TRIGGER_MASK 0x70
#define CIP_CONN_CYCLIC_CONNECTION       0x0
#define CIP_CONN_COS_TRIGGERED_CONNECTION 0x10
#define CIP_CONN_APLICATION_TRIGGERED_CONNECTION 0x20

/*macros for comparing sequence numbers according to CIP spec vol 2 3-4.2*/
#define SEQ_LEQ32(a, b) ((int)((a) - (b)) <= 0)
#define SEQ_GEQ32(a, b) ((int)((a) - (b)) >= 0)

/* similar macros for comparing 16 bit sequence numbers */
#define SEQ_LEQ16(a, b) ((short)((a) - (b)) <= 0)
#define SEQ_GEQ16(a, b) ((short)((a) - (b)) >= 0)


/*! States of a connection */
typedef enum
{
  CONN_STATE_NONEXISTENT = 0,
  CONN_STATE_CONFIGURING = 1,
  CONN_STATE_WAITINGFORCONNECTIONID = 2 /* only used in DeviceNet*/,
  CONN_STATE_ESTABLISHED = 3,
  CONN_STATE_TIMEDOUT = 4,
  CONN_STATE_DEFERREDDELETE = 5 /* only used in DeviceNet */,
  CONN_STATE_CLOSING
} CONN_STATE;

/* instance_type attributes */
typedef enum
{
  enConnTypeExplicit         = 0,
  enConnTypeIOExclusiveOwner = 0x01,
  enConnTypeIOInputOnly      = 0x11,
  enConnTypeIOListenOnly     = 0x21
} EConnType;

/*! Possible values for the watch dog time out action of a connection */
typedef enum
{
  enWatchdogTransitionToTimedOut = 0, /*!< , invalid for explicit message connections */
  enWatchdogAutoDelete = 1, /*!< Default for explicit message connections, default for I/O connections on EIP*/
  enWatchdogAutoReset = 2, /*!< Invalid for explicit message connections */
  enWatchdogDeferredDelete = 3
/*!< Only valid for DeviceNet, invalid for I/O connections */
} EWatchdogTimeOutAction;

typedef struct
{
  CONN_STATE state;
  EIP_UINT16 ConnectionID;
/*TODO think if this is needed anymore
 TCMReceiveDataFunc m_ptfuncReceiveData; */
} S_Link_Consumer;

typedef struct
{
  CONN_STATE state;
  EIP_UINT16 ConnectionID;
} S_Link_Producer;

typedef struct
{
  S_Link_Consumer Consumer;
  S_Link_Producer Producer;
} S_Link_Object;

/*! The data needed for handling connections. This data is strongly related to
 * the connection object defined in the CIP-specification. However the full
 * functionality of the connection object is not implemented. Therefore this
 * data can not be accessed with CIP means.
 */
typedef struct CIP_ConnectionObject
{
  CONN_STATE State;
  EConnType m_eInstanceType;

  /* conditional
   UINT16 DeviceNetProductedConnectionID;
   UINT16 DeviceNetConsumedConnectionID;
   */
  EIP_BYTE DeviceNetInitialCommCharacteristics;
  EIP_UINT16 ProducedConnectionSize;
  EIP_UINT16 ConsumedConnectionSize;
  EIP_UINT16 ExpectedPacketRate;

  /*conditional*/
  EIP_UINT32 CIPProducedConnectionID;
  EIP_UINT32 CIPConsumedConnectionID;
  /**/
  EWatchdogTimeOutAction WatchdogTimeoutAction;
  EIP_UINT16 ProducedConnectionPathLength;
  S_CIP_EPATH ProducedConnectionPath;
  EIP_UINT16 ConsumedConnectionPathLength;
  S_CIP_EPATH ConsumedConnectionPath;
  /* conditional
   UINT16 ProductionInhibitTime;
   */
  /* non CIP Attributes, only relevant for opened connections */
  EIP_BYTE Priority_Timetick;
  EIP_UINT8 Timeoutticks;
  EIP_UINT16 ConnectionSerialNumber;
  EIP_UINT16 OriginatorVendorID;
  EIP_UINT32 OriginatorSerialNumber;
  EIP_UINT16 ConnectionTimeoutMultiplier;
  EIP_UINT32 O_to_T_RPI;
  EIP_UINT16 O_to_T_NetworkConnectionParameter;
  EIP_UINT32 T_to_O_RPI;
  EIP_UINT16 T_to_O_NetworkConnectionParameter;
  EIP_BYTE TransportTypeClassTrigger;
  EIP_UINT8 ConnectionPathSize;
  S_CIP_ElectronicKey ElectronicKey;
  S_CIP_ConnectionPath ConnectionPath; /* padded EPATH*/
  S_Link_Object stLinkObject;

  S_CIP_Instance *p_stConsumingInstance;
  /*S_CIP_CM_Object *p_stConsumingCMObject; */

  S_CIP_Instance *p_stProducingInstance;
  /*S_CIP_CM_Object *p_stProducingCMObject; */

  EIP_UINT32 EIPSequenceCountProducing; /* the EIP level sequence Count for Class 0/1 Producing Connections may have a different value than SequenceCountProducing*/
  EIP_UINT32 EIPSequenceCountConsuming; /* the EIP level sequence Count for Class 0/1 Producing Connections may have a different value than SequenceCountProducing*/

  EIP_UINT16 SequenceCountProducing; /* sequence Count for Class 1 Producing Connections*/
  EIP_UINT16 SequenceCountConsuming; /* sequence Count for Class 1 Producing Connections*/

  EIP_INT32 TransmissionTriggerTimer;
  EIP_INT32 InnacitvityWatchdogTimer;


  /*! Minimal time between the production of two application triggered or change of state triggered
   * I/O connection messages
   */
  EIP_UINT16 m_unProductionInhibitTime;
  /*! Timer for the production inhibition of application triggered or change-of-state
   *  I/O connections.
   */
  EIP_INT32 m_nProductionInhibitTimer;

  struct sockaddr_in remote_addr; /* socket address for produce */
  struct sockaddr_in m_stOriginatorAddr;  /* the address of the originator that established the connection. needed for scanning if the right packet is arriving */
  int sockfd[2]; /* socket handles, indexed by CONSUMING or PRODUCING */

  /* pointers to connection handling functions */
  TConnCloseFunc m_pfCloseFunc;
  TConnTimeOutFunc m_pfTimeOutFunc;
  TConnSendDataFunc m_pfSendDataFunc;
  TConnRecvDataFunc m_pfReceiveDataFunc;

  /* pointers to be used in the active connection list */
  struct CIP_ConnectionObject *m_pstNext;
  struct CIP_ConnectionObject *m_pstFirst;
} S_CIP_ConnectionObject;

#define CIP_CONNECTION_MANAGER_CLASS_CODE 0x06

/* public functions */

/*! Initialize the data of the connection manager object
 */
EIP_STATUS
Connection_Manager_Init(EIP_UINT16 pa_nUniqueConnID);

/*!  Get a connected object dependent on requested ConnectionID.
 *
 *   @param ConnectionID  requested ConnectionID of opened connection
 *   @return pointer to connected Object
 *           0 .. connection not present in device
 */
S_CIP_ConnectionObject *
getConnectedObject(EIP_UINT32 ConnectionID);

/*!  Get a connection object for a given output assembly.
 *
 *   @param pa_unOutputAssemblyId requested output assembly of requested connection
 *   @return pointer to connected Object
 *           0 .. connection not present in device
 */
S_CIP_ConnectionObject *
getConnectedOutputAssembly(EIP_UINT32 pa_unOutputAssemblyId);


/*! Copy the given connection data from pa_pstSrc to pa_pstDst
 */
void
copyConnectionData(S_CIP_ConnectionObject *pa_pstDst,
    S_CIP_ConnectionObject *pa_pstSrc);

/** \brief Close the given connection
 *
 * This function will take the data form the connection and correctly closes the connection (e.g., open sockets)
 * @param pa_pstConnObj pointer to the connection object structure to be closed
 */
void
closeConnection(S_CIP_ConnectionObject *pa_pstConnObj);

EIP_BOOL8 isConnectedOutputAssembly(EIP_UINT32 pa_nInstanceNr);

/** \brief Generate the ConnectionIDs and set the general configuration parameter
 * in the given connection object.
 *
 * @param pa_pstConnObj pointer to the connection object that should be set up.
 */
void
generalConnectionConfiguration(S_CIP_ConnectionObject *pa_pstConnObj);


/** \brief Insert the given connection object to the list of currently active and managed connections.
 *
 * By adding a connection to the active connection list the connection manager will
 * perform the supervision and handle the timing (e.g., timeout, production inhibit, etc).
 *
 * @param pa_pstConn pointer to the connection object to be added.
 */
void
addNewActiveConnection(S_CIP_ConnectionObject *pa_pstConn);

void
removeFromActiveConnections(S_CIP_ConnectionObject *pa_pstConn);

#endif /*CIPCONNECTIONMANAGER_H_*/

