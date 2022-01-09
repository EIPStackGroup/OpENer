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
#include "cipconnectionobject.h"
#include "cipclass3connection.h"
#include "cipioconnection.h"
#include "cipassembly.h"
#include "cpf.h"
#include "appcontype.h"
#include "generic_networkhandler.h"
#include "cipepath.h"
#include "cipelectronickey.h"
#include "cipqos.h"
#include "xorshiftrandom.h"

const size_t g_kForwardOpenHeaderLength = 36; /**< the length in bytes of the forward open command specific data till the start of the connection path (including con path size)*/
const size_t g_kLargeForwardOpenHeaderLength = 40; /**< the length in bytes of the large forward open command specific data till the start of the connection path (including con path size)*/

static const unsigned int g_kNumberOfConnectableObjects = 2 +
                                                          OPENER_CIP_NUM_APPLICATION_SPECIFIC_CONNECTABLE_OBJECTS;

typedef struct {
  EipUint32 class_id;
  OpenConnectionFunction open_connection_function;
} ConnectionManagementHandling;

/* global variables private */
/** List holding information on the object classes and open/close function
 * pointers to which connections may be established.
 */
ConnectionManagementHandling g_connection_management_list[2 +
                                                          OPENER_CIP_NUM_APPLICATION_SPECIFIC_CONNECTABLE_OBJECTS
] = {{0}};

/** buffer connection object needed for forward open */
CipConnectionObject g_dummy_connection_object;

/** @brief Holds the connection ID's "incarnation ID" in the upper 16 bits */
EipUint32 g_incarnation_id;

/* private functions */
EipStatus ForwardOpen(CipInstance *instance,
                      CipMessageRouterRequest *message_router_request,
                      CipMessageRouterResponse *message_router_response,
                      const struct sockaddr *originator_address,
                      const CipSessionHandle encapsulation_session);

EipStatus LargeForwardOpen(CipInstance *instance,
                           CipMessageRouterRequest *message_router_request,
                           CipMessageRouterResponse *message_router_response,
                           const struct sockaddr *originator_address,
                           const CipSessionHandle encapsulation_session);

EipStatus ForwardClose(CipInstance *instance,
                       CipMessageRouterRequest *message_router_request,
                       CipMessageRouterResponse *message_router_response,
                       const struct sockaddr *originator_address,
                       const CipSessionHandle encapsulation_session);

EipStatus GetConnectionOwner(CipInstance *instance,
                             CipMessageRouterRequest *message_router_request,
                             CipMessageRouterResponse *message_router_response,
                             const struct sockaddr *originator_address,
                             const CipSessionHandle encapsulation_session);

EipStatus GetConnectionData(CipInstance *instance,
                            CipMessageRouterRequest *message_router_request,
                            CipMessageRouterResponse *message_router_response,
                            const struct sockaddr *originator_address,
                            const CipUdint encapsulation_session);

EipStatus SearchConnectionData(CipInstance *instance,
                               CipMessageRouterRequest *message_router_request,
                               CipMessageRouterResponse *message_router_response,
                               const struct sockaddr *originator_address,
                               const CipUdint encapsulation_session);

void AssembleConnectionDataResponseMessage(
  CipMessageRouterResponse *message_router_response,
  CipConnectionObject *connection_object);

EipStatus AssembleForwardOpenResponse(CipConnectionObject *connection_object,
                                      CipMessageRouterResponse *message_router_response,
                                      EipUint8 general_status,
                                      EipUint16 extended_status);

EipStatus AssembleForwardCloseResponse(EipUint16 connection_serial_number,
                                       EipUint16 originatior_vendor_id,
                                       EipUint32 originator_serial_number,
                                       CipMessageRouterRequest *message_router_request,
                                       CipMessageRouterResponse *message_router_response,
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
CipConnectionObject *CheckForExistingConnection(
  const CipConnectionObject *const connection_object);

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
                                 void *key_data,
                                 EipUint16 *extended_status);

/** @brief Parse the connection path of a forward open request
 *
 * This function will take the connection object and the received data stream and parse the connection path.
 * @param connection_object pointer to the connection object structure for which the connection should
 *                      be established
 * @param message_router_request pointer to the received request structure. The position of the data stream pointer has to be at the connection length entry
 * @param extended_error the extended error code in case an error happened
 * @return general status on the establishment
 *    - kEipStatusOk ... on success
 *    - On an error the general status code to be put into the response
 */
EipUint8 ParseConnectionPath(CipConnectionObject *connection_object,
                             CipMessageRouterRequest *message_router_request,
                             EipUint16 *extended_error);

ConnectionManagementHandling *GetConnectionManagementEntry(
  const EipUint32 class_id);

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

  if( (padded_logical_path & 3) == 0 ) {
    padded_logical_path = *(*logical_path_segment)++;
  } else if( (padded_logical_path & 3) == 1 ) {
    (*logical_path_segment)++; /* skip pad */
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
 * and the per-new-connection connection number.  The legacy default is to use
 * the lower 16-bit as a connection counter, incrementing for each connection.
 *
 * Some conformance tests may however fail an adapter due to the connection ID
 * not being random enough.  To meet such requirements there is an option to
 * enable fully random connection IDs -- although the upper 16-bits are always
 * derived from the incarnation ID -- i.e., each time OpENer is started the
 * upper 16-bits will remain be the same.
 *
 * @return new 32-bit connection id
 */
CipUdint GetConnectionId(void) {
#ifndef OPENER_RANDOMIZE_CONNECTION_ID
  static CipUint connection_id = 18;
  connection_id++;
#else
  CipUint connection_id = NextXorShiftUint32();
#endif
  return (g_incarnation_id | (connection_id & 0x0000FFFF) );
}

void InitializeConnectionManager(CipClass *class) {

  CipClass *meta_class = class->class_instance.cip_class;

  InsertAttribute( (CipInstance *) class, 1, kCipUint, EncodeCipUint, NULL,
                   (void *) &class->revision, kGetableSingleAndAll ); /* revision */
  InsertAttribute( (CipInstance *) class, 2, kCipUint, EncodeCipUint, NULL,
                   (void *) &class->number_of_instances, kGetableSingleAndAll ); /* largest instance number */
  InsertAttribute( (CipInstance *) class, 3, kCipUint, EncodeCipUint, NULL,
                   (void *) &class->number_of_instances, kGetableSingle ); /* number of instances currently existing*/
  InsertAttribute( (CipInstance *) class, 4, kCipUint, EncodeCipUint, NULL,
                   (void *) &kCipUintZero, kNotSetOrGetable ); /* optional attribute list - default = 0 */
  InsertAttribute( (CipInstance *) class, 5, kCipUint, EncodeCipUint, NULL,
                   (void *) &kCipUintZero, kNotSetOrGetable ); /* optional service list - default = 0 */
  InsertAttribute( (CipInstance *) class, 6, kCipUint, EncodeCipUint, NULL,
                   (void *) &meta_class->highest_attribute_number,
                   kGetableSingleAndAll ); /* max class attribute number*/
  InsertAttribute( (CipInstance *) class, 7, kCipUint, EncodeCipUint, NULL,
                   (void *) &class->highest_attribute_number,
                   kGetableSingleAndAll ); /* max instance attribute number*/

  InsertService(meta_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"); /* bind instance services to the metaclass*/
  InsertService(meta_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle");

}

EipStatus ConnectionManagerInit(EipUint16 unique_connection_id) {
  InitializeConnectionManagerData();

  CipClass *connection_manager = CreateCipClass(kCipConnectionManagerClassCode, /* class code */
                                                0, /* # of class attributes */
                                                7, /* # highest class attribute number*/
                                                2, /* # of class services */
                                                0, /* # of instance attributes */
                                                14, /* # highest instance attribute number*/
                                                8, /* # of instance services */
                                                1, /* # of instances */
                                                "connection manager", /* class name */
                                                1, /* revision */
                                                &InitializeConnectionManager); /* # function pointer for initialization*/
  if(connection_manager == NULL) {
    return kEipStatusError;
  }
  InsertService(connection_manager,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle");
  InsertService(connection_manager,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll");
  InsertService(connection_manager, kForwardOpen, &ForwardOpen, "ForwardOpen");
  InsertService(connection_manager,
                kLargeForwardOpen,
                &LargeForwardOpen,
                "LargeForwardOpen");
  InsertService(connection_manager, kForwardClose, &ForwardClose,
                "ForwardClose");
  InsertService(connection_manager,
                kGetConnectionOwner,
                &GetConnectionOwner,
                "GetConnectionOwner");
  InsertService(connection_manager,
                kGetConnectionData,
                &GetConnectionData,
                "GetConnectionData");
  InsertService(connection_manager,
                kSearchConnectionData,
                &SearchConnectionData,
                "SearchConnectionData");

  g_incarnation_id = ( (EipUint32) unique_connection_id ) << 16;

  AddConnectableObject(kCipMessageRouterClassCode, EstablishClass3Connection);
  AddConnectableObject(kCipAssemblyClassCode, EstablishIoConnection);

  return kEipStatusOk;
}

EipStatus HandleReceivedConnectedData(const EipUint8 *const data,
                                      int data_length,
                                      struct sockaddr_in *from_address) {

  if( (CreateCommonPacketFormatStructure(data, data_length,
                                         &g_common_packet_format_data_item) ) ==
      kEipStatusError ) {
    return kEipStatusError;
  } else {
    /* check if connected address item or sequenced address item received, otherwise it is no connected message and should not be here */
    if( (g_common_packet_format_data_item.address_item.type_id ==
         kCipItemIdConnectionAddress)
        || (g_common_packet_format_data_item.address_item.type_id ==
            kCipItemIdSequencedAddressItem) ) { /* found connected address item or found sequenced address item -> for now the sequence number will be ignored */
      if(g_common_packet_format_data_item.data_item.type_id ==
         kCipItemIdConnectedDataItem) { /* connected data item received */

        CipConnectionObject *connection_object = GetConnectedObject(
          g_common_packet_format_data_item.address_item.data.connection_identifier);
        if(connection_object == NULL) {
          return kEipStatusError;
        }

        /* only handle the data if it is coming from the originator */
        if(connection_object->originator_address.sin_addr.s_addr ==
           from_address->sin_addr.s_addr) {
          ConnectionObjectResetLastPackageInactivityTimerValue(connection_object);

          if(SEQ_GT32(g_common_packet_format_data_item.address_item.data.
                      sequence_number,
                      connection_object->eip_level_sequence_count_consuming) ||
             !connection_object->eip_first_level_sequence_count_received) {
            /* reset the watchdog timer */
            ConnectionObjectResetInactivityWatchdogTimerValue(connection_object);

            /* only inform assembly object if the sequence counter is greater or equal */
            connection_object->eip_level_sequence_count_consuming =
              g_common_packet_format_data_item.address_item.data.sequence_number;
            connection_object->eip_first_level_sequence_count_received = true;

            if(NULL != connection_object->connection_receive_data_function) {
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

/** @brief Function prototype for all Forward Open handle functions
 *
 */
typedef EipStatus (*HandleForwardOpenRequestFunction)(CipConnectionObject *
                                                      connection_object,
                                                      CipInstance *instance,
                                                      CipMessageRouterRequest *
                                                      message_router_request,
                                                      CipMessageRouterResponse *
                                                      message_router_response);

/** @brief Handles a Null Non Matching Forward Open Request
 *
 * Null, Non-Matching - Either ping device, or configure a device’s application,
 * or return  General Status kCipErrorConnectionFailure and
 * Extended Status kConnectionManagerExtendedStatusCodeNullForwardOpenNotSupported
 */
EipStatus HandleNullNonMatchingForwardOpenRequest(
  CipConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

EipStatus HandleNullNonMatchingForwardOpenRequest(
  CipConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response) {
  /* Suppress unused parameter compiler warning. */
  (void) instance;
  (void) message_router_request;
  (void) message_router_response;

  OPENER_TRACE_INFO("Right now we cannot handle Null requests\n");
  return AssembleForwardOpenResponse(connection_object,
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
  CipConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

EipStatus HandleNullMatchingForwardOpenRequest(
  CipConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response) {
  /* Suppress unused parameter compiler warning. */
  (void) instance;
  (void) message_router_request;

  OPENER_TRACE_INFO("Right now we cannot handle Null requests\n");
  return AssembleForwardOpenResponse(connection_object,
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
  CipConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

EipStatus HandleNonNullMatchingForwardOpenRequest(
  CipConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response) {
  /* Suppress unused parameter compiler warning. */
  (void) instance;
  (void) message_router_request;

  OPENER_TRACE_INFO("Right now we cannot handle reconfiguration requests\n");
  return AssembleForwardOpenResponse(connection_object,
                                     message_router_response,
                                     kCipErrorConnectionFailure,
                                     kConnectionManagerExtendedStatusCodeErrorConnectionInUseOrDuplicateForwardOpen);
}

/** @brief Handles a Non Null Non Matching Forward Open Request
 *
 * Non-Null, Non-Matching request - Establish a new connection
 */
EipStatus HandleNonNullNonMatchingForwardOpenRequest(
  CipConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response);

EipStatus HandleNonNullNonMatchingForwardOpenRequest(
  CipConnectionObject *connection_object,
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response) {
  /* Suppress unused parameter compiler warning. */
  (void) connection_object;
  (void) instance;

  EipUint16 connection_status = kConnectionManagerExtendedStatusCodeSuccess;

  /*check if the trigger type value is invalid or ok */
  if(kConnectionObjectTransportClassTriggerProductionTriggerInvalid ==
     ConnectionObjectGetTransportClassTriggerProductionTrigger(&
                                                               g_dummy_connection_object) )
  {
    return AssembleForwardOpenResponse(&g_dummy_connection_object,
                                       message_router_response,
                                       kCipErrorConnectionFailure,
                                       kConnectionManagerExtendedStatusCodeErrorTransportClassAndTriggerCombinationNotSupported);
  }

  EipUint32 temp = ParseConnectionPath(&g_dummy_connection_object,
                                       message_router_request,
                                       &connection_status);
  if(kEipStatusOk != temp) {
    return AssembleForwardOpenResponse(&g_dummy_connection_object,
                                       message_router_response,
                                       temp,
                                       connection_status);
  }

  /*parsing is now finished all data is available and check now establish the connection */
  ConnectionManagementHandling *connection_management_entry =
    GetConnectionManagementEntry( /* Gets correct open connection function for the targeted object */
      g_dummy_connection_object.configuration_path.class_id);
  if(NULL != connection_management_entry) {
    if (NULL != connection_management_entry->open_connection_function) {
      temp = connection_management_entry->open_connection_function(
          &g_dummy_connection_object, &connection_status);
    } else {
      connection_status = kConnectionManagerExtendedStatusCodeMiscellaneous;
    }
  } else {
    temp = kEipStatusError;
    connection_status =
      kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
  }

  if(kEipStatusOk != temp) {
    OPENER_TRACE_INFO("connection manager: connect failed\n");
    /* in case of error the dummy objects holds all necessary information */
    return AssembleForwardOpenResponse(&g_dummy_connection_object,
                                       message_router_response,
                                       temp,
                                       connection_status);
  } else {
    OPENER_TRACE_INFO("connection manager: connect succeeded\n");
    /* in case of success the new connection is added at the head of the connection list */
    return AssembleForwardOpenResponse(connection_list.first->data,
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
    HandleNonNullMatchingForwardOpenRequest },
  { HandleNullNonMatchingForwardOpenRequest,
    HandleNullMatchingForwardOpenRequest } };

EipStatus ForwardOpenRoutine(CipInstance *instance,
                             CipMessageRouterRequest *message_router_request,
                             CipMessageRouterResponse *message_router_response,
                             const struct sockaddr *originator_address,
                             const CipSessionHandle encapsulation_session);

/** @brief Check if resources for new connection available, generate ForwardOpen Reply message.
 *
 * Large Forward Open service calls Forward Open service
 */
EipStatus LargeForwardOpen(CipInstance *instance,
                           CipMessageRouterRequest *message_router_request,
                           CipMessageRouterResponse *message_router_response,
                           const struct sockaddr *originator_address,
                           const CipSessionHandle encapsulation_session) {
  g_dummy_connection_object.is_large_forward_open = true;
  return ForwardOpenRoutine(instance,
                            message_router_request,
                            message_router_response,
                            originator_address,
                            encapsulation_session);
}

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
 *  @param originator_address address struct of the originator as received
 *  @param encapsulation_session associated encapsulation session of the explicit message
 *      @return >0 .. success, 0 .. no reply to send back
 *              -1 .. error
 */
EipStatus ForwardOpen(CipInstance *instance,
                      CipMessageRouterRequest *message_router_request,
                      CipMessageRouterResponse *message_router_response,
                      const struct sockaddr *originator_address,
                      const CipSessionHandle encapsulation_session) {
  g_dummy_connection_object.is_large_forward_open = false;
  return ForwardOpenRoutine(instance,
                            message_router_request,
                            message_router_response,
                            originator_address,
                            encapsulation_session);
}
EipStatus ForwardOpenRoutine(CipInstance *instance,
                             CipMessageRouterRequest *message_router_request,
                             CipMessageRouterResponse *message_router_response,
                             const struct sockaddr *originator_address,
                             const CipSessionHandle encapsulation_session) {
  (void) instance; /*suppress compiler warning */

  bool is_null_request = false; /* 1 = Null Request, 0 =  Non-Null Request  */
  bool is_matching_request = false; /* 1 = Matching Request, 0 = Non-Matching Request  */

  /*first check if we have already a connection with the given params */
  ConnectionObjectInitializeFromMessage(&(message_router_request->data),
                                        &g_dummy_connection_object);
  g_dummy_connection_object.associated_encapsulation_session =
    encapsulation_session;

  memcpy(&(g_dummy_connection_object.originator_address),
         originator_address,
         sizeof(g_dummy_connection_object.originator_address) );

  ConnectionObjectConnectionType o_to_t_connection_type =
    ConnectionObjectGetOToTConnectionType(&g_dummy_connection_object);
  ConnectionObjectConnectionType t_to_o_connection_type =
    ConnectionObjectGetTToOConnectionType(&g_dummy_connection_object);

  /* Check if both connection types are valid, otherwise send error response */
  if(kConnectionObjectConnectionTypeInvalid == o_to_t_connection_type) {
    return AssembleForwardOpenResponse(&g_dummy_connection_object,
                                       message_router_response,
                                       kCipErrorConnectionFailure,
                                       kConnectionManagerExtendedStatusCodeErrorInvalidOToTConnectionType);
  }

  if(kConnectionObjectConnectionTypeInvalid == t_to_o_connection_type) {
    return AssembleForwardOpenResponse(&g_dummy_connection_object,
                                       message_router_response,
                                       kCipErrorConnectionFailure,
                                       kConnectionManagerExtendedStatusCodeErrorInvalidTToOConnectionType);
  }

  if(kConnectionObjectConnectionTypeMulticast == t_to_o_connection_type) {
    /* for multicast, check if IP is within configured net because we send TTL 1 */
    CipUdint originator_ip =
      ( (struct sockaddr_in *) originator_address )->sin_addr.s_addr;
    CipUdint interface_ip = g_network_status.ip_address;
    CipUdint interface_mask = g_network_status.network_mask;
    if( (originator_ip & interface_mask) != (interface_ip & interface_mask) ) {
      return AssembleForwardOpenResponse(&g_dummy_connection_object,
                                         message_router_response,
                                         kCipErrorConnectionFailure,
                                         kConnectionManagerExtendedStatusCodeNotConfiguredForOffSubnetMulticast);
    }
  }

  /* Check if request is a Null request or a Non-Null request */
  if(kConnectionObjectConnectionTypeNull == o_to_t_connection_type &&
     kConnectionObjectConnectionTypeNull == t_to_o_connection_type) {
    is_null_request = true;
    OPENER_TRACE_INFO("We have a Null request\n");
  } else {
    is_null_request = false;
    OPENER_TRACE_INFO("We have a Non-Null request\n");
  }

  /* Check if we have a matching or non matching request */
  if(NULL != CheckForExistingConnection(&g_dummy_connection_object) ) {
    OPENER_TRACE_INFO("We have a Matching request\n");
    is_matching_request = true;

  } else {
    OPENER_TRACE_INFO("We have a Non-Matching request\n");
    is_matching_request = false;
  }

  HandleForwardOpenRequestFunction choosen_function =
    handle_forward_open_request_functions[is_null_request][is_matching_request];

  return choosen_function(&g_dummy_connection_object,
                          instance,
                          message_router_request,
                          message_router_response);
}

EipStatus ForwardClose(CipInstance *instance,
                       CipMessageRouterRequest *message_router_request,
                       CipMessageRouterResponse *message_router_response,
                       const struct sockaddr *originator_address,
                       const CipSessionHandle encapsulation_session) {
  /*Suppress compiler warning*/
  (void) instance;
  (void) encapsulation_session;

  /* check connection_serial_number && originator_vendor_id && originator_serial_number if connection is established */
  ConnectionManagerExtendedStatusCode connection_status =
    kConnectionManagerExtendedStatusCodeErrorConnectionTargetConnectionNotFound;

  /* set AddressInfo Items to invalid TypeID to prevent assembleLinearMsg to read them */
  g_common_packet_format_data_item.address_info_item[0].type_id = 0;
  g_common_packet_format_data_item.address_info_item[1].type_id = 0;

  message_router_request->data += 2; /* ignore Priority/Time_tick and Time-out_ticks */

  EipUint16 connection_serial_number = GetUintFromMessage(
    &message_router_request->data);
  EipUint16 originator_vendor_id = GetUintFromMessage(
    &message_router_request->data);
  EipUint32 originator_serial_number = GetUdintFromMessage(
    &message_router_request->data);

  OPENER_TRACE_INFO("ForwardClose: ConnSerNo %d\n", connection_serial_number);

  DoublyLinkedListNode *node = connection_list.first;

  while(NULL != node) {
    /* this check should not be necessary as only established connections should be in the active connection list */
    CipConnectionObject *connection_object = node->data;
    if( (kConnectionObjectStateEstablished ==
         ConnectionObjectGetState(connection_object) )
        || (kConnectionObjectStateTimedOut ==
            ConnectionObjectGetState(connection_object) ) ) {
      if( (connection_object->connection_serial_number ==
           connection_serial_number) &&
          (connection_object->originator_vendor_id == originator_vendor_id)
          && (connection_object->originator_serial_number ==
              originator_serial_number) ) {
        /* found the corresponding connection object -> close it */
        OPENER_ASSERT(NULL != connection_object->connection_close_function);
        if( ( (struct sockaddr_in *) originator_address )->sin_addr.s_addr ==
            connection_object->originator_address.sin_addr.s_addr ) {
          connection_object->connection_close_function(connection_object);
          connection_status = kConnectionManagerExtendedStatusCodeSuccess;
        } else {
          connection_status = kConnectionManagerExtendedStatusWrongCloser;
        }
        break;
      }
    }
    node = node->next;
  }
  if(kConnectionManagerExtendedStatusCodeErrorConnectionTargetConnectionNotFound
     == connection_status) {
    OPENER_TRACE_INFO(
      "Connection not found! Requested connection tried: %u, %u, %i\n",
      connection_serial_number,
      originator_vendor_id,
      originator_serial_number);
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
                             const struct sockaddr *originator_address,
                             const CipSessionHandle encapsulation_session) {
  /* suppress compiler warnings */
  (void) instance;
  (void) message_router_request;
  (void) message_router_response;
  (void) originator_address;
  (void) encapsulation_session;

  return kEipStatusOk;
}

EipStatus GetConnectionData(CipInstance *instance,
                            CipMessageRouterRequest *message_router_request,
                            CipMessageRouterResponse *message_router_response,
                            const struct sockaddr *originator_address,
                            const CipUdint encapsulation_session) {
  /* Suppress unused parameter compiler warning. */
  (void)instance;
  (void)originator_address;
  (void)encapsulation_session;

  CIPServiceCode service_code = kGetConnectionData;
  message_router_response->reply_service = (0x80 | service_code);

  //get Connection Number from request
  EipUint16 Connection_number =
    GetUintFromMessage(&message_router_request->data);

  OPENER_TRACE_INFO("GetConnectionData for Connection_number: %d\n",
                    Connection_number);

  //search connection
  DoublyLinkedListNode *iterator = connection_list.first;
  CipConnectionObject *search_connection_object = NULL;
  CipConnectionObject *connection_object = NULL;

  while(NULL != iterator) {
    search_connection_object = iterator->data;

    if( (search_connection_object->connection_number == Connection_number) ) {
      connection_object = search_connection_object;
      break;
    }
    iterator = iterator->next;
  }

  if(NULL != connection_object) {
    /* assemble response message */
    AssembleConnectionDataResponseMessage(message_router_response,
                                          connection_object);
    message_router_response->general_status = kEipStatusOk;
    OPENER_TRACE_INFO("Connection found!\n");
  } else {
    message_router_response->general_status = kCipErrorPathDestinationUnknown;
    OPENER_TRACE_INFO("Connection not found!\n");
  }

  return kEipStatusOk;
}

EipStatus SearchConnectionData(CipInstance *instance,
                               CipMessageRouterRequest *message_router_request,
                               CipMessageRouterResponse *message_router_response,
                               const struct sockaddr *originator_address,
                               const CipUdint encapsulation_session) {
  /* Suppress unused parameter compiler warning. */
  (void)instance;
  (void)originator_address;
  (void)encapsulation_session;

  CIPServiceCode service_code = kSearchConnectionData;
  message_router_response->reply_service = (0x80 | service_code);

  //connection data (connection triad) from request
  EipUint16 Connection_serial_number = GetUintFromMessage(
    &message_router_request->data);
  EipUint16 Originator_vendor_id = GetUintFromMessage(
    &message_router_request->data);
  EipUint32 Originator_serial_number = GetUdintFromMessage(
    &message_router_request->data);

  OPENER_TRACE_INFO(
    "SearchConnectionData for ConnSerNo: %d, OrigVendId: %d, OrigSerNo: %i,\n",
    Connection_serial_number,
    Originator_vendor_id,
    Originator_serial_number);

  //search connection
  DoublyLinkedListNode *iterator = connection_list.first;
  CipConnectionObject *search_connection_object = NULL;
  CipConnectionObject *connection_object = NULL;

  while(NULL != iterator) {
    search_connection_object = iterator->data;

    if( (search_connection_object->connection_serial_number ==
         Connection_serial_number)
        && (search_connection_object->originator_vendor_id ==
            Originator_vendor_id)
        && (search_connection_object->originator_serial_number ==
            Originator_serial_number) ) {

      connection_object = search_connection_object;
      break;
    }
    iterator = iterator->next;
  }
  if(NULL != connection_object) {
    /* assemble response message */
    AssembleConnectionDataResponseMessage(message_router_response,
                                          connection_object);
    message_router_response->general_status = kEipStatusOk;
    OPENER_TRACE_INFO("Connection found!\n");
  } else {
    message_router_response->general_status = kCipErrorPathDestinationUnknown;
    OPENER_TRACE_INFO("Connection not found!\n");
  }

  return kEipStatusOk;
}

void AssembleConnectionDataResponseMessage(
  CipMessageRouterResponse *message_router_response,
  CipConnectionObject *connection_object) {

  // Connection number UINT
  AddIntToMessage(connection_object->connection_number,
                  &message_router_response->message);
  // Connection state UINT
  AddIntToMessage(connection_object->state, &message_router_response->message);
  // Originator Port UINT
  AddIntToMessage(connection_object->originator_address.sin_port,
                  &message_router_response->message);
  // Target Port UINT
  AddIntToMessage(connection_object->remote_address.sin_port,
                  &message_router_response->message);
  // Connection Serial Number UINT
  AddIntToMessage(connection_object->connection_serial_number,
                  &message_router_response->message);
  // Originator Vendor ID UINT
  AddIntToMessage(connection_object->originator_vendor_id,
                  &message_router_response->message);
  // Originator Serial number UDINT
  AddDintToMessage(connection_object->originator_serial_number,
                   &message_router_response->message);
  // Originator O->T CID UDINT
  AddDintToMessage(connection_object->cip_consumed_connection_id,
                   &message_router_response->message);
  // Target O->T CID UDINT
  AddDintToMessage(connection_object->cip_consumed_connection_id,
                   &message_router_response->message);
  // Connection Timeout Multiplier USINT
  AddSintToMessage(connection_object->connection_timeout_multiplier,
                   &message_router_response->message);
  // Reserved USINT
  AddSintToMessage(0, &message_router_response->message);
  // Reserved USINT
  AddSintToMessage(0, &message_router_response->message);
  // Reserved USINT
  AddSintToMessage(0, &message_router_response->message);
  // Originator RPI O->T UDINT
  AddDintToMessage(connection_object->o_to_t_requested_packet_interval,
                   &message_router_response->message);
  // Originator API O->T UDINT
  AddDintToMessage(connection_object->transmission_trigger_timer,
                   &message_router_response->message);
  // Originator T->O CID UDINT
  AddDintToMessage(connection_object->cip_produced_connection_id,
                   &message_router_response->message);
  // Target T->O CID UDINT
  AddDintToMessage(connection_object->cip_produced_connection_id,
                   &message_router_response->message);
  // Connection Timeout Multiplier USINT
  AddSintToMessage(connection_object->connection_timeout_multiplier,
                   &message_router_response->message);
  // Reserved USINT
  AddSintToMessage(0, &message_router_response->message);
  // Reserved USINT
  AddSintToMessage(0, &message_router_response->message);
  // Reserved USINT
  AddSintToMessage(0, &message_router_response->message);
  // Originator RPI T->O UDINT
  AddDintToMessage(connection_object->t_to_o_requested_packet_interval,
                   &message_router_response->message);
  // Originator API T->O UDINT
  AddDintToMessage(connection_object->transmission_trigger_timer,
                   &message_router_response->message);
}

EipStatus ManageConnections(MilliSeconds elapsed_time) {
  //OPENER_TRACE_INFO("Entering ManageConnections\n");
  /*Inform application that it can execute */
  HandleApplication();
  ManageEncapsulationMessages(elapsed_time);

  DoublyLinkedListNode *node = connection_list.first;

  while(NULL != node) {
    //OPENER_TRACE_INFO("Entering Connection Object loop\n");
    CipConnectionObject *connection_object = node->data;
    if(kConnectionObjectStateEstablished ==
       ConnectionObjectGetState(connection_object) ) {
      if( (NULL != connection_object->consuming_instance) || /* we have a consuming connection check inactivity watchdog timer */
          (kConnectionObjectTransportClassTriggerDirectionServer ==
           ConnectionObjectGetTransportClassTriggerDirection(connection_object) ) ) /* all server connections have to maintain an inactivity watchdog timer */
      {
        if(elapsed_time >= connection_object->inactivity_watchdog_timer) {
          /* we have a timed out connection perform watchdog time out action*/
          OPENER_TRACE_INFO(">>>>>>>>>>Connection ConnNr: %u timed out\n",
                            connection_object->connection_serial_number);
          OPENER_ASSERT(NULL != connection_object->connection_timeout_function);
          connection_object->connection_timeout_function(connection_object);
        } else {
          connection_object->inactivity_watchdog_timer -= elapsed_time;
          connection_object->last_package_watchdog_timer -= elapsed_time;
        }
      }
      /* only if the connection has not timed out check if data is to be send */
      if(kConnectionObjectStateEstablished ==
         ConnectionObjectGetState(connection_object) ) {
        /* client connection */
        if( (0 != ConnectionObjectGetExpectedPacketRate(connection_object) )
            && (kEipInvalidSocket !=
                connection_object->socket[kUdpCommuncationDirectionProducing]) ) /* only produce for the master connection */
        {
          if(kConnectionObjectTransportClassTriggerProductionTriggerCyclic !=
             ConnectionObjectGetTransportClassTriggerProductionTrigger(
               connection_object) ) {
            /* non cyclic connections have to decrement production inhibit timer */
            if(elapsed_time <= connection_object->production_inhibit_timer) {
              //The connection is allowed to send again
            } else {
              connection_object->production_inhibit_timer -= elapsed_time;
            }
          }

          if(connection_object->transmission_trigger_timer <= elapsed_time) { /* need to send package */
            OPENER_ASSERT(
              NULL != connection_object->connection_send_data_function);
            EipStatus eip_status =
              connection_object->connection_send_data_function(connection_object);
            if(eip_status == kEipStatusError) {
              OPENER_TRACE_ERR(
                "sending of UDP data in manage Connection failed\n");
            }
            /* add the RPI to the timer value */
            connection_object->transmission_trigger_timer +=
              ConnectionObjectGetRequestedPacketInterval(connection_object);
            /* decrecment the elapsed time from timer value, if less than timer value */
            if (connection_object->transmission_trigger_timer > elapsed_time) {
              connection_object->transmission_trigger_timer -= elapsed_time;
            } else {  /* elapsed time was longer than RPI */
              connection_object->transmission_trigger_timer = 0;
              OPENER_TRACE_INFO("elapsed time: %lu ms was longer than RPI: %u ms\n",
                                elapsed_time,
                                ConnectionObjectGetRequestedPacketInterval(connection_object));
            }
            if(kConnectionObjectTransportClassTriggerProductionTriggerCyclic !=
               ConnectionObjectGetTransportClassTriggerProductionTrigger(
                 connection_object) ) {
              /* non cyclic connections have to reload the production inhibit timer */
              ConnectionObjectResetProductionInhibitTimer(connection_object);
            }
          } else {
            connection_object->transmission_trigger_timer -= elapsed_time;
          }
        }
      }
    }
    node = node->next;
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
EipStatus AssembleForwardOpenResponse(CipConnectionObject *connection_object,
                                      CipMessageRouterResponse *message_router_response,
                                      EipUint8 general_status,
                                      EipUint16 extended_status) {
  /* write reply information in CPF struct dependent of pa_status */
  CipCommonPacketFormatData *cip_common_packet_format_data =
    &g_common_packet_format_data_item;
  cip_common_packet_format_data->item_count = 2;
  cip_common_packet_format_data->data_item.type_id =
    kCipItemIdUnconnectedDataItem;

  AddNullAddressItem(cip_common_packet_format_data);

  CIPServiceCode service_code = kForwardOpen;
  if(connection_object->is_large_forward_open) {
    service_code = kLargeForwardOpen;
  }

  message_router_response->reply_service = (0x80 | service_code);
  message_router_response->general_status = general_status;

  if(kCipErrorSuccess == general_status) {
    OPENER_TRACE_INFO("assembleFWDOpenResponse: sending success response\n");
    /* if there is no application specific data, total length should be 26 */
    message_router_response->size_of_additional_status = 0;

    if(cip_common_packet_format_data->address_info_item[0].type_id != 0) {
      cip_common_packet_format_data->item_count = 3;
      if(cip_common_packet_format_data->address_info_item[1].type_id != 0) {
        cip_common_packet_format_data->item_count = 4; /* there are two sockaddrinfo items to add */
      }
    }

    AddDintToMessage(connection_object->cip_consumed_connection_id,
                     &message_router_response->message);
    AddDintToMessage(connection_object->cip_produced_connection_id,
                     &message_router_response->message);
  } else {
    /* we have an connection creation error */
    OPENER_TRACE_WARN("AssembleForwardOpenResponse: sending error response, general/extended status=%d/%d\n", general_status, extended_status);
    ConnectionObjectSetState(connection_object,
                             kConnectionObjectStateNonExistent);
    /* Expected data length is 10 octets */

    switch(general_status) {
      case kCipErrorNotEnoughData:
      case kCipErrorTooMuchData: {
        message_router_response->size_of_additional_status = 0;
        break;
      }

      default: {
        switch(extended_status) {
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

  AddIntToMessage(connection_object->connection_serial_number,
                  &message_router_response->message);
  AddIntToMessage(connection_object->originator_vendor_id,
                  &message_router_response->message);
  AddDintToMessage(connection_object->originator_serial_number,
                   &message_router_response->message);

  if(kCipErrorSuccess == general_status) {
    /* set the actual packet rate to requested packet rate */
    AddDintToMessage(connection_object->o_to_t_requested_packet_interval,
                     &message_router_response->message);
    AddDintToMessage(connection_object->t_to_o_requested_packet_interval,
                     &message_router_response->message);
  }

  AddSintToMessage(0, &message_router_response->message); /* remaining path size - for routing devices relevant */
  AddSintToMessage(0, &message_router_response->message); /* reserved */

  return kEipStatusOkSend; /* send reply */
}

/**
 * @brief Adds a Null Address Item to the common data packet format data
 * @param common_data_packet_format_data The CPF data packet where the Null Address Item shall be added
 */
void AddNullAddressItem(
  CipCommonPacketFormatData *common_data_packet_format_data) {
  /* Precondition: Null Address Item only valid in unconnected messages */
  assert(
    common_data_packet_format_data->data_item.type_id ==
    kCipItemIdUnconnectedDataItem);

  common_data_packet_format_data->address_item.type_id = kCipItemIdNullAddress;
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
EipStatus AssembleForwardCloseResponse(EipUint16 connection_serial_number,
                                       EipUint16 originatior_vendor_id,
                                       EipUint32 originator_serial_number,
                                       CipMessageRouterRequest *message_router_request,
                                       CipMessageRouterResponse *message_router_response,
                                       EipUint16 extended_error_code) {
  /* write reply information in CPF struct dependent of pa_status */
  CipCommonPacketFormatData *common_data_packet_format_data =
    &g_common_packet_format_data_item;
  common_data_packet_format_data->item_count = 2;
  common_data_packet_format_data->data_item.type_id =
    kCipItemIdUnconnectedDataItem;

  AddNullAddressItem(common_data_packet_format_data);

  AddIntToMessage(connection_serial_number, &message_router_response->message);
  AddIntToMessage(originatior_vendor_id, &message_router_response->message);
  AddDintToMessage(originator_serial_number, &message_router_response->message);

  message_router_response->reply_service =
    (0x80 | message_router_request->service);
  /* Excepted length is 10 if there is no application specific data */

  if(kConnectionManagerExtendedStatusCodeSuccess == extended_error_code) {
    AddSintToMessage(0, &message_router_response->message); /* no application data */
    message_router_response->general_status = kCipErrorSuccess;
    message_router_response->size_of_additional_status = 0;
  } else {
    AddSintToMessage(*message_router_request->data,
                     &message_router_response->message); /* remaining path size */
    if(kConnectionManagerExtendedStatusWrongCloser == extended_error_code) {
      message_router_response->general_status = kCipErrorPrivilegeViolation;
    } else {
      message_router_response->general_status = kCipErrorConnectionFailure;
      message_router_response->additional_status[0] = extended_error_code;
      message_router_response->size_of_additional_status = 1;
    }
  }

  AddSintToMessage(0, &message_router_response->message); /* reserved */

  return kEipStatusOkSend;
}

CipConnectionObject *GetConnectedObject(const EipUint32 connection_id) {
  DoublyLinkedListNode *iterator = connection_list.first;

  while(NULL != iterator) {
    if(kConnectionObjectStateEstablished ==
       ConnectionObjectGetState(iterator->data)
       && connection_id ==
       ConnectionObjectGetCipConsumedConnectionID(iterator->data) ) {
      return iterator->data;
    }
    iterator = iterator->next;
  }
  return NULL;
}

CipConnectionObject *GetConnectedOutputAssembly(
  const EipUint32 output_assembly_id) {
  DoublyLinkedListNode *iterator = connection_list.first;

  while(NULL != iterator) {
    if(kConnectionObjectInstanceTypeIOExclusiveOwner ==
       ConnectionObjectGetInstanceType(iterator->data)
       && (kConnectionObjectStateEstablished ==
           ConnectionObjectGetState(iterator->data)
           || kConnectionObjectStateTimedOut ==
           ConnectionObjectGetState(iterator->data) )
       && output_assembly_id ==
       ( (CipConnectionObject *) iterator->data )->produced_path.instance_id) {
      return iterator->data;
    }
    iterator = iterator->next;
  }
  return NULL;
}

CipConnectionObject *CheckForExistingConnection(
  const CipConnectionObject *const connection_object) {

  DoublyLinkedListNode *iterator = connection_list.first;

  while(NULL != iterator) {
    if(kConnectionObjectStateEstablished ==
       ConnectionObjectGetState(iterator->data) ) {
      if(EqualConnectionTriad(connection_object, iterator->data) ) {
        return iterator->data;
      }
    }
    iterator = iterator->next;
  }

  return NULL;
}

EipStatus CheckElectronicKeyData(EipUint8 key_format,
                                 void *key_data,
                                 EipUint16 *extended_status) {
  /* Default return value */
  *extended_status = kConnectionManagerExtendedStatusCodeSuccess;

  /* Check key format */
  if(4 != key_format) {
    *extended_status =
      kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
    return kEipStatusError;
  }

  bool compatiblity_mode = ElectronicKeyFormat4GetMajorRevisionCompatibility(
    key_data);

  /* Check VendorID and ProductCode, must match, or 0 */
  if( ( (ElectronicKeyFormat4GetVendorId(key_data) != g_identity.vendor_id) &&
        (ElectronicKeyFormat4GetVendorId(key_data) != 0) )
      || ( (ElectronicKeyFormat4GetProductCode(key_data) !=
            g_identity.product_code) &&
           (ElectronicKeyFormat4GetProductCode(key_data) != 0) ) ) {
    *extended_status =
      kConnectionManagerExtendedStatusCodeErrorVendorIdOrProductcodeError;
    return kEipStatusError;
  } else {
    /* VendorID and ProductCode are correct */

    /* Check DeviceType, must match or 0 */
    if( (ElectronicKeyFormat4GetDeviceType(key_data) !=
         g_identity.device_type) &&
        (ElectronicKeyFormat4GetDeviceType(key_data) != 0) ) {
      *extended_status =
        kConnectionManagerExtendedStatusCodeErrorDeviceTypeError;
      return kEipStatusError;
    } else {
      /* VendorID, ProductCode and DeviceType are correct */

      if(false == compatiblity_mode) {
        /* Major = 0 is valid */
        if(0 == ElectronicKeyFormat4GetMajorRevision(key_data) ) {
          return kEipStatusOk;
        }

        /* Check Major / Minor Revision, Major must match, Minor match or 0 */
        if( (ElectronicKeyFormat4GetMajorRevision(key_data) !=
             g_identity.revision.major_revision)
            || ( (ElectronicKeyFormat4GetMinorRevision(key_data) !=
                  g_identity.revision.minor_revision) &&
                 (ElectronicKeyFormat4GetMinorRevision(key_data) != 0) ) ) {
          *extended_status =
            kConnectionManagerExtendedStatusCodeErrorRevisionMismatch;
          return kEipStatusError;
        }
      } else {
        /* Compatibility mode is set */

        /* Major must match, Minor != 0 and <= MinorRevision */
        if( (ElectronicKeyFormat4GetMajorRevision(key_data) ==
             g_identity.revision.major_revision) &&
            (ElectronicKeyFormat4GetMinorRevision(key_data) > 0)
            && (ElectronicKeyFormat4GetMinorRevision(key_data) <=
                g_identity.revision.minor_revision) ) {
          return kEipStatusOk;
        } else {
          *extended_status =
            kConnectionManagerExtendedStatusCodeErrorRevisionMismatch;
          return kEipStatusError;
        }
      } /* end if CompatiblityMode handling */
    }
  }

  return (*extended_status ==
          kConnectionManagerExtendedStatusCodeSuccess) ? kEipStatusOk :
         kEipStatusError;
}

EipUint8 ParseConnectionPath(CipConnectionObject *connection_object,
                             CipMessageRouterRequest *message_router_request,
                             EipUint16 *extended_error) {
  const EipUint8 *message = message_router_request->data;
  const size_t connection_path_size = GetUsintFromMessage(&message); /* length in words */
  if(0 == connection_path_size) {
    // A (large) forward open request needs to have a connection path size larger than 0
    return kEipStatusError;
  }
  size_t remaining_path = connection_path_size;
  OPENER_TRACE_INFO("Received connection path size: %zu \n",
                    connection_path_size);
  CipClass *class = NULL;

  CipDword class_id = 0x0;
  CipInstanceNum instance_id = 0x0;

  /* with 256 we mark that we haven't got a PIT segment */
  ConnectionObjectSetProductionInhibitTime(connection_object, 256);

  size_t header_length = g_kForwardOpenHeaderLength;
  if(connection_object->is_large_forward_open) {
    header_length = g_kLargeForwardOpenHeaderLength;
  }

  if( ( header_length + remaining_path * sizeof(CipWord) ) <
      message_router_request->request_data_size ) {
    /* the received packet is larger than the data in the path */
    *extended_error = 0;
    return kCipErrorTooMuchData;
  }

  if( ( header_length + remaining_path * sizeof(CipWord) ) >
      message_router_request->request_data_size ) {
    /*there is not enough data in received packet */
    *extended_error = 0;
    OPENER_TRACE_INFO("Message not long enough for path\n");
    return kCipErrorNotEnoughData;
  }

  if(remaining_path > 0) {
    /* first look if there is an electronic key */
    if(kSegmentTypeLogicalSegment == GetPathSegmentType(message) ) {
      if(kLogicalSegmentLogicalTypeSpecial ==
         GetPathLogicalSegmentLogicalType(message) ) {
        if(kLogicalSegmentSpecialTypeLogicalFormatElectronicKey ==
           GetPathLogicalSegmentSpecialTypeLogicalType(message) ) {
          if(kElectronicKeySegmentFormatKeyFormat4 ==
             GetPathLogicalSegmentElectronicKeyFormat(message) ) {
            /* Check if there is enough data for holding the electronic key segment */
            if(remaining_path < 5) {
              *extended_error = 0;
              OPENER_TRACE_INFO("Message not long enough for electronic key\n");
              return kCipErrorNotEnoughData;
            }
            /* Electronic key format 4 found */
            connection_object->electronic_key.key_format = 4;
            ElectronicKeyFormat4 *electronic_key = ElectronicKeyFormat4New();
            GetElectronicKeyFormat4FromMessage(&message, electronic_key);
            /* logical electronic key found */
            connection_object->electronic_key.key_data = electronic_key;

            remaining_path -= 5; /*length of the electronic key*/
            OPENER_TRACE_INFO(
              "key: ven ID %d, dev type %d, prod code %d, major %d, minor %d\n",
              ElectronicKeyFormat4GetVendorId(connection_object->electronic_key.
                                              key_data),
              ElectronicKeyFormat4GetDeviceType(connection_object->
                                                electronic_key.key_data),
              ElectronicKeyFormat4GetProductCode(connection_object->
                                                 electronic_key.key_data),
              ElectronicKeyFormat4GetMajorRevision(connection_object->
                                                   electronic_key.key_data),
              ElectronicKeyFormat4GetMinorRevision(connection_object->
                                                   electronic_key.key_data) );
            if(kEipStatusOk
               != CheckElectronicKeyData(connection_object->electronic_key.
                                         key_format,
                                         connection_object->electronic_key.
                                         key_data,
                                         extended_error) ) {
              ElectronicKeyFormat4Delete(&electronic_key);
              return kCipErrorConnectionFailure;
            }
            ElectronicKeyFormat4Delete(&electronic_key);
          }

        } else {
          OPENER_TRACE_INFO("no key\n");
        }
      }
    }

    //TODO: Refactor this afterwards
    if(kConnectionObjectTransportClassTriggerProductionTriggerCyclic !=
       ConnectionObjectGetTransportClassTriggerProductionTrigger(
         connection_object) )
    {
      /*non cyclic connections may have a production inhibit */
      if(kSegmentTypeNetworkSegment == GetPathSegmentType(message) ) {
        NetworkSegmentSubtype network_segment_subtype =
          GetPathNetworkSegmentSubtype(message);
        if(kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds ==
           network_segment_subtype) {
          OPENER_TRACE_INFO("PIT segment available - value: %u\n",message[1]);
          connection_object->production_inhibit_time = message[1];
          message += 2;
          remaining_path -= 1;
        }
      }
    }

    if(kSegmentTypeLogicalSegment == GetPathSegmentType(message) &&
       kLogicalSegmentLogicalTypeClassId ==
       GetPathLogicalSegmentLogicalType(message) ) {

      class_id = CipEpathGetLogicalValue(&message);
      class = GetCipClass(class_id);
      if(NULL == class) {
        OPENER_TRACE_ERR("classid %" PRIx32 " not found\n",
                         class_id);

        if(class_id >= 0xC8) { /*reserved range of class ids */
          *extended_error =
            kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
        } else {
          *extended_error =
            kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
        }
        return kCipErrorConnectionFailure;
      }

      OPENER_TRACE_INFO("classid %" PRIx32 " (%s)\n",
                        class_id,
                        class->class_name);
    } else {
      *extended_error =
        kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
      return kCipErrorConnectionFailure;
    }
    remaining_path -= 1; /* 1 16Bit word for the class part of the path */

    /* Get instance ID */
    if(kSegmentTypeLogicalSegment == GetPathSegmentType(message) &&
       kLogicalSegmentLogicalTypeInstanceId ==
       GetPathLogicalSegmentLogicalType(message) ) { /* store the configuration ID for later checking in the application connection types */
      const CipDword temp_id = CipEpathGetLogicalValue(&message);

      OPENER_TRACE_INFO("Configuration instance id %" PRId32 "\n",
                        temp_id);
      if( (temp_id > kCipInstanceNumMax) ||
          ( NULL == GetCipInstance(class, (CipInstanceNum)temp_id) ) ) {
        /*according to the test tool we should respond with this extended error code */
        *extended_error =
          kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
        return kCipErrorConnectionFailure;
      }
      instance_id = (CipInstanceNum)temp_id;
      /* 1 or 2 16Bit words for the configuration instance part of the path  */
      remaining_path -= (instance_id > 0xFF) ? 2 : 1; //TODO: 32 bit case missing
    } else {
      OPENER_TRACE_INFO("no config data\n");
    }

    if(kConnectionObjectTransportClassTriggerTransportClass3 ==
       ConnectionObjectGetTransportClassTriggerTransportClass(connection_object) )
    {
      /*we have Class 3 connection*/
      if(remaining_path > 0) {
        OPENER_TRACE_WARN(
          "Too much data in connection path for class 3 connection\n");
        *extended_error =
          kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
        return kCipErrorConnectionFailure;
      }

      /* connection end point has to be the message router instance 1 */
      if( (class_id != kCipMessageRouterClassCode) || (1 != instance_id) ) {
        *extended_error =
          kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
        return kCipErrorConnectionFailure;
      }
      /* Configuration connection point is producing connection point */
      CipConnectionPathEpath connection_epath =
      { .class_id = class_id, .instance_id = instance_id,
        .attribute_id_or_connection_point = 0 };

      memcpy(&(connection_object->configuration_path),
             &connection_epath,
             sizeof(connection_object->configuration_path) );
      memcpy(&(connection_object->produced_path), &connection_epath,
             sizeof(connection_object->produced_path) );

      /* End class 3 connection handling */
    } else { /* we have an IO connection */
      CipConnectionPathEpath connection_epath =
      { .class_id = class_id, .instance_id = instance_id,
        .attribute_id_or_connection_point = 0 };
      memcpy(&(connection_object->configuration_path),
             &connection_epath,
             sizeof(connection_object->configuration_path) );
      ConnectionObjectConnectionType originator_to_target_connection_type =
        ConnectionObjectGetOToTConnectionType(connection_object);
      ConnectionObjectConnectionType target_to_originator_connection_type =
        ConnectionObjectGetTToOConnectionType(connection_object);

      connection_object->consumed_connection_path_length = 0;
      connection_object->consumed_connection_path = NULL;
      //connection_object->connection_path.connection_point[1] = 0; /* set not available path to Invalid */

      size_t number_of_encoded_paths = 0;
      CipConnectionPathEpath *paths_to_encode[2] = { 0 };
      if(kConnectionObjectConnectionTypeNull ==
         originator_to_target_connection_type) {
        if(kConnectionObjectConnectionTypeNull ==
           target_to_originator_connection_type) { /* configuration only connection */
          number_of_encoded_paths = 0;
          OPENER_TRACE_WARN("assembly: type invalid\n");
        } else { /* 1 path -> path is for production */
          OPENER_TRACE_INFO("assembly: type produce\n");
          number_of_encoded_paths = 1;
          paths_to_encode[0] = &(connection_object->produced_path);
        }
      } else {
        if(kConnectionObjectConnectionTypeNull ==
           target_to_originator_connection_type) { /* 1 path -> path is for consumption */
          OPENER_TRACE_INFO("assembly: type consume\n");
          number_of_encoded_paths = 1;
          paths_to_encode[0] = &(connection_object->consumed_path);
        } else { /* 2 paths -> 1st for production 2nd for consumption */
          OPENER_TRACE_INFO("assembly: type bidirectional\n");
          paths_to_encode[0] = &(connection_object->consumed_path);
          paths_to_encode[1] = &(connection_object->produced_path);
          number_of_encoded_paths = 2;
        }
      }

      for(size_t i = 0; i < number_of_encoded_paths; i++) /* process up to 2 encoded paths */
      {
        if(kSegmentTypeLogicalSegment == GetPathSegmentType(message)
           && (kLogicalSegmentLogicalTypeInstanceId ==
               GetPathLogicalSegmentLogicalType(message)
               || kLogicalSegmentLogicalTypeConnectionPoint ==
               GetPathLogicalSegmentLogicalType(message) ) ) /* Connection Point interpreted as InstanceNr -> only in Assembly Objects */
        {   /* Attribute Id or Connection Point */

          /* Validate encoded instance number. */
          const CipDword temp_instance_id = CipEpathGetLogicalValue(&message);
          if (temp_instance_id > kCipInstanceNumMax) {
            *extended_error =
              kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
            return kCipErrorConnectionFailure;
          }
          instance_id = (CipInstanceNum)temp_instance_id;

          CipConnectionPathEpath path;
          path.class_id = class_id;
          path.instance_id = instance_id;
          path.attribute_id_or_connection_point = 0;
          memcpy(paths_to_encode[i], &path,
                 sizeof(connection_object->produced_path) );
          OPENER_TRACE_INFO(
            "connection point %" PRIu32 "\n",
            instance_id);
          if( NULL == GetCipInstance(class, instance_id) ) {
            *extended_error =
              kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
            return kCipErrorConnectionFailure;
          }
          /* 1 or 2 16Bit word for the connection point part of the path */
          remaining_path -= (instance_id > 0xFF) ? 2 : 1;
        } else {
          *extended_error =
            kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
          return kCipErrorConnectionFailure;
        }
      }

      g_config_data_length = 0;
      g_config_data_buffer = NULL;

      while(remaining_path > 0) { /* remaining_path_size something left in the path should be configuration data */

        SegmentType segment_type = GetPathSegmentType(message);
        switch(segment_type) {
          case kSegmentTypeDataSegment: {
            DataSegmentSubtype data_segment_type = GetPathDataSegmentSubtype(
              message);
            switch(data_segment_type) {
              case kDataSegmentSubtypeSimpleData:
                g_config_data_length = message[1] * 2; /*data segments store length 16-bit word wise */
                g_config_data_buffer = (EipUint8 *) message + 2;
                remaining_path -= (g_config_data_length + 2) / 2;
                message += (g_config_data_length + 2);
                break;
              default:
                OPENER_TRACE_ERR("Not allowed in connection manager");
                return kCipErrorPathSegmentError;
            }
          }
          break;
          case kSegmentTypeNetworkSegment: {
            NetworkSegmentSubtype subtype =
              GetPathNetworkSegmentSubtype(message);
            switch(subtype) {
              case kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds:
                if(kConnectionObjectTransportClassTriggerProductionTriggerCyclic
                   != ConnectionObjectGetTransportClassTriggerProductionTrigger(
                     connection_object) ) {
                  /* only non cyclic connections may have a production inhibit */
                  connection_object->production_inhibit_time = message[1];
                  message += 2;
                  remaining_path -= 2;
                } else {
                  *extended_error = connection_path_size - remaining_path; /*offset in 16Bit words where within the connection path the error happened*/
                  return kCipErrorPathSegmentError; /*status code for invalid segment type*/
                }
                break;
              default:
                OPENER_TRACE_ERR("Not allowed in connection manager");
                return kCipErrorPathSegmentError;
            }
          }
          break;

          default:
            OPENER_TRACE_WARN(
              "No data segment identifier found for the configuration data\n");
            *extended_error = connection_path_size - remaining_path; /*offset in 16Bit words where within the connection path the error happened*/
            return
              kConnectionManagerGeneralStatusPathSegmentErrorInUnconnectedSend;
        }
      }
    }
  }

  OPENER_TRACE_INFO("Resulting PIT value: %u\n",
                    connection_object->production_inhibit_time);
  /*save back the current position in the stream allowing followers to parse anything thats still there*/
  message_router_request->data = message;
  return kEipStatusOk;
}

void CloseConnection(CipConnectionObject *RESTRICT connection_object) {

  OPENER_TRACE_INFO("cipconnectionmanager: CloseConnection, trigger: %d \n",
  	ConnectionObjectGetTransportClassTriggerTransportClass(connection_object));

  if(kConnectionObjectTransportClassTriggerTransportClass3 !=
     ConnectionObjectGetTransportClassTriggerTransportClass(connection_object) )
  {
    /* only close the UDP connection for not class 3 connections */
    CloseUdpSocket(connection_object->socket[kUdpCommuncationDirectionConsuming]);
    connection_object->socket[kUdpCommuncationDirectionConsuming] =
      kEipInvalidSocket;
    CloseUdpSocket(connection_object->socket[kUdpCommuncationDirectionProducing]);
    connection_object->socket[kUdpCommuncationDirectionProducing] =
      kEipInvalidSocket;
  }
  RemoveFromActiveConnections(connection_object);
  ConnectionObjectInitializeEmpty(connection_object);

}

void AddNewActiveConnection(CipConnectionObject *const connection_object) {
  DoublyLinkedListInsertAtHead(&connection_list, connection_object);
  ConnectionObjectSetState(connection_object,
                           kConnectionObjectStateEstablished);
}

void RemoveFromActiveConnections(CipConnectionObject *const connection_object) {
  for(DoublyLinkedListNode *iterator = connection_list.first; iterator != NULL;
      iterator = iterator->next) {
    if(iterator->data == connection_object) {
      DoublyLinkedListRemoveNode(&connection_list, &iterator);
      return;
    }
  } OPENER_TRACE_ERR("Connection not found in active connection list\n");
}

EipBool8 IsConnectedOutputAssembly(const CipInstanceNum instance_number) {
  EipBool8 is_connected = false;

  DoublyLinkedListNode *node = connection_list.first;

  while(NULL != node) {
    CipConnectionObject *connection_object = (CipConnectionObject *) node->data;
    CipDword consumed_connection_point =
      connection_object->consumed_path.instance_id;
    if(instance_number == consumed_connection_point &&
       true == ConnectionObjectIsTypeIOConnection(connection_object) ) {
      is_connected = true;
      break;
    }
    node = node->next;
  }
  return is_connected;
}

EipStatus AddConnectableObject(const CipUdint class_code,
                               OpenConnectionFunction open_connection_function)
{
  EipStatus status = kEipStatusError;

  /*parsing is now finished all data is available and check now establish the connection */
  for(unsigned int i = 0; i < g_kNumberOfConnectableObjects; ++i) {
    if( (0 == g_connection_management_list[i].class_id) ||
        (class_code == g_connection_management_list[i].class_id) ) {
      g_connection_management_list[i].class_id = class_code;
      g_connection_management_list[i].open_connection_function =
        open_connection_function;
      status = kEipStatusOk;
      break;
    }
  }

  return status;
}

ConnectionManagementHandling *
GetConnectionManagementEntry(const EipUint32 class_id) {

  ConnectionManagementHandling *connection_management_entry = NULL;

  for(unsigned int i = 0; i < g_kNumberOfConnectableObjects; ++i) {
    if(class_id == g_connection_management_list[i].class_id) {
      connection_management_entry = &(g_connection_management_list[i]);
      break;
    }
  }
  return connection_management_entry;
}

EipStatus TriggerConnections(unsigned int output_assembly,
                             unsigned int input_assembly) {
  EipStatus status = kEipStatusError;

  DoublyLinkedListNode *node = connection_list.first;
  while(NULL != node) {
    CipConnectionObject *connection_object = node->data;
    if( (output_assembly == connection_object->consumed_path.instance_id) &&
        (input_assembly == connection_object->produced_path.instance_id) ) {
      if(
        kConnectionObjectTransportClassTriggerProductionTriggerApplicationObject
        == ConnectionObjectGetTransportClassTriggerProductionTrigger(
          connection_object) ) {
        /* produce at the next allowed occurrence */
        connection_object->transmission_trigger_timer =
          connection_object->production_inhibit_time;
        status = kEipStatusOk;
      }
      break;
    }
    node = node->next;
  }
  return status;
}

void CheckForTimedOutConnectionsAndCloseTCPConnections(
  const CipConnectionObject *const connection_object,
  CloseSessionFunction CloseSessions)
{

  DoublyLinkedListNode *search_node = connection_list.first;
  bool non_timed_out_connection_found = false;
  while(NULL != search_node) {
    CipConnectionObject *search_connection = search_node->data;
    if(ConnectionObjectEqualOriginator(connection_object,
                                       search_connection) &&
       connection_object != search_connection
       && kConnectionObjectStateTimedOut !=
       ConnectionObjectGetState(search_connection) ) {
      non_timed_out_connection_found = true;
      break;
    }
    search_node = search_node->next;
  }
  if(false == non_timed_out_connection_found) {
    CloseSessions(connection_object);
  }
}

void InitializeConnectionManagerData() {
  memset(g_connection_management_list,
         0,
         g_kNumberOfConnectableObjects * sizeof(ConnectionManagementHandling) );
  InitializeClass3ConnectionData();
  InitializeIoConnectionData();
}
