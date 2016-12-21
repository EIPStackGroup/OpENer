/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_SRC_CIP_CIPCONNECTIONOBJECT_H_
#define OPENER_SRC_CIP_CIPCONNECTIONOBJECT_H_

static const int kConnectionObjectClassId = 0x05;

typedef struct cip_connection_object CipConnectionObject;

extern const size_t CipConnectionObjectSize;

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
  kCipConnectionObjectTransportClassTriggerProductionTriggerApplicationObject =
    2
} CipConnectionObjectTransportClassTriggerProductionTrigger;

typedef enum {
  kCipConnectionObjectTransportClassTriggerClassInvalid = -1,
  kCipConnectionObjectTransportClassTriggerClass0 = 0,
  kCipConnectionObjectTransportClassTriggerClass1 = 1,
  kCipConnectionObjectTransportClassTriggerClass2 = 2,
  kCipConnectionObjectTransportClassTriggerClass3 = 3,
  kCipConnectionObjectTransportClassTriggerClass4 = 4,
  kCipConnectionObjectTransportClassTriggerClass5 = 5,
  kCipConnectionObjectTransportClassTriggerClass6 = 6
} CipConnectionObjectTransportClassTriggerClass;

typedef enum {
  kCipConnectionObjectWatchdogTimeoutActionTransitionToTimedOut = 0,
  kCipConnectionObjectWatchdogTimeoutActionAutoDelete = 1,
  kCipConnectionObjectWatchdogTimeoutActionAutoReset = 2,
  kCipConnectionObjectWatchdogTimeoutActionDeferredDelete = 3
} CipConnectionObjectWatchdogTimeoutAction;

CipConnectionObject *NewCipConnectionObject(CipConnectionObjectInstanceType type);

#endif /* OPENER_SRC_CIP_CIPCONNECTIONOBJECT_H_ */
