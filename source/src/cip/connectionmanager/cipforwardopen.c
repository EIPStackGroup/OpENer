/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "cipforwardopen.h"

#include "endianconv.h"

typedef struct cip_forward_open_data {
  CipByte priority_time_tick;
  CipUsint timeout_ticks;
  CipUdint originiator_to_target_network_connection_id;
  CipUdint target_to_originator_network_connection_id;
  CipUint connection_serial_number;
  CipUint originator_vendor_id;
  CipUdint originator_serial_number;
  CipUsint connection_timeout_multiplier;
  CipUdint originator_to_target_requested_packet_interval;
  CipWord originator_to_target_network_connection_parameters;
  CipUdint target_to_originator_requested_packet_interval;
  CipWord target_to_originator_network_connection_parameters;
  CipByte transport_class_and_trigger;
  CipUsint connection_path_size;
  const CipOctet *connection_path;
} CipForwardOpenData;

const size_t kCipForwardOpenDataSize = sizeof(CipForwardOpenData);

const CipOctet *CipForwardOpenGetForwardOpenDataFromMessage(
  CipForwardOpenData *const data, const CipOctet **const message) {
  const CipOctet **message_runner = message;
  data->priority_time_tick = GetByteFromMessage(message_runner);
  data->timeout_ticks = GetUsintFromMessage(message_runner);
  data->originiator_to_target_network_connection_id = GetUdintFromMessage(
    message_runner);
  data->target_to_originator_network_connection_id = GetUdintFromMessage(
    message_runner);
  data->connection_serial_number = GetUintFromMessage(message_runner);
  data->originator_vendor_id = GetUintFromMessage(message_runner);
  data->originator_serial_number = GetUdintFromMessage(message_runner);
  data->connection_timeout_multiplier = GetUsintFromMessage(message_runner);
  *message_runner += 3; //Move pointer over the reserved space
  data->originator_to_target_requested_packet_interval = GetUdintFromMessage(
    message_runner);
  data->originator_to_target_network_connection_parameters = GetWordFromMessage(
    message_runner);
  data->target_to_originator_requested_packet_interval = GetUdintFromMessage(
    message_runner);
  data->target_to_originator_network_connection_parameters = GetWordFromMessage(
    message_runner);
  data->transport_class_and_trigger = GetByteFromMessage(message_runner);
  data->connection_path_size = GetUsintFromMessage(message_runner);
  data->connection_path = *message_runner;
  *message_runner = *message_runner + data->connection_path_size *
                    sizeof(CipOctet);
  return *message_runner;
}

CipForwardOpenPriorityTickTime CipForwardOpenGetPriorityTickTime(
  CipForwardOpenData *forward_open_data) {
  const CipByte kPriorityMask = 0x10;
  if( kPriorityMask ==
      ( (forward_open_data->priority_time_tick) & kPriorityMask ) ) {
    return kCipForwardOpenPriorityTickTimeReserved;
  }
  else {
    return kCipForwardOpenPriorityTickTimeNormal;
  }
}

uint8_t CipForwardOpenGetTickTime(CipForwardOpenData *forward_open_data) {
  const CipByte kTickTimeMask = 0x0F;
  return (forward_open_data->priority_time_tick & kTickTimeMask);
}

CipUsint CipForwardOpenGetTimeoutTick(CipForwardOpenData *forward_open_data) {
  return forward_open_data->timeout_ticks;
}

MilliSeconds CipForwardOpenGetTimeoutInMilliseconds(
  CipForwardOpenData *forward_open_data) {
  return CipForwardOpenGetTimeoutTick(forward_open_data) * 1 <<
         CipForwardOpenGetTickTime(forward_open_data);
}

CipUdint CipForwardOpenGetOriginatorToTargetNetworkConnectionId(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->originiator_to_target_network_connection_id;
}

CipUdint CipForwardOpenGetTargetToOriginatorNetworkConnectionId(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->target_to_originator_network_connection_id;
}

CipUint CipForwardOpenGetConnectionSerialNumber(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->connection_serial_number;
}

CipUint CipForwardOpenGetOriginatorVendorId(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->originator_vendor_id;
}

CipUdint CipForwardOpenGetOriginatorSerialNumber(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->originator_serial_number;
}

uint16_t CipForwardOpenGetTimeoutMultiplier(
  CipForwardOpenData *forward_open_data) {
  if (8 > forward_open_data->connection_timeout_multiplier) {
    return 4 << forward_open_data->connection_timeout_multiplier;
  }

  return 0;
}

MicroSeconds CipForwardOpenGetOriginatorToTargetRequestedPacketInterval(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->originator_to_target_requested_packet_interval;
}

CipWord CipForwardOpenGetOriginatorToTargetNetworkConnectionParameters(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->originator_to_target_network_connection_parameters;
}

static bool GetRedundantOwner(const CipWord connection_parameters) {
  const CipWord kRedundantOwnerMask = 0x8000;
  if ( kRedundantOwnerMask == (kRedundantOwnerMask & connection_parameters) ) {
    return true;
  }
  return false;
}

bool CipForwardOpenGetOriginatorToTargetRedundantOwner(
  CipForwardOpenData *forward_open_data) {
  return GetRedundantOwner( CipForwardOpenGetOriginatorToTargetNetworkConnectionParameters(
                              forward_open_data) );
}

static CipForwardOpenConnectionType GetConnectionType(
  CipWord connection_parameters) {
#define NULL_CONNECTION_TYPE 0x0000
#define MULTICAST_CONNECTION_TYPE 0x2000
#define POINT_TO_POINT_CONNECTION_TYPE 0x4000

  const CipWord kConnectionTypeMask = 0x6000;
  const CipWord kConnectionType = kConnectionTypeMask & connection_parameters;

  CipForwardOpenConnectionType type = kCipForwardOpenConnectionTypeReserved;

  switch(kConnectionType) {
    case NULL_CONNECTION_TYPE: type = kCipForwardOpenConnectionTypeNull; break;
    case MULTICAST_CONNECTION_TYPE: type =
      kCipForwardOpenConnectionTypeMulticastConnection; break;
    case POINT_TO_POINT_CONNECTION_TYPE: type =
      kCipForwardOpenConnectionTypePointToPointConnection; break;
    default: type = kCipForwardOpenConnectionTypeReserved; break;
  }

  return type;

#undef NULL_CONNECTION_TYPE
#undef MULTICAST_CONNECTION_TYPE
#undef POINT_TO_POINT_CONNECTION_TYPE
}

CipForwardOpenConnectionType CipForwardOpenGetOriginatorToTargetConnectionType(
  CipForwardOpenData *forward_open_data) {
  return GetConnectionType( CipForwardOpenGetOriginatorToTargetNetworkConnectionParameters(
                              forward_open_data) );
}

static CipForwardOpenConnectionPriority GetConnectionPriority(
  const CipWord connection_parameters) {
#define LOW_PRIORITY 0x0000
#define HIGH_PRIORITY 0x0800
#define SCHEDULED_PRIORITY 0x1000
#define URGENT_PRIORITY 0x1800
  const CipWord kPriorityMask = 0x1800;
  const CipWord kPriority = kPriorityMask & connection_parameters;

  CipForwardOpenConnectionPriority priority =
    kCipForwardOpenConnectionPriorityLow;
  switch(kPriority) {
    case LOW_PRIORITY: priority = kCipForwardOpenConnectionPriorityLow; break;
    case HIGH_PRIORITY: priority = kCipForwardOpenConnectionPriorityHigh; break;
    case SCHEDULED_PRIORITY: priority =
      kCipForwardOpenConnectionPriorityScheduled; break;
    case URGENT_PRIORITY: priority = kCipForwardOpenConnectionPriorityUrgent;
      break;
  }
  return priority;
#undef LOW_PRIORITY
#undef HIGH_PRIORITY
#undef SCHEDULED_PRIORITY
#undef URGENT_PRIORITY
}

CipForwardOpenConnectionPriority
CipForwardOpenGetOriginatorToTargetConnectionPriority(
  CipForwardOpenData *forward_open_data) {
  return GetConnectionPriority( CipForwardOpenGetOriginatorToTargetNetworkConnectionParameters(
                                  forward_open_data) );

}

static CipForwardOpenConnectionSizeType GetConnectionSizeType(
  CipWord connection_parameters) {
#define CONNECTION_SIZE_TYPE_FIXED 0x0000
#define CONNECTION_SIZE_TYPE_VARIABLE 0x0200
  const CipWord kConnectionSizeTypeMask = 0x0200;
  const CipWord kConnectionSizeType = kConnectionSizeTypeMask &
                                      connection_parameters;

  CipForwardOpenConnectionSizeType type =
    kCipForwardOpenConnectionSizeTypeFixed;
  switch (kConnectionSizeType) {
    case CONNECTION_SIZE_TYPE_FIXED: type =
      kCipForwardOpenConnectionSizeTypeFixed; break;
    case CONNECTION_SIZE_TYPE_VARIABLE: type =
      kCipForwardOpenConnectionSizeTypeVariable; break;
  }

  return type;

#undef CONNECTION_SIZE_TYPE_FIXED
#undef CONNECTION_SIZE_TYPE_VARIABLE

}

CipForwardOpenConnectionSizeType
CipForwardOpenGetOriginatorToTargetConnectionSizeType(
  CipForwardOpenData *forward_open_data) {
  return GetConnectionSizeType( CipForwardOpenGetOriginatorToTargetNetworkConnectionParameters(
                                  forward_open_data) );
}

static uint16_t GetConnectionSize(CipWord connection_parameters) {
  const CipWord kConnectionSizeMask = 0x01FF;
  return kConnectionSizeMask & connection_parameters;
}

uint16_t CipForwardOpenGetOriginatorToTargetConnectionSize(
  CipForwardOpenData *forward_open_data) {
  return GetConnectionSize( CipForwardOpenGetOriginatorToTargetNetworkConnectionParameters(
                              forward_open_data) );
}

MicroSeconds CipForwardOpenGetTargetToOriginatorRequestedPacketInterval(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->target_to_originator_requested_packet_interval;
}

CipWord CipForwardOpenGetTargetToOriginatorNetworkConnectionParameters(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->target_to_originator_network_connection_parameters;
}

bool CipForwardOpenGetTargetToOriginatorRedundantOwner(
  CipForwardOpenData *forward_open_data) {
  return GetRedundantOwner( CipForwardOpenGetTargetToOriginatorNetworkConnectionParameters(
                              forward_open_data) );
}

CipForwardOpenConnectionType CipForwardOpenGetTargetToOriginatorConnectionType(
  CipForwardOpenData *forward_open_data) {
  return GetConnectionType( CipForwardOpenGetTargetToOriginatorNetworkConnectionParameters(
                              forward_open_data) );
}

CipForwardOpenConnectionPriority
CipForwardOpenGetTargetToOriginatorConnectionPriority(
  CipForwardOpenData *forward_open_data) {
  return GetConnectionPriority( CipForwardOpenGetTargetToOriginatorNetworkConnectionParameters(
                                  forward_open_data) );
}

CipForwardOpenConnectionSizeType
CipForwardOpenGetTargetToOriginatorConnectionSizeType(
  CipForwardOpenData *forward_open_data) {
  return GetConnectionSizeType( CipForwardOpenGetTargetToOriginatorNetworkConnectionParameters(
                                  forward_open_data) );
}

uint16_t CipForwardOpenGetTargetToOriginatorConnectionSize(
  CipForwardOpenData *forward_open_data) {
  return GetConnectionSize( CipForwardOpenGetTargetToOriginatorNetworkConnectionParameters(
                              forward_open_data) );
}

CipConnectionObjectTransportClassTriggerDirection
CipForwardOpenGetTransportClassTriggerDirection(
  CipForwardOpenData *forward_open_data) {
#define DIRECTION_CLIENT 0x00
#define DIRECTION_SERVER 0x80
  const CipByte kDirectionMask = 0x80;
  const CipByte kDirection = kDirectionMask &
                             forward_open_data->transport_class_and_trigger;

  CipConnectionObjectTransportClassTriggerDirection direction =
    kCipConnectionObjectTransportClassTriggerDirectionClient;

  switch(kDirection) {
    case DIRECTION_CLIENT: direction =
      kCipConnectionObjectTransportClassTriggerDirectionClient; break;
    case DIRECTION_SERVER: direction =
      kCipConnectionObjectTransportClassTriggerDirectionServer; break;
  }

  return direction;
#undef DIRECTION_CLIENT
#undef DIRECTION_SERVER
}

CipConnectionObjectTransportClassTriggerProductionTrigger
CipForwardOpenGetTransportClassTriggerProductionTrigger(
  CipForwardOpenData *forward_open_data) {
#define PRODUCTION_TRIGGER_CYCLIC 0x00
#define PRODUCTION_TRIGGER_CHANGE_OF_STATE 0x10
#define PRODUCTION_TRIGGER_APPLICATION_OBJECT 0x20

  const CipByte kProductionTriggerMask = 0x70;
  const CipByte kProductionTrigger = kProductionTriggerMask &
                                     forward_open_data->
                                     transport_class_and_trigger;
  CipConnectionObjectTransportClassTriggerProductionTrigger trigger =
    kCipConnectionObjectTransportClassTriggerProductionTriggerInvalid;

  switch(kProductionTrigger) {
    case PRODUCTION_TRIGGER_CYCLIC: trigger =
      kCipConnectionObjectTransportClassTriggerProductionTriggerCyclic;
      break;
    case PRODUCTION_TRIGGER_CHANGE_OF_STATE: trigger =
      kCipConnectionObjectTransportClassTriggerProductionTriggerChangeOfState;
      break;
    case PRODUCTION_TRIGGER_APPLICATION_OBJECT: trigger =
      kCipConnectionObjectTransportClassTriggerProductionTriggerApplicationObject;
      break;
    default: trigger =
      kCipConnectionObjectTransportClassTriggerProductionTriggerInvalid;
      break;
  }

  return trigger;

#undef PRODUCTION_TRIGGER_CYCLIC
#undef PRODUCTION_TRIGGER_CHANGE_OF_STATE
#undef PRODUCTION_TRIGGER_APPLICATION_OBJECT
}

CipConnectionObjectTransportClassTriggerClass
CipForwardOpenGetTransportClassTriggerClass(
  CipForwardOpenData *forward_open_data) {
#define TRANSPORT_CLASS_0 0x00
#define TRANSPORT_CLASS_1 0x01
#define TRANSPORT_CLASS_2 0x02
#define TRANSPORT_CLASS_3 0x03
  const CipByte kTransportClassMask = 0x0F;
  const CipByte kTransportClass = kTransportClassMask &
                                  forward_open_data->transport_class_and_trigger;

  CipConnectionObjectTransportClassTriggerClass transport_class =
    kCipConnectionObjectTransportClassTriggerClass0;
  switch(kTransportClass) {
    case TRANSPORT_CLASS_0: transport_class =
      kCipConnectionObjectTransportClassTriggerClass0; break;
    case TRANSPORT_CLASS_1: transport_class =
      kCipConnectionObjectTransportClassTriggerClass1; break;
    case TRANSPORT_CLASS_2: transport_class =
      kCipConnectionObjectTransportClassTriggerClass2; break;
    case TRANSPORT_CLASS_3: transport_class =
      kCipConnectionObjectTransportClassTriggerClass3; break;
  }

  return transport_class;

#undef TRANSPORT_CLASS_0
#undef TRANSPORT_CLASS_1
#undef TRANSPORT_CLASS_2
#undef TRANSPORT_CLASS_3
}

CipUsint CipForwardOpenGetConnectionPathSizeInWords(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->connection_path_size;
}

const CipOctet *CipForwardOpenGetConnectionPath(
  CipForwardOpenData *forward_open_data) {
  return forward_open_data->connection_path;
}
