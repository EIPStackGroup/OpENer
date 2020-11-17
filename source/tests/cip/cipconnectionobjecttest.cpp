/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "cipconnectionobject.h"

}

TEST_GROUP (CipConnectionObject) {

};

TEST(CipConnectionObject, InitializeEmpty) {
  CipConnectionObject connection_object;
  ConnectionObjectInitializeEmpty(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateNonExistent, connection_object.state);
  CHECK_EQUAL(kEipInvalidSocket, connection_object.socket[0]);
  CHECK_EQUAL(kEipInvalidSocket, connection_object.socket[1]);
}

/* Get State tests */
TEST(CipConnectionObject, GetStateNonExistent) {
  CipConnectionObject connection_object = {0};
  connection_object.state = 0;
  ConnectionObjectState state = ConnectionObjectGetState(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateNonExistent, state);
}

TEST(CipConnectionObject, GetStateConfiguring) {
  CipConnectionObject connection_object = {0};
  connection_object.state = 1;
  ConnectionObjectState state = ConnectionObjectGetState(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateConfiguring, state);
}

TEST(CipConnectionObject, GetStateWaitingForConnectionID) {
  CipConnectionObject connection_object = {0};
  connection_object.state = 2;
  ConnectionObjectState state = ConnectionObjectGetState(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateWaitingForConnectionID, state);
}

TEST(CipConnectionObject, GetStateEstablished) {
  CipConnectionObject connection_object = {0};
  connection_object.state = 3;
  ConnectionObjectState state = ConnectionObjectGetState(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateEstablished, state);
}

TEST(CipConnectionObject, GetStateTimedOut) {
  CipConnectionObject connection_object = {0};
  connection_object.state = 4;
  ConnectionObjectState state = ConnectionObjectGetState(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateTimedOut, state);
}

TEST(CipConnectionObject, GetStateDeferredDelete) {
  CipConnectionObject connection_object = {0};
  connection_object.state = 5;
  ConnectionObjectState state = ConnectionObjectGetState(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateDeferredDelete, state);
}

TEST(CipConnectionObject, GetStateClosing) {
  CipConnectionObject connection_object = {0};
  connection_object.state = 6;
  ConnectionObjectState state = ConnectionObjectGetState(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateClosing, state);
}

TEST(CipConnectionObject, GetStateInvalid) {
  CipConnectionObject connection_object = {0};
  connection_object.state = 7;
  ConnectionObjectState state = ConnectionObjectGetState(&connection_object);
  CHECK_EQUAL(kConnectionObjectStateInvalid, state);
}

/* Set state tests */
TEST(CipConnectionObject, SetStateNonExistent) {
  CipConnectionObject connection_object = {0};
  ConnectionObjectSetState(&connection_object,
                           kConnectionObjectStateNonExistent);
  CHECK_EQUAL(0, connection_object.state);
}

TEST(CipConnectionObject, SetStateConfiguring) {
  CipConnectionObject connection_object = {0};
  ConnectionObjectSetState(&connection_object,
                           kConnectionObjectStateConfiguring);
  CHECK_EQUAL(1, connection_object.state);
}

TEST(CipConnectionObject, SetStateWaitingForConnectionID) {
  CipConnectionObject connection_object = {0};
  ConnectionObjectSetState(&connection_object,
                           kConnectionObjectStateWaitingForConnectionID);
  CHECK_EQUAL(2, connection_object.state);
}

TEST(CipConnectionObject, SetStateEstablished) {
  CipConnectionObject connection_object = {0};
  ConnectionObjectSetState(&connection_object,
                           kConnectionObjectStateEstablished);
  CHECK_EQUAL(3, connection_object.state);
}

TEST(CipConnectionObject, SetStateTimedOut) {
  CipConnectionObject connection_object = {0};
  ConnectionObjectSetState(&connection_object, kConnectionObjectStateTimedOut);
  CHECK_EQUAL(4, connection_object.state);
}

TEST(CipConnectionObject, SetStateDeferredDelete) {
  CipConnectionObject connection_object = {0};
  ConnectionObjectSetState(&connection_object,
                           kConnectionObjectStateDeferredDelete);
  CHECK_EQUAL(5, connection_object.state);
}

TEST(CipConnectionObject, SetStateClosing) {
  CipConnectionObject connection_object = {0};
  ConnectionObjectSetState(&connection_object, kConnectionObjectStateClosing);
  CHECK_EQUAL(6, connection_object.state);
}

/* Get InstanceType tests */

TEST(CipConnectionObject, GetInstanceType) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type =
    kConnectionObjectInstanceTypeExplicitMessaging;
  CHECK_EQUAL(kConnectionObjectInstanceTypeExplicitMessaging,
              ConnectionObjectGetInstanceType(&connection_object) );
}

TEST(CipConnectionObject, InstanceTypeIExplicitMessaging) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type =
    kConnectionObjectInstanceTypeExplicitMessaging;
  CipUsint value = ConnectionObjectGetInstanceTypeForAttribute(
    &connection_object);
  CHECK_EQUAL(0, value);
}

TEST(CipConnectionObject, InstanceTypeIO) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type = kConnectionObjectInstanceTypeIO;
  CipUsint value = ConnectionObjectGetInstanceTypeForAttribute(
    &connection_object);
  CHECK_EQUAL(1, value);
}

TEST(CipConnectionObject, InstanceTypeIOExclusiveOwner) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type =
    kConnectionObjectInstanceTypeIOExclusiveOwner;
  CipUsint value = ConnectionObjectGetInstanceTypeForAttribute(
    &connection_object);
  CHECK_EQUAL(1, value);
}

TEST(CipConnectionObject, InstanceTypeIOInputOnly) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type = kConnectionObjectInstanceTypeIOInputOnly;
  CipUsint value = ConnectionObjectGetInstanceTypeForAttribute(
    &connection_object);
  CHECK_EQUAL(1, value);
}

TEST(CipConnectionObject, InstanceTypeIOListenOnly) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type = kConnectionObjectInstanceTypeIOListenOnly;
  CipUsint value = ConnectionObjectGetInstanceTypeForAttribute(
    &connection_object);
  CHECK_EQUAL(1, value);
}

TEST(CipConnectionObject, InstanceTypeCipBridged) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type = kConnectionObjectInstanceTypeCipBridged;
  CipUsint value = ConnectionObjectGetInstanceTypeForAttribute(
    &connection_object);
  CHECK_EQUAL(2, value);
}

TEST(CipConnectionObject, IsTypeNonLOIOConnectionTrue) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type = kConnectionObjectInstanceTypeIO;
  CHECK_EQUAL(true,
              ConnectionObjectIsTypeNonLOIOConnection(&connection_object) );
  connection_object.instance_type =
    kConnectionObjectInstanceTypeIOExclusiveOwner;
  CHECK_EQUAL(true,
              ConnectionObjectIsTypeNonLOIOConnection(&connection_object) );
  connection_object.instance_type = kConnectionObjectInstanceTypeIOInputOnly;
  CHECK_EQUAL(true,
              ConnectionObjectIsTypeNonLOIOConnection(&connection_object) );
}

TEST(CipConnectionObject, IsTypeNonLOIOConnectionFalse) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type = kConnectionObjectInstanceTypeIOListenOnly;
  CHECK_EQUAL(false,
              ConnectionObjectIsTypeNonLOIOConnection(&connection_object) );
}

TEST(CipConnectionObject, IsTypeIOConnectionTrue) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type = kConnectionObjectInstanceTypeIO;
  CHECK_EQUAL(true, ConnectionObjectIsTypeIOConnection(&connection_object) );
  connection_object.instance_type =
    kConnectionObjectInstanceTypeIOExclusiveOwner;
  CHECK_EQUAL(true, ConnectionObjectIsTypeIOConnection(&connection_object) );
  connection_object.instance_type = kConnectionObjectInstanceTypeIOInputOnly;
  CHECK_EQUAL(true, ConnectionObjectIsTypeIOConnection(&connection_object) );
  connection_object.instance_type = kConnectionObjectInstanceTypeIOListenOnly;
  CHECK_EQUAL(true, ConnectionObjectIsTypeIOConnection(&connection_object) );
}

TEST(CipConnectionObject, IsTypeIOConnectionFalse) {
  CipConnectionObject connection_object = {0};
  connection_object.instance_type = kConnectionObjectInstanceTypeInvalid;
  CHECK_EQUAL(false, ConnectionObjectIsTypeIOConnection(&connection_object) );
  connection_object.instance_type =
    kConnectionObjectInstanceTypeExplicitMessaging;
  CHECK_EQUAL(false, ConnectionObjectIsTypeIOConnection(&connection_object) );
  connection_object.instance_type = kConnectionObjectInstanceTypeCipBridged;
  CHECK_EQUAL(false, ConnectionObjectIsTypeIOConnection(&connection_object) );
}

TEST(CipConnectionObject, TransportClassTriggerDirectionServer) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 0x80;
  ConnectionObjectTransportClassTriggerDirection direction =
    ConnectionObjectGetTransportClassTriggerDirection(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerDirectionServer, direction);
}

TEST(CipConnectionObject, TransportClassTriggerDirectionClient) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 0x00;
  ConnectionObjectTransportClassTriggerDirection direction =
    ConnectionObjectGetTransportClassTriggerDirection(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerDirectionClient, direction);
}

TEST(CipConnectionObject, TransportClassTriggerProductionTriggerInvalid) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 3 << 4;
  ConnectionObjectTransportClassTriggerProductionTrigger production_trigger =
    ConnectionObjectGetTransportClassTriggerProductionTrigger(
      &connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerProductionTriggerInvalid,
              production_trigger);
}

TEST(CipConnectionObject, TransportClassTriggerProductionTriggerCyclic) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 0x00;
  ConnectionObjectTransportClassTriggerProductionTrigger production_trigger =
    ConnectionObjectGetTransportClassTriggerProductionTrigger(
      &connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerProductionTriggerCyclic,
              production_trigger);
}

TEST(CipConnectionObject,
     TransportClassTriggerProductionTriggerChangeOfState) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 1 << 4;
  ConnectionObjectTransportClassTriggerProductionTrigger production_trigger =
    ConnectionObjectGetTransportClassTriggerProductionTrigger(
      &connection_object);
  CHECK_EQUAL(
    kConnectionObjectTransportClassTriggerProductionTriggerChangeOfState,
    production_trigger);
}

TEST(CipConnectionObject,
     TransportClassTriggerProductionTriggerApplicationObject) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 2 << 4;
  ConnectionObjectTransportClassTriggerProductionTrigger production_trigger =
    ConnectionObjectGetTransportClassTriggerProductionTrigger(
      &connection_object);
  CHECK_EQUAL(
    kConnectionObjectTransportClassTriggerProductionTriggerApplicationObject,
    production_trigger);
}

TEST(CipConnectionObject, TransportClassTriggerClassInvalid) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 5;
  ConnectionObjectTransportClassTriggerTransportClass transport_class =
    ConnectionObjectGetTransportClassTriggerTransportClass(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerTransportClassInvalid,
              transport_class);
}

TEST(CipConnectionObject, TransportClassTriggerClass0) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 0;
  ConnectionObjectTransportClassTriggerTransportClass transport_class =
    ConnectionObjectGetTransportClassTriggerTransportClass(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerTransportClass0,
              transport_class);
}

TEST(CipConnectionObject, TransportClassTriggerClass1) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 1;
  ConnectionObjectTransportClassTriggerTransportClass transport_class =
    ConnectionObjectGetTransportClassTriggerTransportClass(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerTransportClass1,
              transport_class);
}

TEST(CipConnectionObject, TransportClassTriggerClass2) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 2;
  ConnectionObjectTransportClassTriggerTransportClass transport_class =
    ConnectionObjectGetTransportClassTriggerTransportClass(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerTransportClass2,
              transport_class);
}

TEST(CipConnectionObject, TransportClassTriggerClass3) {
  CipConnectionObject connection_object = {0};
  connection_object.transport_class_trigger = 3;
  ConnectionObjectTransportClassTriggerTransportClass transport_class =
    ConnectionObjectGetTransportClassTriggerTransportClass(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerTransportClass3,
              transport_class);
}

TEST(CipConnectionObject, ExpectedPacketRate) {
  CipConnectionObject connection_object = {0};
  connection_object.t_to_o_requested_packet_interval = 11 * 1000; // 11 ms in µs
  ConnectionObjectSetExpectedPacketRate(&connection_object);
  CipUint expected_packet_rate = ConnectionObjectGetExpectedPacketRate(
    &connection_object);
  CHECK_EQUAL(20, expected_packet_rate);
}

TEST(CipConnectionObject, ExpectedPacketRateBelowTimerResolution) {
  CipConnectionObject connection_object = {0};
  connection_object.t_to_o_requested_packet_interval = 9 * 1000; // 9 ms in µs
  ConnectionObjectSetExpectedPacketRate(&connection_object);
  CipUint expected_packet_rate = ConnectionObjectGetExpectedPacketRate(
    &connection_object);
  CHECK_EQUAL(10, expected_packet_rate);
}

TEST(CipConnectionObject, ExpectedPacketRateZero) {
  CipConnectionObject connection_object = {0};
  connection_object.t_to_o_requested_packet_interval = 0; // A value of zero needs to be maintained, as this deactivates timeout
  ConnectionObjectSetExpectedPacketRate(&connection_object);
  CipUint expected_packet_rate = ConnectionObjectGetExpectedPacketRate(
    &connection_object);
  CHECK_EQUAL(0, expected_packet_rate);
}

TEST(CipConnectionObject, ParseConnectionData) {
  CipConnectionObject connection_object = {0};
  const CipOctet message[] =
    "\x06\x28\x00\x00\x00\x00\x00\x00\x00\x00\x98\xff\x18\x00\x78\x56"
    "\x34\x12\x00\x00\x00\x00\xe0\x93\x04\x00\x02\x40\xa0\x86\x01\x00"
    "\x22\x20\x01\x04\x20\x04\x24\x97\x2c\x98\x2c\x64";

  const CipOctet *message_runner = (const CipOctet *)message;

  ConnectionObjectInitializeFromMessage(&message_runner, &connection_object);
  CipUdint o_to_t_network_connection_id =
    ConnectionObjectGetCipConsumedConnectionID(&connection_object);
  CHECK_EQUAL(0, o_to_t_network_connection_id);

  CipUdint t_to_o_network_connection_id =
    ConnectionObjectGetCipProducedConnectionID(&connection_object);
  CHECK_EQUAL(0, t_to_o_network_connection_id);

  CipUint connection_serial_number = ConnectionObjectGetConnectionSerialNumber(
    &connection_object);
  CHECK_EQUAL(0xff98, connection_serial_number);

  CipUint vendor_id = ConnectionObjectGetOriginatorVendorId(&connection_object);
  CHECK_EQUAL(0x0018, vendor_id);

  CipUdint originator_serial_number = ConnectionObjectGetOriginatorSerialNumber(
    &connection_object);
  CHECK_EQUAL(0x12345678, originator_serial_number);

  CipUsint connection_timeout_multiplier =
    ConnectionObjectGetConnectionTimeoutMultiplier(&connection_object);
  CHECK_EQUAL(0, connection_timeout_multiplier);

  CipUdint o_to_t_rpi_in_microseconds =
    ConnectionObjectGetOToTRequestedPacketInterval(&connection_object);
  CHECK_EQUAL(300000, o_to_t_rpi_in_microseconds);

  bool o_to_t_redundant_owner = ConnectionObjectIsOToTRedundantOwner(
    &connection_object);
  CHECK_EQUAL(false, o_to_t_redundant_owner);

  ConnectionObjectConnectionType o_to_t_connection_type =
    ConnectionObjectGetOToTConnectionType(&connection_object);
  CHECK_EQUAL(kConnectionObjectConnectionTypePointToPoint,
              o_to_t_connection_type);

  ConnectionObjectPriority o_to_t_priority = ConnectionObjectGetOToTPriority(
    &connection_object);
  CHECK_EQUAL(kConnectionObjectPriorityLow, o_to_t_priority);

  ConnectionObjectConnectionSizeType o_to_t_connection_size_type =
    ConnectionObjectGetOToTConnectionSizeType(&connection_object);
  CHECK_EQUAL(kConnectionObjectConnectionSizeTypeFixed,
              o_to_t_connection_size_type);

  size_t o_to_t_connection_size = ConnectionObjectGetOToTConnectionSize(
    &connection_object);
  CHECK_EQUAL(2ULL, o_to_t_connection_size);

  //T to O Tests

  CipUdint t_to_o_rpi_in_microseconds =
    ConnectionObjectGetTToORequestedPacketInterval(&connection_object);
  CHECK_EQUAL(100000, t_to_o_rpi_in_microseconds);

  bool t_to_o_redundant_owner = ConnectionObjectIsTToORedundantOwner(
    &connection_object);
  CHECK_EQUAL(false, t_to_o_redundant_owner);

  ConnectionObjectConnectionType t_to_o_connection_type =
    ConnectionObjectGetTToOConnectionType(&connection_object);
  CHECK_EQUAL(kConnectionObjectConnectionTypeMulticast, t_to_o_connection_type);

  ConnectionObjectPriority t_to_o_priority = ConnectionObjectGetTToOPriority(
    &connection_object);
  CHECK_EQUAL(kConnectionObjectPriorityLow, t_to_o_priority);

  ConnectionObjectConnectionSizeType t_to_o_connection_size_type =
    ConnectionObjectGetTToOConnectionSizeType(&connection_object);
  CHECK_EQUAL(kConnectionObjectConnectionSizeTypeFixed,
              t_to_o_connection_size_type);

  size_t t_to_o_connection_size = ConnectionObjectGetTToOConnectionSize(
    &connection_object);
  CHECK_EQUAL(34ULL, t_to_o_connection_size);

  ConnectionObjectTransportClassTriggerDirection direction =
    ConnectionObjectGetTransportClassTriggerDirection(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerDirectionClient, direction);

  ConnectionObjectTransportClassTriggerProductionTrigger trigger =
    ConnectionObjectGetTransportClassTriggerProductionTrigger(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerProductionTriggerCyclic,
              trigger);

  ConnectionObjectTransportClassTriggerTransportClass transport_class =
    ConnectionObjectGetTransportClassTriggerTransportClass(&connection_object);
  CHECK_EQUAL(kConnectionObjectTransportClassTriggerTransportClass1,
              transport_class);

}
