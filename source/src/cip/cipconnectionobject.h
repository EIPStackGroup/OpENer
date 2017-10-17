/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPCONNECTIONOBJECT_H_
#define SRC_CIP_CIPCONNECTIONOBJECT_H_

#include "typedefs.h"
#include "ciptypes.h"

#define CIP_CONNECTION_OBJECT_CODE 0x05

typedef enum {
  kConnectionObjectStateInvalid = -1,
  kConnectionObjectStateNonExistent = 0,
  kConnectionObjectStateConfiguring,
  kConnectionObjectStateWaitingForConnectionID,
  kConnectionObjectStateEstablished,
  kConnectionObjectStateTimedOut,
  kConnectionObjectStateDeferredDelete,
  kConnectionObjectStateClosing
} ConnectionObjectState;

typedef enum {
  kConnectionObjectInstanceTypeInvalid = -1,
  kConnectionObjectInstanceTypeExplicitMessaging = 0,
  kConnectionObjectInstanceTypeIO,
  kConnectionObjectInstanceTypeCipBridged
} ConnectionObjectInstanceType;

typedef enum {
  kConnectionObjectTransportClassTriggerDirectionClient = 0,
  kConnectionObjectTransportClassTriggerDirectionServer
} ConnectionObjectTransportClassTriggerDirection;

typedef enum {
  kConnectionObjectTransportClassTriggerProductionTriggerInvalid = -1,
  kConnectionObjectTransportClassTriggerProductionTriggerCyclic = 0,
  kConnectionObjectTransportClassTriggerProductionTriggerChangeOfState,
  kConnectionObjectTransportClassTriggerProductionTriggerApplicationObject
} ConnectionObjectTransportClassTriggerProductionTrigger;

typedef enum {
  kConnectionObjectTransportClassTriggerTransportClassInvalid = -1,
  kConnectionObjectTransportClassTriggerTransportClass0 = 0,
  kConnectionObjectTransportClassTriggerTransportClass1,
  kConnectionObjectTransportClassTriggerTransportClass2,
  kConnectionObjectTransportClassTriggerTransportClass3
} ConnectionObjectTransportClassTriggerTransportClass;

typedef struct cip_connection_object {
  CipUsint state;       //*< Attribute 1
  CipUsint instance_type;       //*< Attribute 2
  CipByte transport_class_trigger;       //*< Attribute 3
  /* Attribute 4-6 only for device net*/
  CipUint produced_connection_size;       //*< Attribute 7
  CipUint consumed_connection_size;       //*< Attribute 8
  CipUint expected_packet_rate;       //*< Attribute 9
  CipUint cip_produced_connection_id;       //*< Attribute 10
  CipUint cip_consumed_connection_id;       //*< Attribute 11
  CipUsint watchdog_timeout_action;       //*< Attribute 12
  CipUint produced_connection_path_length;       //*< Attribute 13
  CipEpath produced_connection_path;       //*< Attribute 14
  CipUint consumed_connection_path_length;       //*< Attribute 15
  CipEpath consumed_connection_path;       //*< Attribute 16
  CipUint production_inhibit_time;        //*< Attribute 17
  CipUsint connection_timeout_multiplier;        //*< Attribute 18
  /* Attribute 19 not supported as Connection Bind service not supported */

} CipConnectionObject;

ConnectionObjectState GetConnectionObjectState(
  const CipConnectionObject *const connection_object);

ConnectionObjectInstanceType GetConnectionObjectInstanceType(
  const CipConnectionObject *const connection_object);

ConnectionObjectTransportClassTriggerDirection
GetConnectionObjectTransportClassTriggerDirection(
  const CipConnectionObject *const connection_object);

#endif /* SRC_CIP_CIPCONNECTIONOBJECT_H_ */
