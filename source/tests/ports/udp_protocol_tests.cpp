/*******************************************************************************
 * Copyright (c) 2020, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "ciptypes.h"
#include "udp_protocol.h"
}

TEST_GROUP(UdpProtocol) {

};

TEST(UdpProtocol, SetSourcePort) {
  UDPHeader header = {0};
  UDPHeaderSetSourcePort(&header, 10);
  CHECK_EQUAL( 10, header.source_port );
}

TEST(UdpProtocol, GetSourcePort) {
  UDPHeader header = {0};
  header.source_port =  5643;
  CHECK_EQUAL( 5643, UDPHeaderGetSourcePort(&header) );
}

TEST(UdpProtocol, SetDestinationPort) {
  UDPHeader header = {0};
  header.destination_port = 1640;
  CHECK_EQUAL( 1640, header.destination_port );
}

TEST(UdpProtocol, GetDestinationPort) {
  UDPHeader header = {0};
  UDPHeaderSetDestinationPort(&header, 9824);
  CHECK_EQUAL( 9824, UDPHeaderGetDestinationPort(&header) );
}

TEST(UdpProtocol, SetPacketLength) {
  UDPHeader header = {0};
  UDPHeaderSetPacketLength(&header, 26814);
  CHECK_EQUAL( 26814, header.packet_length);
}

TEST(UdpProtocol, GetPacketLength) {
  UDPHeader header = {0};
  header.packet_length = 36521;
  CHECK_EQUAL( 36521, UDPHeaderGetPacketLength(&header) );
}

TEST(UdpProtocol, SetChecksum) {
  UDPHeader header = {0};
  UDPHeaderSetChecksum(&header, 0x8eaf);
  CHECK_EQUAL( 0x8eaf, header.checksum);
}

TEST(UdpProtocol, GetChecksum) {
  UDPHeader header = {0};
  header.checksum = 0xaf8e;
  CHECK_EQUAL( 0xaf8e, UDPHeaderGetChecksum(&header) );
}

TEST(UdpProtocol, HeaderGenerate) {
  char message[OPENER_UDP_HEADER_LENGTH] = {0};
  UDPHeader header = {0};
  header.source_port =  5643;
  header.destination_port = 1640;
  header.packet_length = 36521;
  header.checksum = 0xaf8e;
  UDPHeaderGenerate(&header, message);
  CHECK_EQUAL( htons(5643), *( (uint16_t *)message ) );
  CHECK_EQUAL( htons(1640), *( ( (uint16_t *)message ) + 1 ) );
  CHECK_EQUAL( htons(36521), *( ( (uint16_t *)message ) + 2 ) );
  CHECK_EQUAL( htons(0xaf8e), *( ( (uint16_t *)message ) + 3 ) );
}

TEST(UdpProtocol, CalculateChecksumOddLength) {
  char message[OPENER_UDP_HEADER_LENGTH + 13];
  memset(message, 0, OPENER_UDP_HEADER_LENGTH + 13);
  UDPHeader header = {0};
  header.source_port =  5643;
  header.destination_port = 1640;
  header.packet_length = OPENER_UDP_HEADER_LENGTH + 13;
  header.checksum = 0;
  UDPHeaderGenerate(&header, message);
  for(size_t i = kUdpHeaderLength; i < OPENER_UDP_HEADER_LENGTH + 13; i++) {
    message[i] = i;
  }

  in_addr_t source_addr = 0x0A000001;
  in_addr_t destination_addr = 0x0A000002;

  uint16_t checksum = UDPHeaderCalculateChecksum(message,
                                                 OPENER_UDP_HEADER_LENGTH + 13,
                                                 source_addr,
                                                 destination_addr);
  CHECK_EQUAL(0xD591, checksum); // Aquired via the function under test - correctness verified via Wireshark
}

TEST(UdpProtocol, CalculateChecksumEvenLength) {
  char message[OPENER_UDP_HEADER_LENGTH + 12];
  memset(message, 0, OPENER_UDP_HEADER_LENGTH + 12);
  UDPHeader header = {0};
  header.source_port =  5643;
  header.destination_port = 1640;
  header.packet_length = OPENER_UDP_HEADER_LENGTH + 12;
  header.checksum = 0;
  UDPHeaderGenerate(&header, message);
  for(size_t i = kUdpHeaderLength; i < OPENER_UDP_HEADER_LENGTH + 12; i++) {
    message[i] = i;
  }

  in_addr_t source_addr = 0x0A000001;
  in_addr_t destination_addr = 0x0A000002;

  uint16_t checksum = UDPHeaderCalculateChecksum(message,
                                                 OPENER_UDP_HEADER_LENGTH + 12,
                                                 source_addr,
                                                 destination_addr);
  CHECK_EQUAL(0xEB91, checksum); // Aquired via the function under test - correctness verified via Wireshark
}
