/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "cipconnectionobject.h"

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

ConnectionObjectState ConnectionObjectGetState(
  const CipConnectionObject *const connection_object) {
  switch(connection_object->state) {
    case CIP_CONNECTION_OBJECT_STATE_NON_EXISTENT: return
        kConnectionObjectStateNonExistent; break;
    case CIP_CONNECTION_OBJECT_STATE_CONFIGURING: return
        kConnectionObjectStateConfiguring; break;
    case CIP_CONNECTION_OBJECT_STATE_WAITING_FOR_CONNECTION_ID: return
        kConnectionObjectStateWaitingForConnectionID; break;
    case CIP_CONNECTION_OBJECT_STATE_ESTABLISHED: return
        kConnectionObjectStateEstablished; break;
    case CIP_CONNECTION_OBJECT_STATE_TIMEOUT: return
        kConnectionObjectStateTimedOut; break;
    case CIP_CONNECTION_OBJECT_STATE_DEFERRED_DELETE: return
        kConnectionObjectStateDeferredDelete; break;
    case CIP_CONNECTION_OBJECT_STATE_CLOSING: return
        kConnectionObjectStateClosing; break;
    default: return kConnectionObjectStateInvalid;
  }
}

ConnectionObjectInstanceType ConnectionObjectGetInstanceType(
  const CipConnectionObject *const connection_object) {
  switch(connection_object->instance_type) {
    case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_EXPLICIT_MESSAGING: return
        kConnectionObjectInstanceTypeExplicitMessaging; break;
    case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_IO: return
        kConnectionObjectInstanceTypeIO; break;
    case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_CIP_BRIDGED: return
        kConnectionObjectInstanceTypeCipBridged; break;
    default: return kConnectionObjectInstanceTypeInvalid;
  }
}

ConnectionObjectTransportClassTriggerDirection
ConnectionObjectGetTransportClassTriggerDirection(
  const CipConnectionObject *const connection_object) {
  const CipByte TransportClassTriggerDirectionMask = 0x80;
  return (connection_object->transport_class_trigger &
          TransportClassTriggerDirectionMask) ==
         TransportClassTriggerDirectionMask ?
         kConnectionObjectTransportClassTriggerDirectionServer
         : kConnectionObjectTransportClassTriggerDirectionClient;
}

ConnectionObjectTransportClassTriggerProductionTrigger
ConnectionObjectGetTransportClassTriggerProductionTrigger(
  const CipConnectionObject *const connection_object) {
  const CipByte kTransportClassTriggerProductionTriggerMask = 0x70;
  switch( (connection_object->transport_class_trigger) &
          kTransportClassTriggerProductionTriggerMask ) {
    case
      CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_CYCLIC:
      return kConnectionObjectTransportClassTriggerProductionTriggerCyclic;
      break;
    case
      CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_CHANGE_OF_STATE
      : return
        kConnectionObjectTransportClassTriggerProductionTriggerChangeOfState;
      break;
    case
      CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_PRODUCTION_TRIGGER_APPLICATION_OBJECT
      : return
        kConnectionObjectTransportClassTriggerProductionTriggerApplicationObject;
      break;
    default: return
        kConnectionObjectTransportClassTriggerProductionTriggerInvalid;
  }
}

ConnectionObjectTransportClassTriggerTransportClass
ConnectionObjectGetTransportClassTriggerTransportClass(
  const CipConnectionObject *const connection_object) {
  const CipByte kTransportClassTriggerTransportClassMask = 0x0F;
  switch( (connection_object->transport_class_trigger) &
          kTransportClassTriggerTransportClassMask ) {
    case CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_0:
      return kConnectionObjectTransportClassTriggerTransportClass0; break;
    case CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_1:
      return kConnectionObjectTransportClassTriggerTransportClass1; break;
    case CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_2:
      return kConnectionObjectTransportClassTriggerTransportClass2; break;
    case CIP_CONNECTION_OBJECT_TRANSPORT_CLASS_TRIGGER_TRANSPORT_CLASS_3:
      return kConnectionObjectTransportClassTriggerTransportClass3; break;
    default: return kConnectionObjectTransportClassTriggerTransportClassInvalid;
  }
}

CipUint ConnectionObjectGetProducedConnectionSize(
  const CipConnectionObject *const connection_object) {
  return connection_object->produced_connection_size;
}

void ConnectionObjectSetProducedConnectionSize(
  CipConnectionObject *const connection_object,
  const CipUint
  produced_connection_size) {
  connection_object->produced_connection_size = produced_connection_size;
}

CipUint ConnectionObjectGetConsumedConnectionSize(
  const CipConnectionObject *const connection_object) {
  return connection_object->consumed_connection_size;
}

void ConnectionObjectSetConsumedConnectionSize(
  CipConnectionObject *const connection_object,
  const CipUint
  consumed_connection_size) {
  connection_object->consumed_connection_size = consumed_connection_size;
}

CipUdint ConnectionObjectGetCipProducedConnectionID(
  const CipConnectionObject *const connection_object) {
  return connection_object->cip_produced_connection_id;
}

void ConnectionObjectSetCipProducedConnectionID(
  CipConnectionObject *const connection_object,
  const CipUdint
  cip_produced_connection_id) {
  connection_object->cip_produced_connection_id = cip_produced_connection_id;
}

CipUdint ConnectionObjectGetCipConsumedConnectionID(
  const CipConnectionObject *const connection_object) {
  return connection_object->cip_consumed_connection_id;
}

void ConnectionObjectSetCipConsumedConnectionID(
  CipConnectionObject *const connection_object,
  const CipUdint
  cip_consumed_connection_id) {
  connection_object->cip_consumed_connection_id = cip_consumed_connection_id;
}
