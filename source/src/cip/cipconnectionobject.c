/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @brief Valid values for the state attribute of the Connection Object */
typedef enum {
  kCipConnectionObjectStateNonExistent = 0, /**< Connection not yet instantiated */
  kCipConnectionObjectStateConfiguring = 1, /**< Waiting to be configured or waiting to be told to apply configuration */
  kCipConnectionObjectStateWaitingForConnectionId = 2, /**< Only used on DeviceNet */
  kCipConnectionObjectStateEstablished = 3, /**< Fully configured and successfully applied */
  kCipConnectionObjectStateTimedOut = 4, /**< Has been timed out (Inactivity/Watchdog) */
  kCipConnectionObjectStateDeferredDelete = 5, /**< Only used on DeviceNet */
  kCipConnectionObjectStateClosing = 6 /**< CIP bridge is waiting for successful Forward Close from target node */
} CipConnectionObjectState;

typedef enum {
  kCipConnectionObjectInstanceTypeExplicitMessaging = 0,
  kCipConnectionObjectInstanceTypeIo = 1,
  kCipConnectionObjectInstanceTypeCipBridged = 2
} CipConnectionObjectInstanceType;

typedef enum {
  kCipConnectionObjectTransportClassTriggerDirectionClient = 0,
  kCipConnectionObjectTransportClassTriggerDirectionServer = 1
} CipConnectionObjectTransportClassTriggerDirection;

typedef enum {
  kCipConnectionObjectTransportClassTriggerProductionTriggerCyclic = 0,
  kCipConnectionObjectTransportClassTriggerProductionTriggerChangeOfState = 1,
  kCipConnectionObjectTransportClassTriggerProductionTriggerApplicationObject = 2
} CipConnectionObjectTransportClassTriggerProductionTrigger;

typedef enum {
  kCipConnectionObjectTransportClassTriggerTransportClass0 = 0,
  kCipConnectionObjectTransportClassTriggerTransportClass1 = 1,
  kCipConnectionObjectTransportClassTriggerTransportClass2 = 2,
  kCipConnectionObjectTransportClassTriggerTransportClass3 = 3,
  kCipConnectionObjectTransportClassTriggerTransportClass4 = 4,
  kCipConnectionObjectTransportClassTriggerTransportClass5 = 5,
  kCipConnectionObjectTransportClassTriggerTransportClass6 = 6
} CipConnectionObjectTransportClassTriggerTransportClass;

typedef enum {
  kCipConnectionObjectWatchdogTimeoutActionTransitionToTimedOut = 0,
  kCipConnectionObjectWatchdogTimeoutActionAutoDelete = 1,
  kCipConnectionObjectWatchdogTimeoutActionAutoReset = 2,
  kCipConnectionObjectWatchdogTimeoutActionDeferredDelete = 3
} CipConnectionObjectWatchdogTimeoutAction;

typedef struct {
  CipUint size;
  CipUint *array;
} ConnectionBindingList;

typedef struct {
  CipUsint state; /**< Attribute 1: State of the object, see enum CipConnectionObjectState */
  CipUsint instance_type; /**< Attribute 2: I/O, explicit, or bridged, see enum CipConnectionObjectInstanceType */
  CipByte transport_class_trigger; /**< Attribute 3 */
  CipUint device_net_produced_connection_id; /**< Attribute 4: Only used on DeviceNet */
  CipUint device_net_consumed_connection_id; /**< Attribute 5: Only used on DeviceNet */
  CipUsint device_net_initial_comm_characteristics; /**< Attribute 6: Only used on DeviceNet */
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
  ConnectionBindingList connection_binding_list; /**< Attribute 19: */
};


