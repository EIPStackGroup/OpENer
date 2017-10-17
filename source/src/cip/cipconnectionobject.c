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

ConnectionObjectState GetConnectionObjectState(
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

ConnectionObjectInstanceType GetConnectionObjectInstanceType(
  const CipConnectionObject *const connection_object) {
	switch(connection_object->instance_type) {
	case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_EXPLICIT_MESSAGING: return kConnectionObjectInstanceTypeExplicitMessaging; break;
	case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_IO: return kConnectionObjectInstanceTypeIO; break;
	case CIP_CONNECTION_OBJECT_INSTANCE_TYPE_CIP_BRIDGED: return kConnectionObjectInstanceTypeCipBridged; break;
	default: return kConnectionObjectInstanceTypeInvalid;
	}

}

ConnectionObjectTransportClassTriggerDirection
GetConnectionObjectTransportClassTriggerDirection(
  const CipConnectionObject *const connection_object) {
  const CipByte TransportClassTriggerDirectionMask = 0x80;
  return (connection_object->transport_class_trigger &
          TransportClassTriggerDirectionMask) ==
         TransportClassTriggerDirectionMask ?
         kConnectionObjectTransportClassTriggerDirectionServer
         : kConnectionObjectTransportClassTriggerDirectionClient;
}
