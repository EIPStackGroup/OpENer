/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "cipconnectionobject.h"

#include "typedefs.h"

typedef struct {
  CipUint size;
  CipUint *array;
} CipConnectionBindingList;

typedef struct {
  CipUsint state; /**< Attribute 1: State of the object, see enum CipConnectionObjectState */
  CipUsint instance_type; /**< Attribute 2: Defines instance type I/O, explicit, or bridged, see enum CipConnectionObjectInstanceType */
  CipByte transport_class_trigger; /**< Attribute 3 */
  /* CipUint device_net_produced_connection_id;  **< Attribute 4: Only used on DeviceNet */
  /* CipUint device_net_consumed_connection_id;  **< Attribute 5: Only used on DeviceNet */
  /* CipUsint device_net_initial_comm_characteristics; **< Attribute 6: Only used on DeviceNet */
  CipUint produced_connection_size; /**< Attribute 7: See Vol.1 3-4.4.7 */
  CipUint consumed_connection_size; /**< Attribute 8: See Vol.1 3-4.4.8 */
  CipUint expected_packet_rate; /**< Attribute 9: Resolution in milliseconds*/
  CipUdint cip_produced_connection_id; /**< Attribute 10: */
  CipUdint cip_consumed_connection_id; /**< Attribute 11: */
  CipUsint watchdog_timeout_action; /**< Attribute 12: */
  CipUint produced_connection_path_length; /**< Attribute 13: Number of bytes of the produced_connection_path attribute */
  CipEpath produced_connection_path; /**< Attribute 14: */
  CipUint consumed_connection_path_length; /**< Attribute 15: Number of bytes of the produced_connection_path attribute */
  CipEpath consumed_connection_path; /**< Attribute 16: */
  CipUint production_inhibit_time; /**< Attribute 17: */
  CipUsint connection_timeout_multiplier; /**< Attribute 18: */
  CipConnectionBindingList connection_binding_list; /**< Attribute 19: */
} CipConnectionObject;

CipConnectionObjectState ConnectionObjectGetState(const CipConnectionObject *restrict const connection_object) {
	return (CipConnectionObjectState) connection_object->state;
}

CipConnectionObjectInstanceType ConnectionObjectGetInstanceType(const CipConnectionObject *restrict const connection_object) {
	return (CipConnectionObjectInstanceType) connection_object->instance_type;
}

CipConnectionObjectTransportClassTriggerDirection GetTransportClassTriggerDirection(const CipConnectionObject *restrict const connection_object) {
	const CipByte direction_mask = 0x80; /* Last bit of the byte is the direction flag */
	CipByte direction = connection_object->transport_class_trigger & direction_mask;
	return (CipConnectionObjectTransportClassTriggerDirection)direction >> 7; /* Move to match enum values 0 or 1 */
}

CipConnectionObjectTransportClassTriggerProductionTrigger GetTransportClassTriggerProductionTrigger(const CipConnectionObject *restrict const connection_object) {
	const CipByte production_trigger_mask = 0x70; /* Bits 4-7 are representing the production trigger */
	CipByte production_trigger = connection_object->transport_class_trigger & production_trigger_mask;
	return (CipConnectionObjectTransportClassTriggerProductionTrigger) production_trigger >> 4;
}

CipConnectionObjectTransportClassTriggerTransportClass GetTransportClassTriggerTransportClass(const CipConnectionObject *restrict const connection_object) {
	const CipByte transport_class_mask = 0x0F;
	CipByte transport_class = connection_object->transport_class_trigger & transport_class_mask;
	return (CipConnectionObjectTransportClassTriggerTransportClass) transport_class;
}

CipUint ConnectionObjectGetProducedConnectionSize(const CipConnectionObject *restrict const connection_object) {
	return connection_object->produced_connection_size;
}

CipUint ConnectionObjectGetConsumedConnectionSize(const CipConnectionObject *restrict const connection_object) {
	return connection_object->consumed_connection_size;
}

CipUint ConnectionObjectGetExpectedPacketRate(const CipConnectionObject *restrict const connection_object) {
	return connection_object->expected_packet_rate;
}

CipUdint ConnectionObjectGetCipProducedConnectionId(const CipConnectionObject *restrict const connection_object) {
	return connection_object->cip_produced_connection_id;
}

CipUdint ConnectionObjectGetCipConsumedConnectionId(const CipConnectionObject *restrict const connection_object) {
	return connection_object->cip_consumed_connection_id;
}

CipConnectionObjectWatchdogTimeoutAction ConnectionObjectGetWatchdogTimeoutActionValue(const CipConnectionObject *restrict const connection_object) {
	return (CipConnectionObjectWatchdogTimeoutAction) connection_object->watchdog_timeout_action;
}

CipUint ConnectionObjectGetProducedConnectionPathLength(const CipConnectionObject *restrict const connection_object) {
	return connection_object->produced_connection_path_length;
}

CipEpath ConnectionObjectGetProducedConnectionPath(const CipConnectionObject *restrict const connection_object) {
	return connection_object->produced_connection_path;
}

CipUint ConnectionObjectGetConsumedConnectionPathLength(const CipConnectionObject *restrict const connection_object) {
	return connection_object->consumed_connection_path_length;
}

CipEpath ConnectionObjectGetConsumedConnectionPath(const CipConnectionObject *restrict const connection_object) {
	return connection_object->consumed_connection_path;
}

CipUint ConnectionObjectGetProductionInhibitTime(const CipConnectionObject *restrict const connection_object) {
	return connection_object->production_inhibit_time;
}

CipUsint ConnectionObjectGetConnectionTimeoutMultiplier(const CipConnectionObject *restrict const connection_object) {
	return connection_object->connection_timeout_multiplier;
}

CipConnectionBindingList ConnectionObjectGetConnectionBindingList(const CipConnectionObject *restrict const connection_object) {
	return connection_object->connection_binding_list;
}

EipStatus ConnectionObjectGetAttributeSingle() {

}
