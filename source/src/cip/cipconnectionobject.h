/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_SRC_CIP_CIPCONNECTIONOBJECT_H_
#define OPENER_SRC_CIP_CIPCONNECTIONOBJECT_H_

static const int kConnectionObjectClassId = 0x05;

typedef struct ConnectionObject;

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
  kCipConnectionObjectTransportClassInvalid = -1,
  kCipConnectionObjectTransportClass0 = 0,
  kCipConnectionObjectTransportClass1 = 1,
  kCipConnectionObjectTransportClass2 = 2,
  kCipConnectionObjectTransportClass3 = 3,
  kCipConnectionObjectTransportClass4 = 4,
  kCipConnectionObjectTransportClass5 = 5,
  kCipConnectionObjectTransportClass6 = 6
} CipConnectionObjectTransportClass;

typedef enum {
  kCipConnectionObjectWatchdogTimeoutActionTransitionToTimedOut = 0,
  kCipConnectionObjectWatchdogTimeoutActionAutoDelete = 1,
  kCipConnectionObjectWatchdogTimeoutActionAutoReset = 2,
  kCipConnectionObjectWatchdogTimeoutActionDeferredDelete = 3
} CipConnectionObjectWatchdogTimeoutAction;

ConnectionObject *NewCipConnectionObject(CipConnectionObjectInstanceType type);

#endif /* OPENER_SRC_CIP_CIPCONNECTIONOBJECT_H_ */
