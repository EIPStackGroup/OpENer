/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <string.h>
#include <stdbool.h>

#include "cipconnectionmanager.h"

#include "opener_user_conf.h"
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
#include "generic_networkhandler.h"
#include "cipepath.h"
#include "cipelectronickey.h"

/* values needed from the CIP identity object */
extern EipUint16 vendor_id_;
extern EipUint16 device_type_;
extern EipUint16 product_code_;
extern CipRevision revision_;

#define CIP_CONN_TYPE_MASK 0x6000   /**< Bit mask filter on bit 13 & 14 */

const int g_kForwardOpenHeaderLength = 36; /**< the length in bytes of the forward open command specific data till the start of the connection path (including con path size)*/

/** @brief Compares the logical path on equality */
#define EQLOGICALPATH(x,y) ( ( (x) & 0xfc )==(y) )

static const int g_kNumberOfConnectableObjects = 2 +
                                                 OPENER_CIP_NUM_APPLICATION_SPECIFIC_CONNECTABLE_OBJECTS;

typedef struct {
  EipUint32 class_id;
  OpenConnectionFunction open_connection_function;
} ConnectionManagementHandling;

/* Connection Object functions */
ProductionTrigger GetProductionTrigger(
  const ConnectionObject *const connection_object) {
  const unsigned int ProductionTriggerMask = 0x70;

  switch (connection_object->transport_type_class_trigger
          & ProductionTriggerMask) {
    case 0x0:
      return kProductionTriggerCyclic;
    case 0x10:
      return kProductionTriggerChangeOfState;
    case 0x20:
      return kProductionTriggerApplicationObjectTriggered;
    default:
      return kProductionTriggerInvalid;
  }
}

void SetProductionTrigger(const enum ProductionTrigger production_trigger,
                          ConnectionObject *connection_object) {
  switch (production_trigger) {
    case kProductionTriggerCyclic:
      connection_object->transport_type_class_trigger = 0x0;
      break;
    case kProductionTriggerChangeOfState:
      connection_object->transport_type_class_trigger = 0x10;
      break;
    case kProductionTriggerApplicationObjectTriggered:
      connection_object->transport_type_class_trigger = 0x20;
      break;
    default:
      //TODO: Replace with assert?
      connection_object->transport_type_class_trigger = 0x30;
      break;           /**< Invalid value */
  }
}

CipUint GetProductionInhibitTime(
  const ConnectionObject *const connection_object) {
  return connection_object->production_inhibit_time;
}

void SetProductionInhibitTime(const EipUint16 production_inhibit_time,
                              ConnectionObject *const connection_object) {
  connection_object->production_inhibit_time = production_inhibit_time;
}

CipUdint GetTargetToOriginatorRequestedPackedInterval(
  const ConnectionObject *const connection_object) {
  return connection_object->t_to_o_requested_packet_interval;
}

/* Connection Object functions end */

/* global variables private */
/** List holding information on the object classes and open/close function
 * pointers to which connections may be established.
 */
ConnectionManagementHandling g_connection_management_list[2 +
                                                          OPENER_CIP_NUM_APPLICATION_SPECIFIC_CONNECTABLE_OBJECTS
];

/** List holding all currently active connections*/
/*@null@*/ ConnectionObject *g_active_connection_list = NULL;

/** buffer connection object needed for forward open */
ConnectionObject g_dummy_connection_object;

/** @brief Holds the connection ID's "incarnation ID" in the upper 16 bits */
EipUint32 g_incarnation_id;

/* private functions */
EipStatus ForwardOpen(CipInstance *instance,
                      CipMessageRouterRequest *message_router_request,
                      CipMessageRouterResponse *message_router_response,
                      struct sockaddr *originator_address);

EipStatus ForwardClose(CipInstance *instance,
                       CipMessageRouterRequest *message_router_request,
                       CipMessageRouterResponse *message_router_response,
                       struct sockaddr *originator_address);

EipStatus GetConnectionOwner(CipInstance *instance,
                             CipMessageRouterRequest *message_router_request,
                             CipMessageRouterResponse *message_router_response,
                             struct sockaddr *originator_address);

EipStatus AssembleForwardOpenResponse(
  ConnectionObject *connection_object,CipMessageRouterResponse *
  message_router_response,EipUint8 general_status,EipUint16 extended_status);

EipStatus AssembleForwardCloseResponse(
  EipUint16 connection_serial_number,
  EipUint16 originatior_vendor_id,
  EipUint32 originator_serial_number,
  CipMessageRouterRequest *
  message_router_request,
  CipMessageRouterResponse *
  message_router_response,
  EipUint16 extended_error_code);

/** @brief check if the data given in the connection object match with an already established connection
 *
 * The comparison is done according to the definitions in the CIP specification Section 3-5.5.2:
 * The following elements have to be equal: Vendor ID, Connection Serial Number, Originator Serial Number
 * @param connection_object connection object containing the comparison elements from the forward open request
 * @return
 *    - NULL if no equal established connection exists
 *    - pointer to the equal connection object
 */
ConnectionObject *CheckForExistingConnection(
  ConnectionObject *connection_object);

/** @brief Compare the electronic key received with a forward open request with the device's data.
 *
 * @param key_format format identifier given in the forward open request
 * @param key_data pointer to the electronic key data received in the forward open request
 * @param extended_status the extended error code in case an error happened
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
EipStatus CheckElectronicKeyData(EipUint8 key_format,
                                 CipKeyData *key_data,
                                 EipUint16 *extended_status);

/** @brief Parse the connection path of a forward open request
 *
 * This function will take the connection object and the received data stream and parse the connection path.
 * @param connection_object pointer to the connection object structure for which the connection should
 *                      be established
 * @param message_router_request pointer to the received request structure. The position of the data stream pointer has to be at the connection length entry
 * @param extended_status the extended error code in case an error happened
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
EipUint8 ParseConnectionPath(ConnectionObject *connection_object,
                             CipMessageRouterRequest *message_router_request,
                             EipUint16 *extended_error);

ConnectionManagementHandling *GetConnectionManagementEntry(EipUint32 class_id);

void InitializeConnectionManagerData(void);

void AddNullAddressItem(
  CipCommonPacketFormatData *common_data_packet_format_data);

/** @brief gets the padded logical path TODO: enhance documentation
 * @param logical_path_segment TheLogical Path Segment
 *
 * @return The padded logical path
 */
unsigned int GetPaddedLogicalPath(const EipUint8 **logical_path_segment) {
  unsigned int padded_logical_path = *(*logical_path_segment)++;

  if ( (padded_logical_path & 3) == 0 ) {
    padded_logical_path = *(*logical_path_segment)++;
  } else if ( (padded_logical_path & 3) == 1 ) {
    (*logical_path_segment)++;             /* skip pad */
    padded_logical_path = *(*logical_path_segment)++;
    padded_logical_path |= *(*logical_path_segment)++ << 8;
  } else {
    OPENER_TRACE_ERR("illegal logical path segment\n");
  }
  return padded_logical_path;
}

/** @brief Generate a new connection Id utilizing the Incarnation Id as
 * described in the EIP specs.
 *
 * A unique connectionID is formed from the boot-time-specified "incarnation ID"
 * and the per-new-connection-incremented connection number/counter.
 * @return new connection id
 */
EipUint32 GetConnectionId(void) {
  static EipUint32 connection_id = 18;
  connection_id++;
  return ( g_incarnation_id | (connection_id & 0x0000FFFF) );
}

EipStatus ConnectionManagerInit(EipUint16 unique_connection_id) {
  InitializeConnectionManagerData();

  CipClass *connection_manager = CreateCipClass(
    g_kCipConnectionManagerClassCode,                     /* class ID */
    0,                     /* # of class attributes */
    0xC6,                     /* class getAttributeAll mask */
    0,                     /* # of class services */
    0,                     /* # of instance attributes */
    0xffffffff,                     /* instance getAttributeAll mask */
    3,                     /* # of instance services */
    1,                     /* # of instances */
    "connection manager",                     /* class name */
    1);                     /* revision */
  if (connection_manager == NULL) {
    return kEipStatusError;
  }

  InsertService(connection_manager, kForwardOpen, &ForwardOpen,
                "ForwardOpen");
  InsertService(connection_manager, kForwardClose, &ForwardClose,
                "ForwardClose");
  InsertService(connection_manager, kGetConnectionOwner, &GetConnectionOwner,
                "GetConnectionOwner");

  g_incarnation_id = ( (EipUint32) unique_connection_id ) << 16;

  AddConnectableObject(kCipMessageRouterClassCode, EstablishClass3Connection);
  AddConnectableObject(kCipAssemblyClassCode, EstablishIoConnection);

  return kEipStatusOk;
}

EipStatus HandleReceivedConnectedData(EipUint8 *data,
                                      int data_length,
                                      struct sockaddr_in *from_address) {

  if ( ( CreateCommonPacketFormatStructure(data, data_length,
                                           &g_common_packet_format_data_item) )
       == kEipStatusError ) {
    return kEipStatusError;
  } else {
    /* check if connected address item or sequenced address item received, otherwise it is no connected message and should not be here */
    if ( (g_common_packet_format_data_item.address_item.type_id
          == kCipItemIdConnectionAddress)
         || (g_common_packet_format_data_item.address_item.type_id
             == kCipItemIdSequencedAddressItem) ) {                                   /* found connected address item or found sequenced address item -> for now the sequence number will be ignored */
      if (g_common_packet_format_data_item.data_item.type_id
          == kCipItemIdConnectedDataItem) {                               /* connected data item received */

        ConnectionObject *connection_object =
          GetConnectedObject(
            g_common_packet_format_data_item.address_item.data.
            connection_identifier);
        if (connection_object == NULL) {
          return kEipStatusError;
        }

        /* only handle the data if it is coming from the originator */
        if (connection_object->originator_address.sin_addr.s_addr
            == from_address->sin_addr.s_addr) {

          if ( SEQ_GT32(
                 g_common_packet_format_data_item.address_item.data.
                 sequence_number,
                 connection_object->eip_level_sequence_count_consuming) ) {
            /* reset the watchdog timer */
            connection_object->inactivity_watchdog_timer =
              (connection_object->o_to_t_requested_packet_interval
               / 1000)
                << (2
                    + connection_object->connection_timeout_multiplier);

            /* only inform assembly object if the sequence counter is greater or equal */
            connection_object->eip_level_sequence_count_consuming =
              g_common_packet_format_data_item.address_item.data.
              sequence_number;

            if (NULL
                != connection_object->connection_receive_data_function) {
              return connection_object->connection_receive_data_function(
                connection_object,
                g_common_packet_format_data_item.data_item.data,
                g_common_packet_format_data_item.data_item.length);
            }
          }
        } else {
          OPENER_TRACE_WARN(
            "Connected Message Data Received with wrong address information\n");
        }
      }
    }
  }
  return kEipStatusOk;
}

void ReadOutConnectionObjectFromMessage(
  CipMessageRouterRequest *const message_router_request,
  ConnectionObject *const connection_object);

void ReadOutConnectionObjectFromMessage(
  CipMessageRouterRequest *const message_router_request,
  ConnectionObject *const connection_object) {
  connection_object->priority_timetick = *message_router_request->data++;
  connection_object->timeout_ticks = *message_router_request->data++;
  /* O_to_T Conn ID */
  connection_object->cip_consumed_connection_id = GetDintFromMessage(
    &message_router_request->data);
  /* T_to_O Conn ID */
  connection_object->cip_produced_connection_id = GetDintFromMessage(
    &message_router_request->data);
  connection_object->connection_serial_number = GetIntFromMessage(
    &message_router_request->data);
  connection_object->originator_vendor_id = GetIntFromMessage(
    &message_router_request->data);
  connection_object->originator_serial_number = GetDintFromMessage(
    &message_router_request->data);

  /* keep it to none existent till the setup is done this eases error handling and
   * the state changes within the forward open request can not be detected from
   * the application or from outside (reason we are single threaded)*/
  connection_object->state = kConnectionStateNonExistent;
  connection_object->sequence_count_producing = 0;       /* set the sequence count to zero */

  connection_object->connection_timeout_multiplier =
    *message_router_request->data++;
  message_router_request->data += 3;       /* reserved */
  /* the requested packet interval parameter needs to be a multiple of TIMERTICK from the header file */
  OPENER_TRACE_INFO(
    "ForwardOpen: ConConnID %" PRIu32 ", ProdConnID %" PRIu32
    ", ConnSerNo %u\n",
    connection_object->cip_consumed_connection_id,
    connection_object->cip_produced_connection_id,
    connection_object->connection_serial_number);

  connection_object->o_to_t_requested_packet_interval = GetDintFromMessage(
    &message_router_request->data);

  connection_object->o_to_t_network_connection_parameter = GetIntFromMessage(
    &message_router_request->data);
  connection_object->t_to_o_requested_packet_interval = GetDintFromMessage(
    &message_router_request->data);

  EipUint32 temp = connection_object->t_to_o_requested_packet_interval
                   % (kOpenerTimerTickInMilliSeconds * 1000);
  if (temp > 0) {
    connection_object->t_to_o_requested_packet_interval =
      (EipUint32) ( connection_object->t_to_o_requested_packet_interval
                    / (kOpenerTimerTickInMilliSeconds * 1000) )
      * (kOpenerTimerTickInMilliSeconds * 1000)
      + (kOpenerTimerTickInMilliSeconds * 1000);
  }

  connection_object->t_to_o_network_connection_parameter = GetIntFromMessage(
    &message_router_request->data);

  connection_object->transport_type_class_trigger =
    *message_router_request->data++;
}

ForwardOpenConnectionType GetConnectionType(
  EipUint16 network_connection_parameter) {
  const EipUint16 kConnectionParameterMask = 0x6000;

  ForwardOpenConnectionType connection_type = network_connection_parameter
                                              & kConnectionParameterMask;

  OPENER_TRACE_INFO(
    "Connection type: 0x%x / network connection parameter: 0x%x\n",
    connection_type, network_connection_parameter);
  return connection_type;
}

/** @brief Function prototype for all Forward Open handle functions
 *
 */
typedef EipStatus (*HandleForwardOpenRequestFunction)(
  ConnectionObject *connection_object,CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

/** @brief Handles a Null Non Matching Forward Open Request
 *
 * Null, Non-Matching - Either ping device, or configure a device’s application,
 * or return  General Status kCipErrorConnectionFailure and
 * Extended Status kConnectionManagerExtendedStatusCodeNullForwardOpenNotSupported
 */
EipStatus HandleNullNonMatchingForwardOpenRequest(
  ConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

EipStatus HandleNullNonMatchingForwardOpenRequest(
  ConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response) {
  OPENER_TRACE_INFO("Right now we cannot handle Null requests\n");
  return AssembleForwardOpenResponse(
    connection_object,
    message_router_response,
    kCipErrorConnectionFailure,
    kConnectionManagerExtendedStatusCodeNullForwardOpenNotSupported);
}

/** @brief Handles a Null Matching Forward Open request
 *
 * Either  reconfigure a target device’s application, or
 * return General Status kCipErrorConnectionFailure and
 * Extended Status kConnectionManagerExtendedStatusCodeNullForwardOpenNotSupported
 */
EipStatus HandleNullMatchingForwardOpenRequest(
  ConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

EipStatus HandleNullMatchingForwardOpenRequest(
  ConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response) {
  OPENER_TRACE_INFO("Right now we cannot handle Null requests\n");
  return AssembleForwardOpenResponse(
    connection_object,
    message_router_response,
    kCipErrorConnectionFailure,
    kConnectionManagerExtendedStatusCodeNullForwardOpenNotSupported);
}

/** @brief Handles a Non Null Matching Forward Open Request
 *
 * Non-Null, Matching request - Return General Status = kCipErrorConnectionFailure,
 * Extended Status = kConnectionManagerExtendedStatusCodeErrorConnectionInUseOrDuplicateForwardOpen
 */
EipStatus HandleNonNullMatchingForwardOpenRequest(
  ConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

EipStatus HandleNonNullMatchingForwardOpenRequest(
  ConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response) {
  OPENER_TRACE_INFO("Right now we cannot handle reconfiguration requests\n");
  return AssembleForwardOpenResponse(
    connection_object,
    message_router_response,
    kCipErrorConnectionFailure,
    kConnectionManagerExtendedStatusCodeErrorConnectionInUseOrDuplicateForwardOpen);
}

/** @brief Handles a Non Null Non Matching Forward Open Request
 *
 * Non-Null, Non-Matching request - Establish a new connection
 */
EipStatus HandleNonNullNonMatchingForwardOpenRequest(
  ConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

EipStatus HandleNonNullNonMatchingForwardOpenRequest(
  ConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response) {

  EipUint16 connection_status = kConnectionManagerExtendedStatusCodeSuccess;

  /*check if the trigger type value is ok */
  if (0x40 & g_dummy_connection_object.transport_type_class_trigger) {
    return AssembleForwardOpenResponse(
      &g_dummy_connection_object,
      message_router_response,
      kCipErrorConnectionFailure,
      kConnectionManagerExtendedStatusCodeErrorTransportClassAndTriggerCombinationNotSupported);
  }

  EipUint32 temp = ParseConnectionPath(&g_dummy_connection_object,
                                       message_router_request,
                                       &connection_status);
  if (kEipStatusOk != temp) {
    return AssembleForwardOpenResponse(&g_dummy_connection_object,
                                       message_router_response,
                                       temp,
                                       connection_status);
  }

  /*parsing is now finished all data is available and check now establish the connection */
  ConnectionManagementHandling *connection_management_entry =
    GetConnectionManagementEntry(
      g_dummy_connection_object.connection_path.class_id);
  if (NULL != connection_management_entry) {
    temp = connection_management_entry->open_connection_function(
      &g_dummy_connection_object, &connection_status);
  } else {
    temp = kEipStatusError;
    connection_status =
      kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
  }

  if (kEipStatusOk != temp) {
    OPENER_TRACE_INFO("connection manager: connect failed\n");
    /* in case of error the dummy objects holds all necessary information */
    return AssembleForwardOpenResponse(&g_dummy_connection_object,
                                       message_router_response,
                                       temp,
                                       connection_status);
  } else {
    OPENER_TRACE_INFO("connection manager: connect succeeded\n");
    /* in case of success the g_pstActiveConnectionList points to the new connection */
    return AssembleForwardOpenResponse(g_active_connection_list,
                                       message_router_response,
                                       kCipErrorSuccess,
                                       0);
  }
}

/** @brief Array of Forward Open handle function pointers
 *
 * File scope variable
 * The first dimension handles if the request was a non-null request (0) or a null request (1),
 * the second dimension handles if the request was a non-matchin (0) or matching request (1)
 */
static const HandleForwardOpenRequestFunction
  handle_forward_open_request_functions[2][2] =
{ { HandleNonNullNonMatchingForwardOpenRequest,
    HandleNonNullMatchingForwardOpenRequest }, {
    HandleNullNonMatchingForwardOpenRequest,
    HandleNullMatchingForwardOpenRequest
  } };

/** @brief Check if resources for new connection available, generate ForwardOpen Reply message.
 *
 *  Forward Open four cases
 *  Non-Null/Not matching - open a connection
 *  Non-Null/Matching - error
 *  Null/Not matching - ping a device/configure
 *  Null/Matching - reconfigure
 *
 *  Null connection - both O->T and T->O connection parameter field are null
 *  Non-Null connection - one or both O->T and T->O connection parameter field are not null
 *  Matching - Connection Triad matches an existing connection
 *  (Connection Serial Number, Originator Vendor ID and Originator Serial Number)
 *
 *  @param instance	pointer to CIP object instance
 *  @param message_router_request		pointer to Message Router Request.
 *  @param message_router_response		pointer to Message Router Response.
 *      @return >0 .. success, 0 .. no reply to send back
 *              -1 .. error
 */
EipStatus ForwardOpen(CipInstance *instance,
                      CipMessageRouterRequest *message_router_request,
                      CipMessageRouterResponse *message_router_response,
                      struct sockaddr *originator_address) {
  (void) instance;       /*suppress compiler warning */

  uint8_t is_null_request = -1;       /* 1 = Null Request, 0 =  Non-Null Request  */
  uint8_t is_matching_request = -1;       /* 1 = Matching Request, 0 = Non-Matching Request  */

  /*first check if we have already a connection with the given params */
  ReadOutConnectionObjectFromMessage(message_router_request,
                                     &g_dummy_connection_object);

  memcpy( &(g_dummy_connection_object.originator_address), originator_address,
          sizeof(g_dummy_connection_object.original_opener_ip_address) );

  ForwardOpenConnectionType o_to_t_connection_type = GetConnectionType(
    g_dummy_connection_object.o_to_t_network_connection_parameter);
  ForwardOpenConnectionType t_to_o_connection_type = GetConnectionType(
    g_dummy_connection_object.t_to_o_network_connection_parameter);

  /* Check if both connection types are valid, otherwise send error response */
  if (kForwardOpenConnectionTypeReserved == o_to_t_connection_type) {
    return AssembleForwardOpenResponse(
      &g_dummy_connection_object,
      message_router_response,
      kCipErrorConnectionFailure,
      kConnectionManagerExtendedStatusCodeErrorInvalidOToTConnectionType);
  }

  if (kForwardOpenConnectionTypeReserved == t_to_o_connection_type) {
    return AssembleForwardOpenResponse(
      &g_dummy_connection_object,
      message_router_response,
      kCipErrorConnectionFailure,
      kConnectionManagerExtendedStatusCodeErrorInvalidTToOConnectionType);
  }

  /* Check if request is a Null request or a Non-Null request */
  if (kForwardOpenConnectionTypeNull == o_to_t_connection_type
      && kForwardOpenConnectionTypeNull == t_to_o_connection_type) {
    is_null_request = 1;
    OPENER_TRACE_INFO("We have a Null request\n");
  } else {
    is_null_request = 0;
    OPENER_TRACE_INFO("We have a Non-Null request\n");
  }

  /* Check if we have a matching or non matching request */
  if ( ( NULL != CheckForExistingConnection(&g_dummy_connection_object) ) ) {
    OPENER_TRACE_INFO("We have a Matching request\n");
    is_matching_request = 1;

  } else {
    OPENER_TRACE_INFO("We have a Non-Matching request\n");
    is_matching_request = 0;
  }

  HandleForwardOpenRequestFunction choosen_function =
    handle_forward_open_request_functions[is_null_request][is_matching_request];

  return choosen_function(&g_dummy_connection_object, instance,
                          message_router_request, message_router_response);
}

void GeneralConnectionConfiguration(ConnectionObject *connection_object) {
  if ( kForwardOpenConnectionTypePointToPointConnection
       == (connection_object->o_to_t_network_connection_parameter
           & kForwardOpenConnectionTypePointToPointConnection) ) {
    /* if we have a point to point connection for the O to T direction
     * the target shall choose the connection ID.
     */
    connection_object->cip_consumed_connection_id = GetConnectionId();
  }

  if ( kForwardOpenConnectionTypeMulticastConnection
       == (connection_object->t_to_o_network_connection_parameter
           & kForwardOpenConnectionTypeMulticastConnection) ) {
    /* if we have a multi-cast connection for the T to O direction the
     * target shall choose the connection ID.
     */
    connection_object->cip_produced_connection_id = GetConnectionId();
  }

  connection_object->eip_level_sequence_count_producing = 0;
  connection_object->sequence_count_producing = 0;
  connection_object->eip_level_sequence_count_consuming = 0;
  connection_object->sequence_count_consuming = 0;

  connection_object->watchdog_timeout_action =
    kWatchdogTimeoutActionAutoDelete;                     /* the default for all connections on EIP*/

  connection_object->expected_packet_rate = 0;       /* default value */

  if ( (connection_object->transport_type_class_trigger & 0x80) == 0x00 ) {     /* Client Type Connection requested */
    connection_object->expected_packet_rate =
      (EipUint16) ( (connection_object->t_to_o_requested_packet_interval)
                    / 1000 );
    /* As soon as we are ready we should produce the connection. With the 0 here we will produce with the next timer tick
     * which should be sufficient. */
    connection_object->transmission_trigger_timer = 0;
  } else {
    /* Server Type Connection requested */
    connection_object->expected_packet_rate =
      (EipUint16) ( (connection_object->o_to_t_requested_packet_interval)
                    / 1000 );
  }

  connection_object->production_inhibit_timer =
    connection_object->production_inhibit_time = 0;

  /*setup the preconsuption timer: max(ConnectionTimeoutMultiplier * ExpectedPacketRate, 10s) */
  connection_object->inactivity_watchdog_timer =
    ( ( ( (connection_object->o_to_t_requested_packet_interval) / 1000 )
        << (2 + connection_object->connection_timeout_multiplier) )
      > 10000 ) ?
    ( ( (connection_object->o_to_t_requested_packet_interval)
        / 1000 )
      << (2
          + connection_object->connection_timeout_multiplier) ) :
    10000;

  connection_object->consumed_connection_size =
    connection_object->o_to_t_network_connection_parameter & 0x01FF;

  connection_object->produced_connection_size =
    connection_object->t_to_o_network_connection_parameter & 0x01FF;

}

EipStatus ForwardClose(CipInstance *instance,
                       CipMessageRouterRequest *message_router_request,
                       CipMessageRouterResponse *message_router_response,
                       struct sockaddr *originator_address) {
  /*Suppress compiler warning*/
  (void) instance;

  /* check connection_serial_number && originator_vendor_id && originator_serial_number if connection is established */
  ConnectionManagerExtendedStatusCode connection_status =
    kConnectionManagerExtendedStatusCodeErrorConnectionTargetConnectionNotFound;
  ConnectionObject *connection_object = g_active_connection_list;

  /* set AddressInfo Items to invalid TypeID to prevent assembleLinearMsg to read them */
  g_common_packet_format_data_item.address_info_item[0].type_id = 0;
  g_common_packet_format_data_item.address_info_item[1].type_id = 0;

  message_router_request->data += 2;       /* ignore Priority/Time_tick and Time-out_ticks */

  EipUint16 connection_serial_number = GetIntFromMessage(
    &message_router_request->data);
  EipUint16 originator_vendor_id = GetIntFromMessage(
    &message_router_request->data);
  EipUint32 originator_serial_number = GetDintFromMessage(
    &message_router_request->data);

  OPENER_TRACE_INFO("ForwardClose: ConnSerNo %d\n", connection_serial_number);

  while (NULL != connection_object) {
    /* this check should not be necessary as only established connections should be in the active connection list */
    if ( (connection_object->state == kConnectionStateEstablished)
         || (connection_object->state == kConnectionStateTimedOut) ) {
      if ( (connection_object->connection_serial_number
            == connection_serial_number)
           && (connection_object->originator_vendor_id
               == originator_vendor_id)
           && (connection_object->originator_serial_number
               == originator_serial_number) ) {
        /* found the corresponding connection object -> close it */
        OPENER_ASSERT(
          NULL != connection_object->connection_close_function);
        if( ( (struct sockaddr_in *)originator_address )->sin_addr.s_addr ==
            connection_object->originator_address.sin_addr.s_addr ) {
          connection_object->connection_close_function(connection_object);
          connection_status = kConnectionManagerExtendedStatusCodeSuccess;
        } else {
          connection_status = kConnectionManagerExtendedStatusWrongCloser;
        }
        break;
      }
    }
    connection_object = connection_object->next_connection_object;
  }

  return AssembleForwardCloseResponse(connection_serial_number,
                                      originator_vendor_id,
                                      originator_serial_number,
                                      message_router_request,
                                      message_router_response,
                                      connection_status);
}

/* TODO: Not implemented */
EipStatus GetConnectionOwner(CipInstance *instance,
                             CipMessageRouterRequest *message_router_request,
                             CipMessageRouterResponse *message_router_response,
                             struct sockaddr *originator_address) {
  /* suppress compiler warnings */
  (void) instance;
  (void) message_router_request;
  (void) message_router_response;

  return kEipStatusOk;
}

EipStatus ManageConnections(MilliSeconds elapsed_time) {
  //OPENER_TRACE_INFO("Entering ManageConnections\n");
  /*Inform application that it can execute */
  HandleApplication();
  ManageEncapsulationMessages(elapsed_time);

  ConnectionObject *connection_object = g_active_connection_list;
  while (NULL != connection_object) {
    //OPENER_TRACE_INFO("Entering Connection Object loop\n");
    if (kConnectionStateEstablished == connection_object->state) {
      if ( (0 != connection_object->consuming_instance) ||                  /* we have a consuming connection check inactivity watchdog timer */
           (connection_object->transport_type_class_trigger & 0x80) )             /* all sever connections have to maintain an inactivity watchdog timer */
      {
        if (elapsed_time >= connection_object->inactivity_watchdog_timer) {
          /* we have a timed out connection perform watchdog time out action*/
          OPENER_TRACE_INFO(">>>>>>>>>>Connection timed out\n");
          OPENER_ASSERT(
            NULL != connection_object->connection_timeout_function);
          connection_object->connection_timeout_function(
            connection_object);
        } else {
          connection_object->inactivity_watchdog_timer -= elapsed_time;
        }
      }
      /* only if the connection has not timed out check if data is to be send */
      if (kConnectionStateEstablished == connection_object->state) {
        /* client connection */
        if ( (connection_object->expected_packet_rate != 0)
             && (kEipInvalidSocket
                 != connection_object->socket[
                   kUdpCommuncationDirectionProducing]) )                                                                          /* only produce for the master connection */
        {
          if ( kConnectionTriggerTypeCyclicConnection
               != (connection_object->transport_type_class_trigger
                   & kConnectionTriggerTypeProductionTriggerMask) ) {
            /* non cyclic connections have to decrement production inhibit timer */
            if (0 <= connection_object->production_inhibit_timer) {
              connection_object->production_inhibit_timer -=
                elapsed_time;
            }
          }
          connection_object->transmission_trigger_timer -=
            elapsed_time;
          if (connection_object->transmission_trigger_timer <= 0) {                               /* need to send package */
            OPENER_ASSERT(
              NULL != connection_object->connection_send_data_function);
            EipStatus eip_status =
              connection_object->connection_send_data_function(
                connection_object);
            if (eip_status == kEipStatusError) {
              OPENER_TRACE_ERR(
                "sending of UDP data in manage Connection failed\n");
            }
            /* reload the timer value */
            connection_object->transmission_trigger_timer =
              connection_object->expected_packet_rate;
            if ( kConnectionTriggerTypeCyclicConnection
                 != (connection_object->transport_type_class_trigger
                     & kConnectionTriggerTypeProductionTriggerMask) ) {
              /* non cyclic connections have to reload the production inhibit timer */
              connection_object->production_inhibit_timer =
                connection_object->production_inhibit_time;
            }
          }
        }
      }
    }
    connection_object = connection_object->next_connection_object;
  }
  return kEipStatusOk;
}

/** @brief Assembles the Forward Open Response
 *
 * @param connection_object pointer to connection Object
 * @param message_router_response pointer to message router response
 * @param general_status the general status of the response
 * @param extended_status extended status in the case of an error otherwise 0
 * @return status
 *   kEipStatusOk .. no reply need to be sent back
 *   kEipStatusOkSend .. need to send reply
 *   kEipStatusError .. error
 */
EipStatus AssembleForwardOpenResponse(
  ConnectionObject *connection_object,CipMessageRouterResponse *
  message_router_response,EipUint8 general_status,EipUint16 extended_status) {
  /* write reply information in CPF struct dependent of pa_status */
  CipCommonPacketFormatData *cip_common_packet_format_data =
    &g_common_packet_format_data_item;
  EipByte *message = message_router_response->data;
  cip_common_packet_format_data->item_count = 2;
  cip_common_packet_format_data->data_item.type_id =
    kCipItemIdUnconnectedDataItem;

  AddNullAddressItem(cip_common_packet_format_data);

  message_router_response->reply_service = (0x80 | kForwardOpen);
  message_router_response->general_status = general_status;

  if (kCipErrorSuccess == general_status) {
    OPENER_TRACE_INFO(
      "assembleFWDOpenResponse: sending success response\n");
    message_router_response->data_length = 26;             /* if there is no application specific data */
    message_router_response->size_of_additional_status = 0;

    if (cip_common_packet_format_data->address_info_item[0].type_id != 0) {
      cip_common_packet_format_data->item_count = 3;
      if (cip_common_packet_format_data->address_info_item[1].type_id
          != 0) {
        cip_common_packet_format_data->item_count = 4;                         /* there are two sockaddrinfo items to add */
      }
    }

    AddDintToMessage(connection_object->cip_consumed_connection_id,
                     &message);
    AddDintToMessage(connection_object->cip_produced_connection_id,
                     &message);
  } else {
    /* we have an connection creation error */
    OPENER_TRACE_INFO("AssembleForwardOpenResponse: sending error response\n");
    connection_object->state = kConnectionStateNonExistent;
    message_router_response->data_length = 10;

    switch (general_status) {
      case kCipErrorNotEnoughData:
      case kCipErrorTooMuchData: {
        message_router_response->size_of_additional_status = 0;
        break;
      }

      default: {
        switch (extended_status) {
          case
            kConnectionManagerExtendedStatusCodeErrorInvalidOToTConnectionSize:
          {
            message_router_response->size_of_additional_status = 2;
            message_router_response->additional_status[0] = extended_status;
            message_router_response->additional_status[1] =
              connection_object->correct_originator_to_target_size;
            break;
          }

          case
            kConnectionManagerExtendedStatusCodeErrorInvalidTToOConnectionSize:
          {
            message_router_response->size_of_additional_status = 2;
            message_router_response->additional_status[0] = extended_status;
            message_router_response->additional_status[1] =
              connection_object->correct_target_to_originator_size;
            break;
          }

          default: {
            message_router_response->size_of_additional_status = 1;
            message_router_response->additional_status[0] = extended_status;
            break;
          }
        }
        break;
      }
    }
  }

  AddIntToMessage(connection_object->connection_serial_number, &message);
  AddIntToMessage(connection_object->originator_vendor_id, &message);
  AddDintToMessage(connection_object->originator_serial_number, &message);

  if (kCipErrorSuccess == general_status) {
    /* set the actual packet rate to requested packet rate */
    AddDintToMessage(connection_object->o_to_t_requested_packet_interval,
                     &message);
    AddDintToMessage(connection_object->t_to_o_requested_packet_interval,
                     &message);
  }

  *message = 0;       /* remaining path size - for routing devices relevant */
  message++;
  *message = 0;       /* reserved */
  message++;

  return kEipStatusOkSend;       /* send reply */
}

/**
 * @brief Adds a Null Address Item to the common data packet format data
 * @param common_data_packet_format_data The CPF data packet where the Null Address Item shall be added
 */
void AddNullAddressItem(
  CipCommonPacketFormatData *common_data_packet_format_data) {
  /* Precondition: Null Address Item only valid in unconnected messages */
  assert(
    common_data_packet_format_data->data_item.type_id
    == kCipItemIdUnconnectedDataItem);

  common_data_packet_format_data->address_item.type_id =
    kCipItemIdNullAddress;
  common_data_packet_format_data->address_item.length = 0;
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
 *                      0 .. no reply need to ne sent back
 *                      1 .. need to send reply
 *                     -1 .. error
 */
EipStatus AssembleForwardCloseResponse(
  EipUint16 connection_serial_number,
  EipUint16 originatior_vendor_id,
  EipUint32 originator_serial_number,
  CipMessageRouterRequest *
  message_router_request,
  CipMessageRouterResponse *
  message_router_response,
  EipUint16 extended_error_code) {
  /* write reply information in CPF struct dependent of pa_status */
  CipCommonPacketFormatData *common_data_packet_format_data =
    &g_common_packet_format_data_item;
  EipByte *message = message_router_response->data;
  common_data_packet_format_data->item_count = 2;
  common_data_packet_format_data->data_item.type_id =
    kCipItemIdUnconnectedDataItem;

  AddNullAddressItem(common_data_packet_format_data);

  AddIntToMessage(connection_serial_number, &message);
  AddIntToMessage(originatior_vendor_id, &message);
  AddDintToMessage(originator_serial_number, &message);

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->data_length = 10;       /* if there is no application specific data */

  if (kConnectionManagerExtendedStatusCodeSuccess == extended_error_code) {
    *message = 0;             /* no application data */
    message_router_response->general_status = kCipErrorSuccess;
    message_router_response->size_of_additional_status = 0;
  } else {
    *message = *message_router_request->data;             /* remaining path size */
    if (kConnectionManagerExtendedStatusWrongCloser == extended_error_code) {
      message_router_response->general_status = kCipErrorPrivilegeViolation;
    } else {
      message_router_response->general_status = kCipErrorConnectionFailure;
      message_router_response->additional_status[0] = extended_error_code;
      message_router_response->size_of_additional_status = 1;
    }
  }

  message++;
  *message = 0;       /* reserved */
  message++;

  return kEipStatusOkSend;
}

ConnectionObject *GetConnectedObject(EipUint32 connection_id) {
  ConnectionObject *active_connection_object_list_item =
    g_active_connection_list;
  while (NULL != active_connection_object_list_item) {
    if (active_connection_object_list_item->state
        == kConnectionStateEstablished) {
      if (active_connection_object_list_item->cip_consumed_connection_id
          == connection_id) {
        return active_connection_object_list_item;
      }
    }
    active_connection_object_list_item =
      active_connection_object_list_item->next_connection_object;
  }
  return NULL;
}

ConnectionObject *GetConnectedOutputAssembly(EipUint32 output_assembly_id) {
  ConnectionObject *active_connection_object_list_item =
    g_active_connection_list;

  while (NULL != active_connection_object_list_item) {
    if (active_connection_object_list_item->state
        == kConnectionStateEstablished) {
      if (active_connection_object_list_item->connection_path.connection_point[
            0]
          == output_assembly_id) {
        return active_connection_object_list_item;
      }
    }
    active_connection_object_list_item =
      active_connection_object_list_item->next_connection_object;
  }
  return NULL;
}

ConnectionObject *CheckForExistingConnection(
  ConnectionObject *connection_object) {
  ConnectionObject *active_connection_object_list_item =
    g_active_connection_list;

  while (NULL != active_connection_object_list_item) {
    if (active_connection_object_list_item->state
        == kConnectionStateEstablished) {
      if ( (connection_object->connection_serial_number
            == active_connection_object_list_item->connection_serial_number)
           && (connection_object->originator_vendor_id
               == active_connection_object_list_item->originator_vendor_id)
           && (connection_object->originator_serial_number
               == active_connection_object_list_item->originator_serial_number) )
      {
        return active_connection_object_list_item;
      }
    }
    active_connection_object_list_item =
      active_connection_object_list_item->next_connection_object;
  }
  return NULL;
}

EipStatus CheckElectronicKeyData(EipUint8 key_format,
                                 CipKeyData *key_data,
                                 EipUint16 *extended_status) {
  EipByte compatiblity_mode = key_data->major_revision & 0x80;

  /* Remove compatibility bit */
  key_data->major_revision &= 0x7F;

  /* Default return value */
  *extended_status = kConnectionManagerExtendedStatusCodeSuccess;

  /* Check key format */
  if (4 != key_format) {
    *extended_status =
      kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
    return kEipStatusError;
  }

  /* Check VendorID and ProductCode, must match, or 0 */
  if ( ( (key_data->vendor_id != vendor_id_) && (key_data->vendor_id != 0) )
       || ( (key_data->product_code != product_code_)
            && (key_data->product_code != 0) ) ) {
    *extended_status =
      kConnectionManagerExtendedStatusCodeErrorVendorIdOrProductcodeError;
    return kEipStatusError;
  } else {
    /* VendorID and ProductCode are correct */

    /* Check DeviceType, must match or 0 */
    if ( (key_data->device_type != device_type_)
         && (key_data->device_type != 0) ) {
      *extended_status =
        kConnectionManagerExtendedStatusCodeErrorDeviceTypeError;
      return kEipStatusError;
    } else {
      /* VendorID, ProductCode and DeviceType are correct */

      if (!compatiblity_mode) {
        /* Major = 0 is valid */
        if (0 == key_data->major_revision) {
          return (kEipStatusOk);
        }

        /* Check Major / Minor Revision, Major must match, Minor match or 0 */
        if ( (key_data->major_revision != revision_.major_revision)
             || ( (key_data->minor_revision
                   != revision_.minor_revision)
                  && (key_data->minor_revision != 0) ) ) {
          *extended_status =
            kConnectionManagerExtendedStatusCodeErrorRevisionMismatch;
          return kEipStatusError;
        }
      } else {
        /* Compatibility mode is set */

        /* Major must match, Minor != 0 and <= MinorRevision */
        if ( (key_data->major_revision == revision_.major_revision)
             && (key_data->minor_revision > 0)
             && (key_data->minor_revision <= revision_.minor_revision) ) {
          return (kEipStatusOk);
        } else {
          *extended_status =
            kConnectionManagerExtendedStatusCodeErrorRevisionMismatch;
          return kEipStatusError;
        }
      }                   /* end if CompatiblityMode handling */
    }
  }

  return (*extended_status == kConnectionManagerExtendedStatusCodeSuccess) ?
         kEipStatusOk : kEipStatusError;
}

EipUint8 ParseConnectionPath(ConnectionObject *connection_object,
                             CipMessageRouterRequest *message_router_request,
                             EipUint16 *extended_error) {
  const EipUint8 *message = message_router_request->data;
  unsigned int remaining_path_size = connection_object->connection_path_size =
                                       *message++; /* length in words */
  CipClass *class = NULL;

  int originator_to_target_connection_type =
    kForwardOpenConnectionTypeReserved;                     /* Set to invalid value */
  int target_to_originator_connection_type =
    kForwardOpenConnectionTypeReserved;                     /* Set to invalid value */

  /* with 256 we mark that we haven't got a PIT segment */
  connection_object->production_inhibit_time = 256;

  if ( (g_kForwardOpenHeaderLength + remaining_path_size * 2)
       < message_router_request->request_path_size ) {
    /* the received packet is larger than the data in the path */
    *extended_error = 0;
    return kCipErrorTooMuchData;
  }

  if ( (g_kForwardOpenHeaderLength + remaining_path_size * 2)
       > message_router_request->request_path_size ) {
    /*there is not enough data in received packet */
    *extended_error = 0;
    return kCipErrorNotEnoughData;
  }

  if (remaining_path_size > 0) {
    /* first look if there is an electronic key */
    if ( kSegmentTypeLogicalSegment == GetPathSegmentType(message) ) {
      if ( kLogicalSegmentLogicalTypeSpecial
           == GetPathLogicalSegmentLogicalType(message) ) {
        if ( kLogicalSegmentSpecialTypeLogicalFormatElectronicKey
             == GetPathLogicalSegmentSpecialTypeLogicalType(
               message) ) {
          if ( kElectronicKeySegmentFormatKeyFormat4
               == GetPathLogicalSegmentElectronicKeyFormat(
                 message) ) {
            /* Check if there is enough data for holding the electronic key segment */
            if (remaining_path_size < 5) {
              *extended_error = 0;
              return kCipErrorNotEnoughData;
            }
            /* Electronic key format 4 found */
            ElectronicKeyFormat4 *electronic_key =
              ElectronicKeyFormat4New();
            GetPathLogicalSegmentElectronicKeyFormat4(message,
                                                      electronic_key);
            /* logical electronic key found */
            connection_object->electronic_key.segment_type = 0x34;
            connection_object->electronic_key.key_format = 0x04;                                     //ELECTRONIC_KEY_SEGMENT_KEY_FORMAT_4;
            connection_object->electronic_key.key_data.vendor_id =
              ElectronicKeyFormat4GetVendorId(electronic_key);
            connection_object->electronic_key.key_data.device_type =
              ElectronicKeyFormat4GetDeviceType(
                electronic_key);
            connection_object->electronic_key.key_data.product_code =
              ElectronicKeyFormat4GetProductCode(
                electronic_key);
            connection_object->electronic_key.key_data.major_revision =
              ElectronicKeyFormat4GetMajorRevision(
                electronic_key);
            if ( true
                 == ElectronicKeyFormat4GetMajorRevisionCompatibility(
                   electronic_key) ) {
              connection_object->electronic_key.key_data.major_revision |=
                0x80;
            }
            connection_object->electronic_key.key_data.minor_revision =
              ElectronicKeyFormat4GetMinorRevision(
                electronic_key);
            ElectronicKeyFormat4Delete(&electronic_key);
            message += 10;
            remaining_path_size -= 5;                                     /*length of the electronic key*/
            OPENER_TRACE_INFO(
              "key: ven ID %d, dev type %d, prod code %d, major %d, minor %d\n",
              connection_object->electronic_key.key_data.vendor_id,
              connection_object->electronic_key.key_data.device_type,
              connection_object->electronic_key.key_data.product_code,
              connection_object->electronic_key.key_data.major_revision,
              connection_object->electronic_key.key_data.minor_revision);
            if ( kEipStatusOk
                 != CheckElectronicKeyData(
                   connection_object->electronic_key.key_format,
                   &(connection_object->electronic_key.key_data),
                   extended_error) ) {
              return kCipErrorConnectionFailure;
            }
          }

        } else {
          OPENER_TRACE_INFO("no key\n");
        }
      }
    }

    if ( kConnectionTriggerTypeCyclicConnection
         != (connection_object->transport_type_class_trigger
             & kConnectionTriggerTypeProductionTriggerMask) ) {
      /*non cyclic connections may have a production inhibit */
      if ( kSegmentTypeNetworkSegment == GetPathSegmentType(message) ) {
        NetworkSegmentSubtype network_segment_subtype =
          GetPathNetworkSegmentSubtype(message);
        if (kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds
            == network_segment_subtype) {
          connection_object->production_inhibit_time = message[1];
          message += 2;
          remaining_path_size -= 1;
        }
      }
    }

    if ( EQLOGICALPATH(*message, 0x20) ) {           /* classID */
      connection_object->connection_path.class_id = GetPaddedLogicalPath(
        &message);
      class = GetCipClass(connection_object->connection_path.class_id);
      if (0 == class) {
        OPENER_TRACE_ERR("classid %" PRIx32 " not found\n",
                         connection_object->connection_path.class_id);
        if (connection_object->connection_path.class_id >= 0xC8) {                         /*reserved range of class ids */
          *extended_error =
            kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
        } else {
          *extended_error =
            kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
        }
        return kCipErrorConnectionFailure;
      }

      OPENER_TRACE_INFO("classid %" PRIx32 " (%s)\n",
                        connection_object->connection_path.class_id,
                        class->class_name);
    } else {
      *extended_error =
        kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
      return kCipErrorConnectionFailure;
    }
    remaining_path_size -= 1;             /* 1 16Bit word for the class part of the path */

    if ( EQLOGICALPATH(*message, 0x24) ) {           /* store the configuration ID for later checking in the application connection types */
      connection_object->connection_path.connection_point[2] =
        GetPaddedLogicalPath(&message);
      OPENER_TRACE_INFO("Configuration instance id %" PRId32 "\n",
                        connection_object->connection_path.connection_point[2]);
      if ( NULL
           == GetCipInstance(class,
                             connection_object->connection_path.
                             connection_point[2]) ) {
        /*according to the test tool we should respond with this extended error code */
        *extended_error =
          kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
        return kCipErrorConnectionFailure;
      }
      /* 1 or 2 16Bit words for the configuration instance part of the path  */
      remaining_path_size -=
        (connection_object->connection_path.connection_point[2]
         > 0xFF) ? 2 : 1;
    } else {
      OPENER_TRACE_INFO("no config data\n");
    }

    if ( 0x03 == (connection_object->transport_type_class_trigger & 0x03) ) {
      /*we have Class 3 connection*/
      if (remaining_path_size > 0) {
        OPENER_TRACE_WARN(
          "Too much data in connection path for class 3 connection\n");
        *extended_error =
          kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
        return kCipErrorConnectionFailure;
      }

      /* connection end point has to be the message router instance 1 */
      if ( (connection_object->connection_path.class_id
            != kCipMessageRouterClassCode)
           || (connection_object->connection_path.connection_point[2]
               != 1) ) {
        *extended_error =
          kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
        return kCipErrorConnectionFailure;
      }
      connection_object->connection_path.connection_point[0] =
        connection_object->connection_path.connection_point[2];
    } else {             /* we have an IO connection */
      originator_to_target_connection_type = GetConnectionType(
        connection_object->o_to_t_network_connection_parameter);
      target_to_originator_connection_type = GetConnectionType(
        connection_object->t_to_o_network_connection_parameter);

      connection_object->connection_path.connection_point[1] = 0;                   /* set not available path to Invalid */

      int number_of_encoded_paths = 0;
      if (originator_to_target_connection_type == 0) {
        if (target_to_originator_connection_type == 0) {                         /* configuration only connection */
          number_of_encoded_paths = 0;
          OPENER_TRACE_WARN("assembly: type invalid\n");
        } else {                         /* 1 path -> path is for production */
          OPENER_TRACE_INFO("assembly: type produce\n");
          number_of_encoded_paths = 1;
        }
      } else {
        if (target_to_originator_connection_type == 0) {                         /* 1 path -> path is for consumption */
          OPENER_TRACE_INFO("assembly: type consume\n");
          number_of_encoded_paths = 1;
        } else {                         /* 2 paths -> 1st for production 2nd for consumption */
          OPENER_TRACE_INFO("assembly: type bidirectional\n");
          number_of_encoded_paths = 2;
        }
      }

      for (int i = 0; i < number_of_encoded_paths; i++)                   /* process up to 2 encoded paths */
      {
        if ( EQLOGICALPATH(*message,
                           0x24) || EQLOGICALPATH(*message, 0x2C) )                     /* Connection Point interpreted as InstanceNr -> only in Assembly Objects */
        {                         /* InstanceNR */
          connection_object->connection_path.connection_point[i] =
            GetPaddedLogicalPath(&message);
          OPENER_TRACE_INFO(
            "connection point %" PRIu32 "\n",
            connection_object->connection_path.connection_point
            [i]);
          if ( 0
               == GetCipInstance(class,
                                 connection_object->connection_path.
                                 connection_point[i]) ) {
            *extended_error =
              kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
            return kCipErrorConnectionFailure;
          }
          /* 1 or 2 16Bit word for the connection point part of the path */
          remaining_path_size -=
            (connection_object->connection_path.connection_point[i]
             > 0xFF) ? 2 : 1;
        } else {
          *extended_error =
            kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
          return kCipErrorConnectionFailure;
        }
      }

      g_config_data_length = 0;
      g_config_data_buffer = NULL;

      while (remaining_path_size > 0) {                   /* have something left in the path should be configuration data */

        SegmentType segment_type = GetPathSegmentType(message);
        switch (segment_type) {
          case kSegmentTypeDataSegment: {
            DataSegmentSubtype data_segment_type =
              GetPathDataSegmentSubtype(message);
            switch (data_segment_type) {
              case kDataSegmentSubtypeSimpleData:
                g_config_data_length = message[1] * 2;                                 /*data segments store length 16-bit word wise */
                g_config_data_buffer = (EipUint8 *) message + 2;
                remaining_path_size -= (g_config_data_length + 2);
                message += (g_config_data_length + 2);
                break;
              default:
                OPENER_TRACE_ERR("Not allowed in connection manager");
                break;
            }
          }
          break;
          case kSegmentTypeNetworkSegment: {
            NetworkSegmentSubtype subtype =
              GetPathNetworkSegmentSubtype(message);
            switch (subtype) {
              case kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds:
                if ( kConnectionTriggerTypeCyclicConnection
                     != (connection_object->transport_type_class_trigger
                         & kConnectionTriggerTypeProductionTriggerMask) ) {
                  /* only non cyclic connections may have a production inhibit */
                  connection_object->production_inhibit_time =
                    message[1];
                  message += 2;
                  remaining_path_size -= 2;
                } else {
                  *extended_error =
                    connection_object->connection_path_size
                    - remaining_path_size;                                                                     /*offset in 16Bit words where within the connection path the error happend*/
                  return kCipErrorPathSegmentError;                                       /*status code for invalid segment type*/
                }
              default:
                OPENER_TRACE_ERR("Not allowed in connection manager");
                break;
            }
          }
          break;

          default:
            OPENER_TRACE_WARN(
              "No data segment identifier found for the configuration data\n");
            *extended_error = connection_object->connection_path_size
                              - remaining_path_size;                           /*offset in 16Bit words where within the connection path the error happend*/
            return 0x04; /* kConnectionManagerGeneralStatusPathSegmentErrorInUnconnectedSend */
            break;
        }
      }
    }
  }

  /*save back the current position in the stream allowing followers to parse anything thats still there*/
  message_router_request->data = message;
  return kEipStatusOk;
}

void CloseConnection(ConnectionObject *RESTRICT connection_object) {
  connection_object->state = kConnectionStateNonExistent;
  if ( 0x03 != (connection_object->transport_type_class_trigger & 0x03) ) {
    /* only close the UDP connection for not class 3 connections */
    IApp_CloseSocket_udp(
      connection_object->socket[kUdpCommuncationDirectionConsuming]);
    connection_object->socket[kUdpCommuncationDirectionConsuming] =
      kEipInvalidSocket;
    IApp_CloseSocket_udp(
      connection_object->socket[kUdpCommuncationDirectionProducing]);
    connection_object->socket[kUdpCommuncationDirectionProducing] =
      kEipInvalidSocket;
  }
  RemoveFromActiveConnections(connection_object);
}

void CopyConnectionData(ConnectionObject *RESTRICT destination,
                        const ConnectionObject *RESTRICT const source) {
  memcpy( destination, source, sizeof(ConnectionObject) );
}

void AddNewActiveConnection(ConnectionObject *connection_object) {
  connection_object->first_connection_object = NULL;
  connection_object->next_connection_object = g_active_connection_list;
  if (NULL != g_active_connection_list) {
    g_active_connection_list->first_connection_object = connection_object;
  }
  g_active_connection_list = connection_object;
  g_active_connection_list->state = kConnectionStateEstablished;
}

void RemoveFromActiveConnections(ConnectionObject *connection_object) {
  if (NULL != connection_object->first_connection_object) {
    connection_object->first_connection_object->next_connection_object =
      connection_object->next_connection_object;
  } else {
    g_active_connection_list = connection_object->next_connection_object;
  }
  if (NULL != connection_object->next_connection_object) {
    connection_object->next_connection_object->first_connection_object =
      connection_object->first_connection_object;
  }
  connection_object->first_connection_object = NULL;
  connection_object->next_connection_object = NULL;
  connection_object->state = kConnectionStateNonExistent;
}

EipBool8 IsConnectedOutputAssembly(const EipUint32 instance_number) {
  EipBool8 is_connected = false;

  ConnectionObject *iterator = g_active_connection_list;

  while (NULL != iterator) {
    if (instance_number == iterator->connection_path.connection_point[0]) {
      is_connected = true;
      break;
    }
    iterator = iterator->next_connection_object;
  }
  return is_connected;
}

EipStatus AddConnectableObject(EipUint32 class_id,
                               OpenConnectionFunction open_connection_function)
{
  EipStatus status = kEipStatusError;

  /*parsing is now finished all data is available and check now establish the connection */
  for (int i = 0; i < g_kNumberOfConnectableObjects; ++i) {
    if ( (0 == g_connection_management_list[i].class_id)
         || (class_id == g_connection_management_list[i].class_id) ) {
      g_connection_management_list[i].class_id = class_id;
      g_connection_management_list[i].open_connection_function =
        open_connection_function;
      status = kEipStatusOk;
      break;
    }
  }

  return status;
}

ConnectionManagementHandling *
GetConnectionManagementEntry(EipUint32 class_id) {

  ConnectionManagementHandling *connection_management_entry = NULL;

  for (int i = 0; i < g_kNumberOfConnectableObjects; ++i) {
    if (class_id == g_connection_management_list[i].class_id) {
      connection_management_entry = &(g_connection_management_list[i]);
      break;
    }
  }
  return connection_management_entry;
}

EipStatus TriggerConnections(unsigned int output_assembly,
                             unsigned int input_assembly) {
  EipStatus status = kEipStatusError;

  ConnectionObject *pstRunner = g_active_connection_list;
  while (NULL != pstRunner) {
    if ( (output_assembly == pstRunner->connection_path.connection_point[0])
         && (input_assembly
             == pstRunner->connection_path.connection_point[1]) ) {
      if ( kConnectionTriggerTypeApplicationTriggeredConnection
           == (pstRunner->transport_type_class_trigger
               & kConnectionTriggerTypeProductionTriggerMask) ) {
        /* produce at the next allowed occurrence */
        pstRunner->transmission_trigger_timer =
          pstRunner->production_inhibit_timer;
        status = kEipStatusOk;
      }
      break;
    }
  }
  return status;
}

void InitializeConnectionManagerData() {
  memset( g_connection_management_list, 0,
          g_kNumberOfConnectableObjects
          * sizeof(ConnectionManagementHandling) );
  InitializeClass3ConnectionData();
  InitializeIoConnectionData();
}
