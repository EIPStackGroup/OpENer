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
  kConnectionObjectStateInvalid = -1, /**< An invalid state, shall never occur! */
  kConnectionObjectStateNonExistent = 0, /**< Connection is non existent */
  kConnectionObjectStateConfiguring, /**< Waiting for both to be configured and to apply the configuration */
  kConnectionObjectStateWaitingForConnectionID, /**< Only used for device net */
  kConnectionObjectStateEstablished, /**< Connection is established */
  kConnectionObjectStateTimedOut, /**< Connection timed out - inactivity or watchdog timer expired */
  kConnectionObjectStateDeferredDelete, /**< Only used for device net */
  kConnectionObjectStateClosing /**< For CIP bridged connections - have to wait for a successful forward close */
} ConnectionObjectState;

typedef enum {
  kConnectionObjectInstanceTypeInvalid = -1, /**< Invalid instance type - shall never occur! */
  kConnectionObjectInstanceTypeExplicitMessaging = 0, /**< Connection is an explicit messaging connection */
  kConnectionObjectInstanceTypeIO, /**< Connection is an I/O connection */
  kConnectionObjectInstanceTypeCipBridged  /**< Connection is a bridged connection */
} ConnectionObjectInstanceType;

typedef enum {
  kConnectionObjectTransportClassTriggerDirectionClient = 0,  /**< Endpoint provides client behavior */
  kConnectionObjectTransportClassTriggerDirectionServer /**< Endpoint provides server behavior - production trigger bits are to be ignored */
} ConnectionObjectTransportClassTriggerDirection;

typedef enum {
  kConnectionObjectTransportClassTriggerProductionTriggerInvalid = -1,  /**< Invalid Production trigger - shall never occur! */
  kConnectionObjectTransportClassTriggerProductionTriggerCyclic = 0, /**< Transmission Trigger Timer trigger data production */
  kConnectionObjectTransportClassTriggerProductionTriggerChangeOfState, /**< Production is trigger when change-of-state is detected by the Application Object */
  kConnectionObjectTransportClassTriggerProductionTriggerApplicationObject /**< The Application Object decided when production is triggered */
} ConnectionObjectTransportClassTriggerProductionTrigger;

typedef enum {
  kConnectionObjectTransportClassTriggerTransportClassInvalid = -1, /**< Invalid Transport Class - shall never occur! */
  kConnectionObjectTransportClassTriggerTransportClass0 = 0, /**< Class 0 producing or consuming connection, based on Direction */
  kConnectionObjectTransportClassTriggerTransportClass1, /**< Class 1 producing or consuming connection, based on Direction */
  kConnectionObjectTransportClassTriggerTransportClass2, /**< Class 2 producing and consuming connection, Client starts producing */
  kConnectionObjectTransportClassTriggerTransportClass3 /**< Class 3 producing and consuming connection, Client starts producing */
  /* Higher transport classes not supported */
} ConnectionObjectTransportClassTriggerTransportClass;

typedef enum {
  kConnectionObjectWatchdogTimeoutActionInvalid = -1,       /**< Invalid Watchdog Timeout Action - shall never occur! */
  kConnectionObjectWatchdogTimeoutActionTransitionToTimedOut = 0,       /**< Default for I/O connections, invalid for Explicit Messaging */
  kConnectionObjectWatchdogTimeoutActionAutoDelete,       /**< Default for explicit connections */
  kConnectionObjectWatchdogTimeoutActionAutoReset,       /**< Invalid for explicit connections */
  kConnectionObjectWatchdogTimeoutActionDeferredDelete       /**< Only for Device Net, invalid for I/O connections */
} ConnectionObjectWatchdogTimeoutAction;

typedef struct cip_connection_object CipConnectionObject;

typedef EipStatus (*CipConnectionStateHandler)(CipConnectionObject *RESTRICT const connection_object, ConnectionObjectState new_state);

struct cip_connection_object {
  CipUsint state; /*< Attribute 1 */
  CipUsint instance_type; /*< Attribute 2 */
  CipByte transport_class_trigger; /*< Attribute 3 */
  /* Attribute 4-6 only for device net*/
  CipUint produced_connection_size; /*< Attribute 7 - Limits produced connection size - for explicit messaging 0xFFFF means no predefined limit */
  CipUint consumed_connection_size; /*< Attribute 8 - Limits produced connection size - for explicit messaging 0xFFFF means no predefined limit */
  CipUint expected_packet_rate; /*< Attribute 9 - Resolution in Milliseconds */
  CipUint cip_produced_connection_id; /*< Attribute 10 */
  CipUint cip_consumed_connection_id; /*< Attribute 11 */
  CipUsint watchdog_timeout_action; /*< Attribute 12 */
  CipUint produced_connection_path_length; /*< Attribute 13 */
  unsigned char *produced_connection_path; /*< Attribute 14 */
  CipUint consumed_connection_path_length; /*< Attribute 15 */
  unsigned char *consumed_connection_path; /*< Attribute 16 */
  CipUint production_inhibit_time; /*< Attribute 17 */
  CipUsint connection_timeout_multiplier; /*< Attribute 18 */
  /* Attribute 19 not supported as Connection Bind service not supported */

  /* End of CIP attributes */
  /* Start of needed non-object variables */
  CipUint requested_produced_connection_size;
  CipUint requested_consumed_connection_size;

  uint64_t transmission_trigger_timer;
  uint64_t inactivity_watchdog_timer;
  uint64_t production_inhibit_timer;

  CipUint connection_serial_number;
  CipUint originator_vendor_id;
  CipUdint originator_serial_number;

  CipUint sequence_count_producing;

  CipConnectionStateHandler current_state_handler;

};

/** @brief Array allocator
 *
 */
CipConnectionObject *CipConnectionObjectCreate(const CipOctet *message);

/** @brief Array deallocator
 *
 */
void CipConnectionObjectDelete(CipConnectionObject **connection_object);

void ConnectionObjectInitializeEmpty(
  CipConnectionObject *const connection_object);

void ConnectionObjectInitializeFromMessage(
  const CipOctet *message,
  CipConnectionObject *const
  connection_object);

ConnectionObjectState ConnectionObjectGetState(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetState(CipConnectionObject *const connection_object,
                              const ConnectionObjectState state);

ConnectionObjectInstanceType ConnectionObjectGetInstanceType(
  const CipConnectionObject *const connection_object);

ConnectionObjectTransportClassTriggerDirection
ConnectionObjectGetTransportClassTriggerDirection(
  const CipConnectionObject *const connection_object);

ConnectionObjectTransportClassTriggerProductionTrigger
ConnectionObjectGetTransportClassTriggerProductionTrigger(
  const CipConnectionObject *const connection_object);

ConnectionObjectTransportClassTriggerTransportClass
ConnectionObjectGetTransportClassTriggerTransportClass(
  const CipConnectionObject *const connection_object);

CipUint ConnectionObjectGetProducedConnectionSize(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetProducedConnectionSize(
  CipConnectionObject *const connection_object,
  const CipUint
  produced_connection_size);

CipUint ConnectionObjectGetConsumedConnectionSize(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetConsumedConnectionSize(
  CipConnectionObject *const connection_object,
  const CipUint
  consumed_connection_size);

CipUint ConnectionObjectGetExpectedPacketRate(
  const CipConnectionObject *const connection_object);

/**
 * @brief Sets the expected packet rate according to the rules of the CIP specification
 *
 * As this function sets the expected packet rate according to the rules of the CIP specification, it is not always
 * the exact value entered, but rounded up to the next serviceable increment, relative to the timer resolution
 */
void ConnectionObjectSetExpectedPacketRate(
  CipConnectionObject *const connection_object,
  CipUint expected_packet_rate);

CipUdint ConnectionObjectGetCipProducedConnectionID(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetCipProducedConnectionID(
  CipConnectionObject *const connection_object,
  const CipUdint
  cip_produced_connection_id);

CipUdint ConnectionObjectGetCipConsumedConnectionID(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetCipConsumedConnectionID(
  CipConnectionObject *const connection_object,
  const CipUdint
  cip_consumed_connection_id);

ConnectionObjectWatchdogTimeoutAction ConnectionObjectGetWatchdogTimeoutAction(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetWatchdogTimeoutAction(
  CipConnectionObject *const connection_object,
  const CipUsint
  watchdog_timeout_action);

CipUint ConnectionObjectGetProducedConnectionPathLength(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetProducedConnectionPathLength(
  CipConnectionObject *const connection_object,
  const CipUint
  produced_connection_path_length);

CipUint ConnectionObjectGetConsumedConnectionPathLength(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetConsumedConnectionPathLength(
  CipConnectionObject *const connection_object,
  const CipUint
  consumed_connection_path_length);

CipUint ConnectionObjectGetProductionInhibitTime(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetProductionInhibitTime(
  CipConnectionObject *const connection_object,
  const CipUint
  production_inhibit_time);

CipUsint ConnectionObjectGetConnectionTimeoutMultiplier(
  const CipConnectionObject *const connection_object);

void ConnectionObjectSetConnectionTimeoutMultiplier(
  CipConnectionObject *const connection_object,
  const CipUsint
  connection_timeout_multiplier);


#endif /* SRC_CIP_CIPCONNECTIONOBJECT_H_ */
