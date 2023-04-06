/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>

#include "cipconnectionobject.h"

#include "endianconv.h"
#include "trace.h"
#include "cipconnectionmanager.h"
#include "stdlib.h"

#define CIP_CONNECTION_OBJECT_STATE_NON_EXISTENT 0U
#define CIP_CONNECTION_OBJECT_STATE_CONFIGURING 1U
#define CIP_CONNECTION_OBJECT_STATE_WAITING_FOR_CONNECTION_ID 2U
#define CIP_CONNECTION_OBJECT_STATE_ESTABLISHED 3U
#define CIP_CONNECTION_OBJECT_STATE_TIMEOUT 4U
#define CIP_CONNECTION_OBJECT_STATE_DEFERRED_DELETE 5U
#define CIP_CONNECTION_OBJECT_STATE_CLOSING 6U

#define CIP_CONNECTION_OBJECT_INSTANCE_TYPE_EXPLICIT_MESSAGING 0
#define CIP_CONNECTION_OBJECT_INSTANCE_TYPE_IO 1
#define CIP_CONNECTION_OBJECT_INSTANCE_TYPE_CIP_BRIDGED 2

#define CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_CYCLIC ( \
    0 << 4)
#define \
  CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_CHANGE_OF_STATE ( \
    1 << 4)
#define \
  CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_APPLICATION_OBJECT ( \
    2 << 4)

#define CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_0 0
#define CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_1 1
#define CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_2 2
#define CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_3 3

#define CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_TRANSITION_TO_TIMED_OUT 0
#define CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_AUTO_DELETE 1
#define CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_AUTO_RESET 2
#define CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_DEFERRED_DELETE 3

#define CIP_CONNECTION_OBJECT_CONNECTION_TYPE_NULL 0
#define CIP_CONNECTION_OBJECT_CONNECTION_TYPE_MULTICAST 1
#define CIP_CONNECTION_OBJECT_CONNECTION_TYPE_POINT_TO_POINT 2

#define CIP_CONNECTION_OBJECT_PRIORITY_LOW 0
#define CIP_CONNECTION_OBJECT_PRIORITY_HIGH 1
#define CIP_CONNECTION_OBJECT_PRIORITY_SCHEDULED 2
#define CIP_CONNECTION_OBJECT_PRIORITY_URGENT 3

/** @brief Definition of the global connection list */
DoublyLinkedList connection_list;

/** @brief Array of the available explicit connections */
CipConnectionObject explicit_connection_object_pool[
  OPENER_CIP_NUM_EXPLICIT_CONNS];

DoublyLinkedListNode *CipConnectionObjectListArrayAllocator() {
  enum {
    kNodesAmount = OPENER_CIP_NUM_EXPLICIT_CONNS +
                   OPENER_CIP_NUM_INPUT_ONLY_CONNS +
                   OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS +
                   OPENER_CIP_NUM_LISTEN_ONLY_CONNS
  };
  static DoublyLinkedListNode nodes[kNodesAmount] = { 0 };
  for(size_t i = 0; i < kNodesAmount; ++i) {
    if(nodes[i].previous == NULL && nodes[i].next == NULL &&
       nodes[i].data == NULL) {
      return &nodes[i];
    }
  }
  return NULL;
}

void CipConnectionObjectListArrayFree(DoublyLinkedListNode **node) {

  if(NULL != node) {
    if(NULL != *node) {
      memset(*node, 0, sizeof(DoublyLinkedListNode) );
      *node = NULL;
    } else {
      OPENER_TRACE_ERR("Attempt to delete NULL pointer to node\n");
    }
  } else {
    OPENER_TRACE_ERR("Attempt to provide a NULL pointer to node pointer\n");
  }

}

/* Private methods declaration */
uint64_t ConnectionObjectCalculateRegularInactivityWatchdogTimerValue(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetInitialInactivityWatchdogTimerValue(
  CipConnectionObject *const connection_object);
/* End private methods declaration */

void ConnectionObjectInitializeEmpty(
  CipConnectionObject *const connection_object) {
  memset(connection_object, 0, sizeof(*connection_object) );
  ConnectionObjectSetState(connection_object,
                           kConnectionObjectStateNonExistent);
  connection_object->socket[0] = kEipInvalidSocket;
  connection_object->socket[1] = kEipInvalidSocket;
}

CipConnectionObject *CipConnectionObjectCreate(const CipOctet *message) {
  /* Suppress unused parameter compiler warning. */
  (void)message;

  assert(false); /* NOT IMPLEMENTED */
  return NULL;
}

void ConnectionObjectInitializeFromMessage(const CipOctet **message,
                                           CipConnectionObject *const connection_object)
{
  /* For unconnected send - can be ignored by targets, and is ignored here */
  CipByte priority_timetick = GetByteFromMessage(message);
  CipUsint timeout_ticks = GetUsintFromMessage(message);
  (void) priority_timetick; /* Silence unused variable compiler warning */
  (void) timeout_ticks;

  /* O_to_T Conn ID */
  ConnectionObjectSetCipConsumedConnectionID(connection_object,
                                             GetUdintFromMessage(message) );
  /* T_to_O Conn ID */
  ConnectionObjectSetCipProducedConnectionID(connection_object,
                                             GetUdintFromMessage(message) );

  ConnectionObjectSetConnectionSerialNumber(connection_object,
                                            GetUintFromMessage(message) );
  ConnectionObjectSetOriginatorVendorId(connection_object,
                                        GetUintFromMessage(message) );
  ConnectionObjectSetOriginatorSerialNumber(connection_object,
                                            GetUdintFromMessage(message) );

  ConnectionObjectSetConnectionNumber(connection_object);

  /* keep it to none existent till the setup is done this eases error handling and
   * the state changes within the forward open request can not be detected from
   * the application or from outside (reason we are single threaded)
   * */
  ConnectionObjectSetState(connection_object,
                           kConnectionObjectStateNonExistent);
  connection_object->sequence_count_producing = 0; /* set the sequence count to zero */

  ConnectionObjectSetConnectionTimeoutMultiplier(connection_object,
                                                 GetUsintFromMessage(message) );

  (*message) += 3; /* 3 bytes reserved */

  /* the requested packet interval parameter needs to be a multiple of TIMERTICK from the header file */
  OPENER_TRACE_INFO(
    "ForwardOpen: ConConnID %" PRIu32 ", ProdConnID %" PRIu32
    ", ConnSerNo %u\n",
    connection_object->cip_consumed_connection_id,
    connection_object->cip_produced_connection_id,
    connection_object->connection_serial_number);

  ConnectionObjectSetOToTRequestedPacketInterval(connection_object,
                                                 GetUdintFromMessage(message) );

  ConnectionObjectSetInitialInactivityWatchdogTimerValue(connection_object);

  if(connection_object->is_large_forward_open == true) {
    ConnectionObjectSetOToTNetworkConnectionParameters(connection_object,
                                                       GetDwordFromMessage(
                                                         message) );
  } else {
    ConnectionObjectSetOToTNetworkConnectionParameters(connection_object,
                                                       GetWordFromMessage(
                                                         message) );
  }

  ConnectionObjectSetTToORequestedPacketInterval(connection_object,
                                                 GetUdintFromMessage(message) );

  ConnectionObjectSetExpectedPacketRate(connection_object);

  if(connection_object->is_large_forward_open == true) {
    ConnectionObjectSetTToONetworkConnectionParameters(connection_object,
                                                       GetDwordFromMessage(
                                                         message) );
  } else {
    ConnectionObjectSetTToONetworkConnectionParameters(connection_object,
                                                       GetWordFromMessage(
                                                         message) );
  }

  connection_object->transport_class_trigger = GetByteFromMessage(message);
}

ConnectionObjectState ConnectionObjectGetState(
  const CipConnectionObject *const connection_object) {
  ConnectionObjectState new_state = kConnectionObjectStateInvalid;
  switch(connection_object->state) {
    case CIP_CONNECTION_OBJECT_STATE_NON_EXISTENT:
      new_state = kConnectionObjectStateNonExistent;
      break;
    case CIP_CONNECTION_OBJECT_STATE_CONFIGURING:
      new_state = kConnectionObjectStateConfiguring;
      break;
    case CIP_CONNECTION_OBJECT_STATE_WAITING_FOR_CONNECTION_ID:
      new_state = kConnectionObjectStateWaitingForConnectionID;
      break;
    case CIP_CONNECTION_OBJECT_STATE_ESTABLISHED:
      new_state = kConnectionObjectStateEstablished;
      break;
    case CIP_CONNECTION_OBJECT_STATE_TIMEOUT:
      new_state = kConnectionObjectStateTimedOut;
      break;
    case CIP_CONNECTION_OBJECT_STATE_DEFERRED_DELETE:
      new_state = kConnectionObjectStateDeferredDelete;
      break;
    case CIP_CONNECTION_OBJECT_STATE_CLOSING:
      new_state = kConnectionObjectStateClosing;
      break;
    default:
      new_state = kConnectionObjectStateInvalid;
      break;
  }
  return new_state;
}

void ConnectionObjectSetState(CipConnectionObject *const connection_object,
                              const ConnectionObjectState state) {
  switch(state) {
    case kConnectionObjectStateNonExistent:
      connection_object->state =
        CIP_CONNECTION_OBJECT_STATE_NON_EXISTENT;
      break;
    case kConnectionObjectStateConfiguring:
      connection_object->state =
        CIP_CONNECTION_OBJECT_STATE_CONFIGURING;
      break;
    case kConnectionObjectStateWaitingForConnectionID:
      connection_object->state =
        CIP_CONNECTION_OBJECT_STATE_WAITING_FOR_CONNECTION_ID;
      break;
    case kConnectionObjectStateEstablished:
      connection_object->state =
        CIP_CONNECTION_OBJECT_STATE_ESTABLISHED;
      break;
    case kConnectionObjectStateTimedOut:
      connection_object->state =
        CIP_CONNECTION_OBJECT_STATE_TIMEOUT;
      break;
    case kConnectionObjectStateDeferredDelete:
      connection_object->state =
        CIP_CONNECTION_OBJECT_STATE_DEFERRED_DELETE;
      break;
    case kConnectionObjectStateClosing:
      connection_object->state =
        CIP_CONNECTION_OBJECT_STATE_CLOSING;
      break;
    default:
      OPENER_ASSERT(false);/* Never get here */
      break;
  }
}

ConnectionObjectInstanceType ConnectionObjectGetInstanceType(
  const CipConnectionObject *const connection_object) {
  return connection_object->instance_type;
//  switch (connection_object->instance_type) {
//    case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_EXPLICIT_MESSAGING:
//      return kConnectionObjectInstanceTypeExplicitMessaging;
//      break;
//    case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_IO:
//      return kConnectionObjectInstanceTypeIO;
//      break;
//    case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_CIP_BRIDGED:
//      return kConnectionObjectInstanceTypeCipBridged;
//      break;
//    default:
//      return kConnectionObjectInstanceTypeInvalid;
//  }
}

void ConnectionObjectSetInstanceType(
  CipConnectionObject *const connection_object,
  const ConnectionObjectInstanceType instance_type) {
  connection_object->instance_type = (CipUsint) instance_type;
}

CipUsint ConnectionObjectGetInstanceTypeForAttribute(
  const CipConnectionObject *const connection_object) {
  CipUsint instance_type = kConnectionObjectInstanceTypeInvalid;
  switch(connection_object->instance_type) {
    case kConnectionObjectInstanceTypeExplicitMessaging:
      instance_type = CIP_CONNECTION_OBJECT_INSTANCE_TYPE_EXPLICIT_MESSAGING;
      break;
    case kConnectionObjectInstanceTypeIO:
    case kConnectionObjectInstanceTypeIOExclusiveOwner:
    case kConnectionObjectInstanceTypeIOInputOnly:
    case kConnectionObjectInstanceTypeIOListenOnly:
      instance_type = CIP_CONNECTION_OBJECT_INSTANCE_TYPE_IO;
      break;
    case kConnectionObjectInstanceTypeCipBridged:
      instance_type = CIP_CONNECTION_OBJECT_INSTANCE_TYPE_CIP_BRIDGED;
      break;
    default:
      OPENER_ASSERT(false);/* This is a fault case */
      instance_type = kConnectionObjectInstanceTypeInvalid;
      break;
  }
  return instance_type;
}

bool ConnectionObjectIsTypeNonLOIOConnection(
  const CipConnectionObject *const connection_object) {
  switch(connection_object->instance_type) {
    case kConnectionObjectInstanceTypeIO:
    case kConnectionObjectInstanceTypeIOExclusiveOwner:
    case kConnectionObjectInstanceTypeIOInputOnly:
      return true;
    default:
      return false;
  }
}

bool ConnectionObjectIsTypeIOConnection(
  const CipConnectionObject *const connection_object) {
  switch(connection_object->instance_type) {
    case kConnectionObjectInstanceTypeIO:
    case kConnectionObjectInstanceTypeIOExclusiveOwner:
    case kConnectionObjectInstanceTypeIOInputOnly:
    case kConnectionObjectInstanceTypeIOListenOnly:
      return true;
    default:
      return false;
  }
}

ConnectionObjectTransportClassTriggerDirection
ConnectionObjectGetTransportClassTriggerDirection(
  const CipConnectionObject *const connection_object) {
  const CipByte TransportClassTriggerDirectionMask = 0x80;
  return
    (connection_object->transport_class_trigger &
     TransportClassTriggerDirectionMask) == TransportClassTriggerDirectionMask ?
    kConnectionObjectTransportClassTriggerDirectionServer :
    kConnectionObjectTransportClassTriggerDirectionClient;
}

ConnectionObjectTransportClassTriggerProductionTrigger
ConnectionObjectGetTransportClassTriggerProductionTrigger(
  const CipConnectionObject *const connection_object) {
  const CipByte kTransportClassTriggerProductionTriggerMask = 0x70;

  ConnectionObjectTransportClassTriggerProductionTrigger production_trigger =
    kConnectionObjectTransportClassTriggerProductionTriggerInvalid;
  switch( (connection_object->transport_class_trigger) &
          kTransportClassTriggerProductionTriggerMask ) {
    case
      CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_CYCLIC:
      production_trigger =
        kConnectionObjectTransportClassTriggerProductionTriggerCyclic;
      break;
    case
      CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_CHANGE_OF_STATE
      :
      production_trigger =
        kConnectionObjectTransportClassTriggerProductionTriggerChangeOfState;
      break;
    case
      CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_APPLICATION_OBJECT
      :
      production_trigger =
        kConnectionObjectTransportClassTriggerProductionTriggerApplicationObject;
      break;
    default:
      production_trigger =
        kConnectionObjectTransportClassTriggerProductionTriggerInvalid;
      break;
  }
  return production_trigger;
}

ConnectionObjectTransportClassTriggerTransportClass
ConnectionObjectGetTransportClassTriggerTransportClass(
  const CipConnectionObject *const connection_object) {
  const CipByte kTransportClassTriggerTransportClassMask = 0x0F;

  ConnectionObjectTransportClassTriggerTransportClass transport_class_trigger =
    kConnectionObjectTransportClassTriggerTransportClassInvalid;
  switch( (connection_object->transport_class_trigger) &
          kTransportClassTriggerTransportClassMask ) {
    case CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_0:
      transport_class_trigger =
        kConnectionObjectTransportClassTriggerTransportClass0;
      break;
    case CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_1:
      transport_class_trigger =
        kConnectionObjectTransportClassTriggerTransportClass1;
      break;
    case CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_2:
      transport_class_trigger =
        kConnectionObjectTransportClassTriggerTransportClass2;
      break;
    case CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_3:
      transport_class_trigger =
        kConnectionObjectTransportClassTriggerTransportClass3;
      break;
    default:
      transport_class_trigger =
        kConnectionObjectTransportClassTriggerTransportClassInvalid;
  }
  return transport_class_trigger;
}

CipUint ConnectionObjectGetProducedConnectionSize(
  const CipConnectionObject *const connection_object) {
  return connection_object->produced_connection_size;
}

void ConnectionObjectSetProducedConnectionSize(
  CipConnectionObject *const connection_object,
  const CipUint produced_connection_size) {
  connection_object->produced_connection_size = produced_connection_size;
}

CipUint ConnectionObjectGetConsumedConnectionSize(
  const CipConnectionObject *const connection_object) {
  return connection_object->consumed_connection_size;
}

void ConnectionObjectSetConsumedConnectionSize(
  CipConnectionObject *const connection_object,
  const CipUint consumed_connection_size) {
  connection_object->consumed_connection_size = consumed_connection_size;
}

CipUint ConnectionObjectGetExpectedPacketRate(
  const CipConnectionObject *const connection_object) {
  return connection_object->expected_packet_rate;
}

CipUint ConnectionObjectGetRequestedPacketInterval(
  const CipConnectionObject *const connection_object) {
  CipUdint remainder_to_resolution =
    (connection_object->t_to_o_requested_packet_interval) %
    (kOpenerTimerTickInMilliSeconds * 1000);
  if(0 == remainder_to_resolution) { /* Value can be represented in multiples of the timer resolution */
    return (CipUint) (connection_object->t_to_o_requested_packet_interval /
                      1000);
  } else {
    return (CipUint) (connection_object->t_to_o_requested_packet_interval /
                      1000 - remainder_to_resolution / 1000);
  }
}

void ConnectionObjectSetExpectedPacketRate(
  CipConnectionObject *const connection_object) {
  CipUdint remainder_to_resolution =
    (connection_object->t_to_o_requested_packet_interval) %
    (kOpenerTimerTickInMilliSeconds * 1000);
  if(0 == remainder_to_resolution) { /* Value can be represented in multiples of the timer resolution */
    connection_object->expected_packet_rate =
      connection_object->t_to_o_requested_packet_interval / 1000;
  } else {
    connection_object->expected_packet_rate =
      connection_object->t_to_o_requested_packet_interval / 1000
      + ( (CipUdint)
          kOpenerTimerTickInMilliSeconds - remainder_to_resolution / 1000 );
  }
}

CipUdint ConnectionObjectGetCipProducedConnectionID(
  const CipConnectionObject *const connection_object) {
  return connection_object->cip_produced_connection_id;
}

void ConnectionObjectSetCipProducedConnectionID(
  CipConnectionObject *const connection_object,
  const CipUdint cip_produced_connection_id) {
  connection_object->cip_produced_connection_id = cip_produced_connection_id;
}

CipUdint ConnectionObjectGetCipConsumedConnectionID(
  const CipConnectionObject *const connection_object) {
  return connection_object->cip_consumed_connection_id;
}

void ConnectionObjectSetCipConsumedConnectionID(
  CipConnectionObject *const connection_object,
  const CipUdint cip_consumed_connection_id) {
  connection_object->cip_consumed_connection_id = cip_consumed_connection_id;
}

ConnectionObjectWatchdogTimeoutAction ConnectionObjectGetWatchdogTimeoutAction(
  const CipConnectionObject *const connection_object) {
  ConnectionObjectWatchdogTimeoutAction timeout_action =
    kConnectionObjectWatchdogTimeoutActionInvalid;
  switch(connection_object->watchdog_timeout_action) {
    case CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_TRANSITION_TO_TIMED_OUT:
      timeout_action =
        kConnectionObjectWatchdogTimeoutActionTransitionToTimedOut;
      break;
    case CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_AUTO_DELETE:
      timeout_action = kConnectionObjectWatchdogTimeoutActionAutoDelete;
      break;
    case CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_AUTO_RESET:
      timeout_action = kConnectionObjectWatchdogTimeoutActionAutoReset;
      break;
    case CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_DEFERRED_DELETE:
      timeout_action = kConnectionObjectWatchdogTimeoutActionDeferredDelete;
      break;
    default:
      timeout_action = kConnectionObjectWatchdogTimeoutActionInvalid;
      break;
  }
  return timeout_action;
}

void ConnectionObjectSetWatchdogTimeoutAction(
  CipConnectionObject *const connection_object,
  const CipUsint watchdog_timeout_action) {
  switch(watchdog_timeout_action) {
    case CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_TRANSITION_TO_TIMED_OUT:
      connection_object->watchdog_timeout_action =
        kConnectionObjectWatchdogTimeoutActionTransitionToTimedOut;
      break;
    case CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_AUTO_DELETE:
      connection_object->watchdog_timeout_action =
        kConnectionObjectWatchdogTimeoutActionAutoDelete;
      break;
    case CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_AUTO_RESET:
      connection_object->watchdog_timeout_action =
        kConnectionObjectWatchdogTimeoutActionAutoReset;
      break;
    case CIP_CONNECTION_OBJECT_WATCHDOG_TIMEOUT_ACTION_DEFERRED_DELETE:
      connection_object->watchdog_timeout_action =
        kConnectionObjectWatchdogTimeoutActionDeferredDelete;
      break;
    default:
      connection_object->watchdog_timeout_action =
        kConnectionObjectWatchdogTimeoutActionInvalid;
      break;
  }
}

CipUint ConnectionObjectGetProducedConnectionPathLength(
  const CipConnectionObject *const connection_object) {
  return connection_object->produced_connection_path_length;
}

void ConnectionObjectSetProducedConnectionPathLength(
  CipConnectionObject *const connection_object,
  const CipUint produced_connection_path_length) {
  connection_object->produced_connection_path_length =
    produced_connection_path_length;
}

CipUint ConnectionObjectGetConsumedConnectionPathLength(
  const CipConnectionObject *const connection_object) {
  return connection_object->consumed_connection_path_length;
}

void ConnectionObjectSetConsumedConnectionPathLength(
  CipConnectionObject *const connection_object,
  const CipUint consumed_connection_path_length) {
  connection_object->consumed_connection_path_length =
    consumed_connection_path_length;
}

CipUint ConnectionObjectGetProductionInhibitTime(
  const CipConnectionObject *const connection_object) {
  return connection_object->production_inhibit_time;
}

void ConnectionObjectSetProductionInhibitTime(
  CipConnectionObject *const connection_object,
  const CipUint production_inhibit_time) {
  connection_object->production_inhibit_time = production_inhibit_time;
}

/*setup the preconsumption timer: max(ConnectionTimeoutMultiplier * ExpectedPacketRate, 10s) */
void ConnectionObjectSetInitialInactivityWatchdogTimerValue(
  CipConnectionObject *const connection_object) {
  const uint64_t kMinimumInitialTimeoutValue = 10000;
  const uint64_t calculated_timeout_value =
    ConnectionObjectCalculateRegularInactivityWatchdogTimerValue(
      connection_object);
  connection_object->inactivity_watchdog_timer =
    (calculated_timeout_value >
     kMinimumInitialTimeoutValue) ? calculated_timeout_value :
    kMinimumInitialTimeoutValue;
}

void ConnectionObjectResetInactivityWatchdogTimerValue(
  CipConnectionObject *const connection_object) {
  connection_object->inactivity_watchdog_timer =
    ConnectionObjectCalculateRegularInactivityWatchdogTimerValue(
      connection_object);
}

void ConnectionObjectResetLastPackageInactivityTimerValue(
  CipConnectionObject *const connection_object) {
  connection_object->last_package_watchdog_timer =
    ConnectionObjectCalculateRegularInactivityWatchdogTimerValue(
      connection_object);
}

uint64_t ConnectionObjectCalculateRegularInactivityWatchdogTimerValue(
  const CipConnectionObject *const connection_object) {
  return ( ( (uint64_t)(connection_object->o_to_t_requested_packet_interval) /
             (uint64_t) 1000 ) <<
           (2 + connection_object->connection_timeout_multiplier) );
}

CipUint ConnectionObjectGetConnectionSerialNumber(
  const CipConnectionObject *const connection_object) {
  return connection_object->connection_serial_number;
}

void ConnectionObjectSetConnectionSerialNumber(
  CipConnectionObject *connection_object,
  const CipUint connection_serial_number) {
  connection_object->connection_serial_number = connection_serial_number;
}

CipUint ConnectionObjectGetOriginatorVendorId(
  const CipConnectionObject *const connection_object) {
  return connection_object->originator_vendor_id;
}

void ConnectionObjectSetOriginatorVendorId(
  CipConnectionObject *connection_object,
  const CipUint vendor_id) {
  connection_object->originator_vendor_id = vendor_id;
}

CipUdint ConnectionObjectGetOriginatorSerialNumber(
  const CipConnectionObject *const connection_object) {
  return connection_object->originator_serial_number;
}

void ConnectionObjectSetOriginatorSerialNumber(
  CipConnectionObject *connection_object,
  CipUdint originator_serial_number) {
  connection_object->originator_serial_number = originator_serial_number;
}

CipUdint ConnectionObjectGetConnectionlNumber(
  const CipConnectionObject *const connection_object) {
  return connection_object->connection_number;
}

void ConnectionObjectSetConnectionNumber(
  CipConnectionObject *connection_object) {
  connection_object->connection_number = GenerateRandomConnectionNumber();
}

CipUint GenerateRandomConnectionNumber(void) {
	CipUint rand_num = (CipUint)rand(); //TODO: update to random.c functions

	//search for existing connection_numbers
	DoublyLinkedListNode *iterator = connection_list.first;
	CipConnectionObject *search_connection_object = NULL;

	while (NULL != iterator) {
		search_connection_object = iterator->data;

		if ((search_connection_object->connection_number == rand_num)) {

			rand_num = GenerateRandomConnectionNumber();
		}
		iterator = iterator->next;
	}

	return rand_num;
}

CipUsint ConnectionObjectGetConnectionTimeoutMultiplier(
  const CipConnectionObject *const connection_object) {
  return connection_object->connection_timeout_multiplier;
}

void ConnectionObjectSetConnectionTimeoutMultiplier(
  CipConnectionObject *const connection_object,
  const CipUsint connection_timeout_multiplier) {
  connection_object->connection_timeout_multiplier =
    connection_timeout_multiplier;
}

CipUdint ConnectionObjectGetOToTRequestedPacketInterval(
  const CipConnectionObject *const connection_object) {
  return connection_object->o_to_t_requested_packet_interval;
}

void ConnectionObjectSetOToTRequestedPacketInterval(
  CipConnectionObject *connection_object,
  const CipUdint requested_packet_interval) {
  connection_object->o_to_t_requested_packet_interval =
    requested_packet_interval;
}

CipUdint ConnectionObjectGetTToORequestedPacketInterval(
  const CipConnectionObject *const connection_object) {
  return connection_object->t_to_o_requested_packet_interval;
}

void ConnectionObjectSetTToORequestedPacketInterval(
  CipConnectionObject *connection_object,
  const CipUdint requested_packet_interval) {
  connection_object->t_to_o_requested_packet_interval =
    requested_packet_interval;
}

void ConnectionObjectSetTToONetworkConnectionParameters(
  CipConnectionObject *connection_object,
  const CipDword connection_parameters) {
  connection_object->t_to_o_network_connection_parameters =
    connection_parameters;
}

void ConnectionObjectSetOToTNetworkConnectionParameters(
  CipConnectionObject *connection_object,
  const CipDword connection_parameters) {
  connection_object->o_to_t_network_connection_parameters =
    connection_parameters;
}

bool ConnectionObjectIsRedundantOwner(const CipDword connection_parameters,
                                      const CipBool is_lfo) {
  if(is_lfo) {
    return (connection_parameters & (1 << 31) );
  } else {
    return (connection_parameters & (1 << 15) );
  }
}

bool ConnectionObjectIsOToTRedundantOwner(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectIsRedundantOwner(
    connection_object->o_to_t_network_connection_parameters,
    connection_object->is_large_forward_open);
}

bool ConnectionObjectIsTToORedundantOwner(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectIsRedundantOwner(
    connection_object->t_to_o_network_connection_parameters,
    connection_object->is_large_forward_open);
}

ConnectionObjectConnectionType ConnectionObjectGetConnectionType(
  const CipDword connection_parameters,
  const CipBool is_lfo) {

  CipUsint connection_type;
  if(is_lfo) {
    connection_type = (connection_parameters & (3 << 29) ) >> 29;
  } else {
    connection_type = (connection_parameters & (3 << 13) ) >> 13;
  }

  switch(connection_type) {
    case CIP_CONNECTION_OBJECT_CONNECTION_TYPE_NULL:
      return kConnectionObjectConnectionTypeNull;
    case CIP_CONNECTION_OBJECT_CONNECTION_TYPE_MULTICAST:
      return kConnectionObjectConnectionTypeMulticast;
    case CIP_CONNECTION_OBJECT_CONNECTION_TYPE_POINT_TO_POINT:
      return kConnectionObjectConnectionTypePointToPoint;
    default:
      return kConnectionObjectConnectionTypeInvalid;
  }
}

ConnectionObjectConnectionType ConnectionObjectGetOToTConnectionType(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectGetConnectionType(
    connection_object->o_to_t_network_connection_parameters,
    connection_object->is_large_forward_open);
}

ConnectionObjectConnectionType ConnectionObjectGetTToOConnectionType(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectGetConnectionType(
    connection_object->t_to_o_network_connection_parameters,
    connection_object->is_large_forward_open);
}

ConnectionObjectPriority ConnectionObjectGetPriority(
  const CipDword connection_parameters,
  const CipBool is_lfo) {

  CipUsint priority;
  if(is_lfo) {
    priority = (connection_parameters & (3 << 26) ) >> 26;
  } else {
    priority = (connection_parameters & (3 << 10) ) >> 10;
  }

  ConnectionObjectPriority result;
  switch(priority) {
    case CIP_CONNECTION_OBJECT_PRIORITY_LOW:
      result = kConnectionObjectPriorityLow;
      break;
    case CIP_CONNECTION_OBJECT_PRIORITY_HIGH:
      result = kConnectionObjectPriorityHigh;
      break;
    case CIP_CONNECTION_OBJECT_PRIORITY_SCHEDULED:
      result = kConnectionObjectPriorityScheduled;
      break;
    case CIP_CONNECTION_OBJECT_PRIORITY_URGENT:
      result = kConnectionObjectPriorityUrgent;
      break;
    default:
      OPENER_ASSERT(false);/* Not possible to get here! */
      result = kConnectionObjectPriorityLow;
      break;
  }
  return result;
}

ConnectionObjectPriority ConnectionObjectGetOToTPriority(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectGetPriority(
    connection_object->o_to_t_network_connection_parameters,
    connection_object->is_large_forward_open);
}

ConnectionObjectPriority ConnectionObjectGetTToOPriority(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectGetPriority(
    connection_object->t_to_o_network_connection_parameters,
    connection_object->is_large_forward_open);
}

ConnectionObjectConnectionSizeType ConnectionObjectGetConnectionSizeType(
  const CipDword connection_parameters,
  const CipBool is_lfo) {

  bool connection_size_type;
  if(is_lfo) {
    connection_size_type = (connection_parameters & (1 << 25) );
  } else {
    connection_size_type = (connection_parameters & (1 << 9) );
  }

  if(connection_size_type) {
    return kConnectionObjectConnectionSizeTypeVariable;
  } else {
    return kConnectionObjectConnectionSizeTypeFixed;
  }
}

ConnectionObjectConnectionSizeType ConnectionObjectGetOToTConnectionSizeType(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectGetConnectionSizeType(
    connection_object->o_to_t_network_connection_parameters,
    connection_object->is_large_forward_open);
}

ConnectionObjectConnectionSizeType ConnectionObjectGetTToOConnectionSizeType(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectGetConnectionSizeType(
    connection_object->t_to_o_network_connection_parameters,
    connection_object->is_large_forward_open);
}

size_t ConnectionObjectGetConnectionSize(const CipDword connection_parameters,
                                         const CipBool is_lfo) {
  const CipDword kConnectionSizeMask = 0x000001FF;
  const CipDword kConnectionSizeMaskLFO = 0x0000FFFF;

  CipDword mask = kConnectionSizeMask;
  if(is_lfo) {
    mask = kConnectionSizeMaskLFO;
  }

  return connection_parameters & mask;
}

size_t ConnectionObjectGetOToTConnectionSize(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectGetConnectionSize(
    connection_object->o_to_t_network_connection_parameters,
    connection_object->is_large_forward_open);
}

size_t ConnectionObjectGetTToOConnectionSize(
  const CipConnectionObject *const connection_object) {
  return ConnectionObjectGetConnectionSize(
    connection_object->t_to_o_network_connection_parameters,
    connection_object->is_large_forward_open);
}

void ConnectionObjectDeepCopy(
  CipConnectionObject *RESTRICT destination,
  const CipConnectionObject *RESTRICT const source
  ) {
  memcpy( destination, source, sizeof(CipConnectionObject) );
}

void ConnectionObjectResetSequenceCounts(
  CipConnectionObject *const connection_object) {
  connection_object->eip_level_sequence_count_producing = 0;
  connection_object->sequence_count_producing = 0;
  connection_object->eip_level_sequence_count_consuming = 0;
  connection_object->eip_first_level_sequence_count_received = false;
  connection_object->sequence_count_consuming = 0;
}

void ConnectionObjectResetProductionInhibitTimer(
  CipConnectionObject *const connection_object) {
  connection_object->production_inhibit_timer =
    connection_object->production_inhibit_time;
}

void ConnectionObjectGeneralConfiguration(
  CipConnectionObject *const connection_object) {

  connection_object->socket[0] = kEipInvalidSocket;
  connection_object->socket[1] = kEipInvalidSocket;

  if(kConnectionObjectConnectionTypePointToPoint ==
     ConnectionObjectGetOToTConnectionType(connection_object) ) {
    /* if we have a point to point connection for the O to T direction
     * the target shall choose the connection ID.
     */
    ConnectionObjectSetCipConsumedConnectionID(connection_object,
                                               GetConnectionId() );
  }

  if(kConnectionObjectConnectionTypeMulticast ==
     ConnectionObjectGetTToOConnectionType(connection_object) ) {
    /* if we have a multi-cast connection for the T to O direction the
     * target shall choose the connection ID.
     */
    ConnectionObjectSetCipProducedConnectionID(connection_object,
                                               GetConnectionId() );
  }

  ConnectionObjectResetSequenceCounts(connection_object);

  ConnectionObjectSetWatchdogTimeoutAction(connection_object,
                                           kConnectionObjectWatchdogTimeoutActionInvalid);                    /* Correct value not know at this point */

  ConnectionObjectResetProductionInhibitTimer(connection_object);

  connection_object->transmission_trigger_timer = 0;
}

bool ConnectionObjectEqualOriginator(const CipConnectionObject *const object1,
                                     const CipConnectionObject *const object2) {
  if( (object1->originator_vendor_id == object2->originator_vendor_id) &&
      (object1->originator_serial_number ==
       object2->originator_serial_number) ) {
    return true;
  }
  return false;
}

bool EqualConnectionTriad(const CipConnectionObject *const object1,
                          const CipConnectionObject *const object2) {
  if( (object1->connection_serial_number ==
       object2->connection_serial_number) &&
      (object1->originator_vendor_id == object2->originator_vendor_id)
      && (object1->originator_serial_number ==
          object2->originator_serial_number) ) {
    return true;
  }
  return false;
}

bool CipConnectionObjectOriginatorHasSameIP(
  const CipConnectionObject *const connection_object,
  const struct sockaddr *const originator_address) {
  return ( (struct sockaddr_in *) originator_address )->sin_addr.s_addr ==
         connection_object->originator_address.sin_addr.s_addr;
}
