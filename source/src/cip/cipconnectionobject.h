/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPCONNECTIONOBJECT_H_
#define SRC_CIP_CIPCONNECTIONOBJECT_H_

#include "api/opener_api.h"
#include "cip/cipelectronickey.h"
#include "cip/cipepath.h"
#include "cip/ciptypes.h"
#include "core/typedefs.h"
#include "opener_user_conf.h"  // NOLINT(build/include_subdir)  // NOLINT(build/include_subdir)
#include "utils/doublylinkedlist.h"

#define CIP_CONNECTION_OBJECT_CODE 0x05

typedef enum {
  kConnectionObjectStateNonExistent = 0,  ///< Connection is non existent
  kConnectionObjectStateConfiguring,  ///< Waiting for both to be configured and
                                      ///< to apply the configuration
  kConnectionObjectStateWaitingForConnectionID,  ///< Only used for device net
  kConnectionObjectStateEstablished,             ///< Connection is established
  kConnectionObjectStateTimedOut,  ///< Connection timed out - inactivity or
                                   ///< watchdog timer expired
  kConnectionObjectStateDeferredDelete,  ///< Only used for device net
  kConnectionObjectStateClosing,  ///< For CIP bridged connections - have to
                                  ///< wait for a successful forward close
  kConnectionObjectStateInvalid   ///< The connection object is in an invalid
                                  ///< state
} ConnectionObjectState;

typedef enum {
  kConnectionObjectInstanceTypeInvalid =
    (CipUsint)(~0),  ///< Invalid instance type - shall never occur!
  kConnectionObjectInstanceTypeExplicitMessaging =
    0,  ///< Connection is an explicit messaging connection
  kConnectionObjectInstanceTypeIO,  ///< Connection is an I/O connection
  kConnectionObjectInstanceTypeIOExclusiveOwner,  ///< Also I/O connection, only
                                                  ///< for easy differentiation
  kConnectionObjectInstanceTypeIOInputOnly,   ///< Also I/O connection, only for
                                              ///< easy differentiation
  kConnectionObjectInstanceTypeIOListenOnly,  ///< Also I/O connection, only for
                                              ///< easy differentiation
  kConnectionObjectInstanceTypeCipBridged     ///< Connection is a bridged
                                              ///< connection
} ConnectionObjectInstanceType;

typedef enum {
  /// Endpoint provides client behavior
  kConnectionObjectTransportClassTriggerDirectionClient = 0,
  /// Endpoint provides server behavior - production trigger bits are to be
  /// ignored
  kConnectionObjectTransportClassTriggerDirectionServer
} ConnectionObjectTransportClassTriggerDirection;

typedef enum {
  /// Invalid Production trigger - shall never occur!
  kConnectionObjectTransportClassTriggerProductionTriggerInvalid = -1,
  /// Transmission Trigger Timer trigger data production
  kConnectionObjectTransportClassTriggerProductionTriggerCyclic = 0,
  /// Production is trigger when change-of-state is detected by the Application
  /// Object
  kConnectionObjectTransportClassTriggerProductionTriggerChangeOfState,
  /// The Application Object decided when production is triggered
  kConnectionObjectTransportClassTriggerProductionTriggerApplicationObject
} ConnectionObjectTransportClassTriggerProductionTrigger;

typedef enum {
  /// Invalid Transport Class - shall never occur!
  kConnectionObjectTransportClassTriggerTransportClassInvalid = -1,
  /// Class 0 producing or consuming connection, based on Direction
  kConnectionObjectTransportClassTriggerTransportClass0 = 0,
  /// Class 1 producing or consuming connection, based on Direction
  kConnectionObjectTransportClassTriggerTransportClass1,
  /// Class 2 producing and consuming connection, Client starts producing
  kConnectionObjectTransportClassTriggerTransportClass2,
  /// Class 3 producing and consuming connection, Client starts producing
  kConnectionObjectTransportClassTriggerTransportClass3
  /* Higher transport classes not supported */
} ConnectionObjectTransportClassTriggerTransportClass;

/** @brief Possible values for the watch dog time out action of a connection
 *
 * Only positive values allowed
 */
typedef enum {
  /// Default for I/O connections, invalid for Explicit Messaging
  kConnectionObjectWatchdogTimeoutActionTransitionToTimedOut = 0,
  /// Default for explicit connections
  kConnectionObjectWatchdogTimeoutActionAutoDelete,
  /// Invalid for explicit connections
  kConnectionObjectWatchdogTimeoutActionAutoReset,
  /// Only for Device Net, invalid for I/O  connections
  kConnectionObjectWatchdogTimeoutActionDeferredDelete,
  /// Invalid Watchdog Timeout Action - shall never occur!
  kConnectionObjectWatchdogTimeoutActionInvalid
} ConnectionObjectWatchdogTimeoutAction;

typedef enum {
  kConnectionObjectConnectionTypeNull = 0,
  kConnectionObjectConnectionTypeMulticast,
  kConnectionObjectConnectionTypePointToPoint,
  kConnectionObjectConnectionTypeInvalid
} ConnectionObjectConnectionType;

typedef enum {
  kConnectionObjectPriorityLow = 0,
  kConnectionObjectPriorityHigh,
  kConnectionObjectPriorityScheduled,
  kConnectionObjectPriorityUrgent,
  kConnectionObjectPriorityExplicit
} ConnectionObjectPriority;

typedef enum {
  kConnectionObjectConnectionSizeTypeFixed,
  kConnectionObjectConnectionSizeTypeVariable
} ConnectionObjectConnectionSizeType;

typedef enum {
  kConnectionObjectSocketTypeProducing = 0,
  kConnectionObjectSocketTypeConsuming = 1
} ConnectionObjectSocketType;

typedef struct cip_connection_object CipConnectionObject;

typedef EipStatus (*CipConnectionStateHandler)(
  CipConnectionObject* RESTRICT const connection_object,
  ConnectionObjectState new_state);

struct cip_connection_object {
  CipUsint state;                   ///< Attribute 1
  CipUsint instance_type;           ///< Attribute 2
  CipByte transport_class_trigger;  ///< Attribute 3
  /* Attribute 4-6 only for device net*/
  CipUint
    produced_connection_size;  ///< Attribute 7 - Limits produced connection
                               ///< size - for explicit messaging 0xFFFF
                               ///< means no predefined limit
  CipUint
    consumed_connection_size;    ///< Attribute 8 - Limits produced connection
                                 ///< ///< size - for explicit messaging 0xFFFF
                                 ///< ///< means no predefined limit
  CipUint expected_packet_rate;  ///< Attribute 9 - Resolution in Milliseconds
  CipUdint cip_produced_connection_id;      ///< Attribute 10
  CipUdint cip_consumed_connection_id;      ///< Attribute 11
  CipUsint watchdog_timeout_action;         ///< Attribute 12
  CipUint produced_connection_path_length;  ///< Attribute 13
  CipOctet* produced_connection_path;       ///< Attribute 14
  CipConnectionPathEpath produced_path;
  CipUint consumed_connection_path_length;  ///< Attribute 15
  CipOctet* consumed_connection_path;       ///< Attribute 16
  CipConnectionPathEpath consumed_path;
  CipUint production_inhibit_time;         ///< Attribute 17
  CipUsint connection_timeout_multiplier;  ///< Attribute 18
  /* Attribute 19 not supported as Connection Bind service not supported */

  /* End of CIP attributes */
  /* Start of needed non-object variables */
  CipElectronicKey
    electronic_key;  // TODO(MartinMelikMerkumians): Check if really needed

  CipConnectionPathEpath configuration_path;

  CipInstance* producing_instance;
  CipInstance* consuming_instance;

  CipUint requested_produced_connection_size;
  CipUint requested_consumed_connection_size;

  uint64_t transmission_trigger_timer;
  uint64_t inactivity_watchdog_timer;
  uint64_t last_package_watchdog_timer;
  uint64_t production_inhibit_timer;

  CipUint connection_serial_number;
  CipUint originator_vendor_id;
  CipUdint originator_serial_number;

  CipUint connection_number;

  CipUdint o_to_t_requested_packet_interval;
  CipDword o_to_t_network_connection_parameters;

  CipUdint t_to_o_requested_packet_interval;
  CipDword t_to_o_network_connection_parameters;

  CipUint sequence_count_producing;  ///< sequence Count for Class 1 Producing
                                     ///< Connections
  CipUint sequence_count_consuming;  ///< sequence Count for Class 1 Producing
                                     ///< Connections
  /// the EIP level sequence Count for Class 0/1 Producing Connections
  /// may have a different value than SequenceCountProducing
  EipUint32 eip_level_sequence_count_producing;
  /// the EIP level sequence Count for Class 0/1 Producing
  /// Connections may have a different value than SequenceCountProducing
  EipUint32 eip_level_sequence_count_consuming;
  /// False if eip_level_sequence_count_consuming hasn't been initialized
  /// with a sequence count yet, true otherwise
  CipBool eip_first_level_sequence_count_received;
  CipInt correct_originator_to_target_size;
  CipInt correct_target_to_originator_size;

  /* Sockets for consuming and producing connection */
  int socket[2];

  struct sockaddr_in remote_address;  ///< socket address for produce

  /// the address of the originator that established the connection. needed
  /// for scanning if the right packet is arriving
  struct sockaddr_in originator_address;

  /// The session handle ID via which the forward open was sent
  CipSessionHandle associated_encapsulation_session;

  /* pointers to connection handling functions */
  CipConnectionStateHandler current_state_handler;

  ConnectionCloseFunction connection_close_function;
  ConnectionTimeoutFunction connection_timeout_function;
  ConnectionSendDataFunction connection_send_data_function;
  ConnectionReceiveDataFunction connection_receive_data_function;

  ENIPMessage last_reply_sent;
  CipBool is_large_forward_open;
};

/** @brief Extern declaration of the global connection list */
extern DoublyLinkedList connection_list;

DoublyLinkedListNode* CipConnectionObjectListArrayAllocator();
void CipConnectionObjectListArrayFree(DoublyLinkedListNode** node);

/** @brief CIP Connection Object constructor - NOT IMPLEMENTED
 *
 */
CipConnectionObject* CipConnectionObjectCreate(const CipOctet* message);

/** @brief CIP Connection Object destructor - NOT IMPLEMENTED
 *
 */
void CipConnectionObjectDelete(CipConnectionObject** connection_object);

void ConnectionObjectInitializeEmpty(
  CipConnectionObject* const connection_object);

void ConnectionObjectInitializeFromMessage(
  const CipOctet** message, CipConnectionObject* const connection_object);

ConnectionObjectState ConnectionObjectGetState(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetState(CipConnectionObject* const connection_object,
                              const ConnectionObjectState state);

ConnectionObjectInstanceType ConnectionObjectGetInstanceType(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetInstanceType(
  CipConnectionObject* const connection_object,
  const ConnectionObjectInstanceType instance_type);

CipUsint ConnectionObjectGetInstanceTypeForAttribute(
  const CipConnectionObject* const connection_object);

ConnectionObjectTransportClassTriggerDirection
ConnectionObjectGetTransportClassTriggerDirection(
  const CipConnectionObject* const connection_object);

ConnectionObjectTransportClassTriggerProductionTrigger
ConnectionObjectGetTransportClassTriggerProductionTrigger(
  const CipConnectionObject* const connection_object);

ConnectionObjectTransportClassTriggerTransportClass
ConnectionObjectGetTransportClassTriggerTransportClass(
  const CipConnectionObject* const connection_object);

CipUint ConnectionObjectGetProducedConnectionSize(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetProducedConnectionSize(
  CipConnectionObject* const connection_object,
  const CipUint produced_connection_size);

CipUint ConnectionObjectGetConsumedConnectionSize(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetConsumedConnectionSize(
  CipConnectionObject* const connection_object,
  const CipUint consumed_connection_size);

CipUint ConnectionObjectGetExpectedPacketRate(
  const CipConnectionObject* const connection_object);

CipUint ConnectionObjectGetRequestedPacketInterval(
  const CipConnectionObject* const connection_object);

/**
 * @brief Sets the expected packet rate according to the rules of the CIP
 * specification
 *
 * As this function sets the expected packet rate according to the rules of the
 * CIP specification, it is not always the exact value entered, but rounded up
 * to the next serviceable increment, relative to the timer resolution
 *
 * @param connection_object The connection object for which the expected packet
 * rate is set
 */
void ConnectionObjectSetExpectedPacketRate(
  CipConnectionObject* const connection_object);

/** @brief Gets the CIP Produced Connection ID
 *
 * Return the CIP Produced Connection ID of the given connection object
 * @param connection_object The connection object from which the CIP Produced
 * Connection ID is retrieved
 * @return The CIP Produced Connection ID
 */
CipUdint ConnectionObjectGetCipProducedConnectionID(
  const CipConnectionObject* const connection_object);

/** @brief Sets the CIP Produced Connection ID
 *
 * Sets the CIP Produced Connection ID for the given connection object
 * @param connection_object The connection object from which the CIP Produced
 * Connection ID is retrieved
 * @param cip_produced_connection_id The CIP Produced Connection ID to set
 */
void ConnectionObjectSetCipProducedConnectionID(
  CipConnectionObject* const connection_object,
  const CipUdint cip_produced_connection_id);

/** @brief Get the CIP Consumed Connection ID
 *
 * Return the CIP Consumed Connection ID of the given connection object
 * @param connection_object The connection object from which the CIP Consumed
 * Connection ID is retrieved
 * @return The CIP Consumed Connection ID
 */
CipUdint ConnectionObjectGetCipConsumedConnectionID(
  const CipConnectionObject* const connection_object);

/** @brief Sets the CIP Consumed Connection ID
 *
 * Sets the CIP Consumed Connection ID for the given connection object
 * @param connection_object The connection object from which the CIP Consumed
 * Connection ID is retrieved
 * @param cip_consumed_connection_id The CIP Consumed Connection ID to set
 */
void ConnectionObjectSetCipConsumedConnectionID(
  CipConnectionObject* const connection_object,
  const CipUdint cip_consumed_connection_id);

ConnectionObjectWatchdogTimeoutAction ConnectionObjectGetWatchdogTimeoutAction(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetWatchdogTimeoutAction(
  CipConnectionObject* const connection_object,
  const CipUsint watchdog_timeout_action);

CipUint ConnectionObjectGetProducedConnectionPathLength(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetProducedConnectionPathLength(
  CipConnectionObject* const connection_object,
  const CipUint produced_connection_path_length);

CipUint ConnectionObjectGetConsumedConnectionPathLength(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetConsumedConnectionPathLength(
  CipConnectionObject* const connection_object,
  const CipUint consumed_connection_path_length);

CipUint ConnectionObjectGetProductionInhibitTime(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetProductionInhibitTime(
  CipConnectionObject* const connection_object,
  const CipUint production_inhibit_time);

CipUsint ConnectionObjectGetConnectionTimeoutMultiplier(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetConnectionTimeoutMultiplier(
  CipConnectionObject* const connection_object,
  const CipUsint connection_timeout_multiplier);

void ConnectionObjectResetInactivityWatchdogTimerValue(
  CipConnectionObject* const connection_object);

void ConnectionObjectResetLastPackageInactivityTimerValue(
  CipConnectionObject* const connection_object);

CipUint ConnectionObjectGetConnectionSerialNumber(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetConnectionSerialNumber(
  CipConnectionObject* connection_object,
  const CipUint connection_serial_number);

CipUint ConnectionObjectGetOriginatorVendorId(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetOriginatorVendorId(
  CipConnectionObject* connection_object, const CipUint vendor_id);

CipUdint ConnectionObjectGetOriginatorSerialNumber(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetOriginatorSerialNumber(
  CipConnectionObject* connection_object, CipUdint originator_serial_number);

void ConnectionObjectGetConnectionNumber(CipConnectionObject* connection_object,
                                         const CipUint connection_number);

void ConnectionObjectSetConnectionNumber(
  CipConnectionObject* connection_object);

CipUint GenerateRandomConnectionNumber(void);

CipUdint ConnectionObjectGetOToTRequestedPacketInterval(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetOToTRequestedPacketInterval(
  CipConnectionObject* connection_object,
  const CipUdint requested_packet_interval);

bool ConnectionObjectIsOToTRedundantOwner(
  const CipConnectionObject* const connection_object);

ConnectionObjectConnectionType ConnectionObjectGetOToTConnectionType(
  const CipConnectionObject* const connection_object);

ConnectionObjectPriority ConnectionObjectGetOToTPriority(
  const CipConnectionObject* const connection_object);

ConnectionObjectConnectionSizeType ConnectionObjectGetOToTConnectionSizeType(
  const CipConnectionObject* const connection_object);

size_t ConnectionObjectGetOToTConnectionSize(
  const CipConnectionObject* const connection_object);

/* T to O */
CipUdint ConnectionObjectGetTToORequestedPacketInterval(
  const CipConnectionObject* const connection_object);

void ConnectionObjectSetTToORequestedPacketInterval(
  CipConnectionObject* connection_object,
  const CipUdint requested_packet_interval);

void ConnectionObjectSetTToONetworkConnectionParameters(
  CipConnectionObject* connection_object, const CipDword connection_parameters);

void ConnectionObjectSetOToTNetworkConnectionParameters(
  CipConnectionObject* connection_object, const CipDword connection_parameters);

bool ConnectionObjectIsTToORedundantOwner(
  const CipConnectionObject* const connection_object);

ConnectionObjectConnectionType ConnectionObjectGetTToOConnectionType(
  const CipConnectionObject* const connection_object);

ConnectionObjectPriority ConnectionObjectGetTToOPriority(
  const CipConnectionObject* const connection_object);

ConnectionObjectConnectionSizeType ConnectionObjectGetTToOConnectionSizeType(
  const CipConnectionObject* const connection_object);

size_t ConnectionObjectGetTToOConnectionSize(
  const CipConnectionObject* const connection_object);

/** @brief Copy the given connection data from source to destination
 *
 * @param destination Destination of the copy operation
 * @param source Source of the copy operation
 */
void ConnectionObjectDeepCopy(CipConnectionObject* RESTRICT destination,
                              const CipConnectionObject* RESTRICT const source);

void ConnectionObjectResetProductionInhibitTimer(
  CipConnectionObject* const connection_object);

/** @brief Generate the ConnectionIDs and set the general configuration
 * parameter in the given connection object.
 *
 * @param connection_object pointer to the connection object that should be set
 * up.
 */
void ConnectionObjectGeneralConfiguration(
  CipConnectionObject* const connection_object);

bool ConnectionObjectIsTypeNonLOIOConnection(
  const CipConnectionObject* const connection_object);

bool ConnectionObjectIsTypeIOConnection(
  const CipConnectionObject* const connection_object);

bool ConnectionObjectEqualOriginator(const CipConnectionObject* const object1,
                                     const CipConnectionObject* const object2);

bool EqualConnectionTriad(const CipConnectionObject* const object1,
                          const CipConnectionObject* const object2);

bool CipConnectionObjectOriginatorHasSameIP(
  const CipConnectionObject* const connection_object,
  const struct sockaddr* const originator_address);

#endif  // SRC_CIP_CIPCONNECTIONOBJECT_H_
