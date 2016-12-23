/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CONNECTIONMANAGER_CIPFORWARDOPEN_H_
#define SRC_CIP_CONNECTIONMANAGER_CIPFORWARDOPEN_H_

#include <stdbool.h>
#include <stddef.h>

#include "typedefs.h"
#include "cipconnectionobject.h"

typedef struct cip_forward_open_data CipForwardOpenData;
extern const size_t kCipForwardOpenDataSize;

typedef enum cip_forward_open_priority_tick_time {
  kCipForwardOpenPriorityTickTimeNormal,
  kCipForwardOpenPriorityTickTimeReserved
} CipForwardOpenPriorityTickTime;

/**
 * @brief Connection Type constants of the Forward Open service request
 *   Indicates either a
 * - Null Request
 * - Point-to-point connection request (unicast)
 * - Multicast connection request
 */
typedef enum cip_forward_open_connection_type {
  kCipForwardOpenConnectionTypeNull,
  kCipForwardOpenConnectionTypePointToPointConnection,
  kCipForwardOpenConnectionTypeMulticastConnection,
  kCipForwardOpenConnectionTypeReserved  /**< Reserved and therefore invalid type */
} CipForwardOpenConnectionType;

typedef enum cip_forward_open_connection_priority {
  kCipForwardOpenConnectionPriorityLow,
  kCipForwardOpenConnectionPriorityHigh,
  kCipForwardOpenConnectionPriorityScheduled,
  kCipForwardOpenConnectionPriorityUrgent
} CipForwardOpenConnectionPriority;

typedef enum cip_forward_open_connection_size_type {
  kCipForwardOpenConnectionSizeTypeFixed,
  kCipForwardOpenConnectionSizeTypeVariable
} CipForwardOpenConnectionSizeType;

const CipOctet *CipForwardOpenGetForwardOpenDataFromMessage(
  CipForwardOpenData *const data, const CipOctet **const message);

CipForwardOpenPriorityTickTime CipForwardOpenGetPriorityTickTime(
  CipForwardOpenData *forward_open_data);

uint8_t CipForwardOpenGetTickTime(CipForwardOpenData *forward_open_data);

CipUsint CipForwardOpenGetTimeoutTick(CipForwardOpenData *forward_open_data);

MilliSeconds CipForwardOpenGetTimeoutInMilliseconds(
  CipForwardOpenData *forward_open_data);

CipUdint CipForwardOpenGetOriginatorToTargetNetworkConnectionId(
  CipForwardOpenData *forward_open_data);

CipUdint CipForwardOpenGetTargetToOriginatorNetworkConnectionId(
  CipForwardOpenData *forward_open_data);

CipUint CipForwardOpenGetConnectionSerialNumber(
  CipForwardOpenData *forward_open_data);

CipUint CipForwardOpenGetOriginatorVendorId(
  CipForwardOpenData *forward_open_data);

CipUdint CipForwardOpenGetOriginatorSerialNumber(
  CipForwardOpenData *forward_open_data);

uint16_t CipForwardOpenGetTimeoutMultiplier(
  CipForwardOpenData *forward_open_data);

MicroSeconds CipForwardOpenGetOriginatorToTargetRequestedPacketInterval(
  CipForwardOpenData *forward_open_data);

CipWord CipForwardOpenGetOriginatorToTargetNetworkConnectionParameters(
  CipForwardOpenData *forward_open_data);

bool CipForwardOpenGetOriginatorToTargetRedundantOwner(
  CipForwardOpenData *forward_open_data);

CipForwardOpenConnectionType CipForwardOpenGetOriginatorToTargetConnectionType(
  CipForwardOpenData *forward_open_data);

CipForwardOpenConnectionPriority
CipForwardOpenGetOriginatorToTargetConnectionPriority(
  CipForwardOpenData *forward_open_data);

CipForwardOpenConnectionSizeType
CipForwardOpenGetOriginatorToTargetConnectionSizeType(
  CipForwardOpenData *forward_open_data);

uint16_t CipForwardOpenGetOriginatorToTargetConnectionSize(
  CipForwardOpenData *forward_open_data);

MicroSeconds CipForwardOpenGetTargetToOriginatorRequestedPacketInterval(
  CipForwardOpenData *forward_open_data);

CipWord CipForwardOpenGetTargetToOriginatorNetworkConnectionParameters(
  CipForwardOpenData *forward_open_data);

bool CipForwardOpenGetTargetToOriginatorRedundantOwner(
  CipForwardOpenData *forward_open_data);

CipForwardOpenConnectionType CipForwardOpenGetTargetToOriginatorConnectionType(
  CipForwardOpenData *forward_open_data);

CipForwardOpenConnectionPriority
CipForwardOpenGetTargetToOriginatorConnectionPriority(
  CipForwardOpenData *forward_open_data);

CipForwardOpenConnectionSizeType
CipForwardOpenGetTargetToOriginatorConnectionSizeType(
  CipForwardOpenData *forward_open_data);

uint16_t CipForwardOpenGetTargetToOriginatorConnectionSize(
  CipForwardOpenData *forward_open_data);

CipConnectionObjectTransportClassTriggerDirection
CipForwardOpenGetTransportClassTriggerDirection(
  CipForwardOpenData *forward_open_data);

CipConnectionObjectTransportClassTriggerProductionTrigger
CipForwardOpenGetTransportClassTriggerProductionTrigger(
  CipForwardOpenData *forward_open_data);

CipConnectionObjectTransportClassTriggerClass
CipForwardOpenGetTransportClassTriggerClass(
  CipForwardOpenData *forward_open_data);

CipUsint CipForwardOpenGetConnectionPathSizeInWords(
  CipForwardOpenData *forward_open_data);

const CipOctet *CipForwardOpenGetConnectionPath(
  CipForwardOpenData *forward_open_data);

#endif /* SRC_CIP_CONNECTIONMANAGER_CIPFORWARDOPEN_H_ */
