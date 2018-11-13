/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "udp_protocol.h"

#ifdef WIN32
#include <winsock.h>
#endif

void UDPHeaderSetSourcePort(UDPHeader *const header,
                            const uint16_t source_port) {
  header->source_port = source_port;
}

uint16_t UDPHeaderGetSourcePort(const UDPHeader *const header) {
  return header->source_port;
}

void UDPHeaderSetDestinationPort(UDPHeader *const header,
                                 const uint16_t destination_port) {
  header->destination_port = destination_port;
}

uint16_t UDPHeaderGetDestinationPort(const UDPHeader *const header) {
  return header->destination_port;
}

void UDPHeaderSetPacketLength(UDPHeader *const header,
                              const uint16_t packet_length) {
  header->packet_length = packet_length;
}

uint16_t UDPHeaderGetPacketLength(const UDPHeader *const header) {
  return header->packet_length;
}

void UDPHeaderSetChecksum(UDPHeader *const header,
                          const uint16_t checksum) {
  header->checksum = checksum;
}

uint16_t UDPHeaderGetChecksum(const UDPHeader *const header) {
  return header->checksum;
}

void UDPHeaderGenerate(const UDPHeader *header,
                       char *message) {
  *( (uint16_t *)message ) = htons(UDPHeaderGetSourcePort(header) );
  message += 2;
  *( (uint16_t *)message ) = htons(UDPHeaderGetDestinationPort(header) );
  message += 2;
  *( (uint16_t *)message ) = htons(UDPHeaderGetPacketLength(header) );
  message += 2;
  *( (uint16_t *)message ) = htons(UDPHeaderGetChecksum(header) );
  message += 2;
}

uint16_t UDPHeaderCalculateChecksum(const void *udp_packet,
                                    const size_t udp_packet_length,
                                    const in_addr_t source_addr,
                                    const in_addr_t destination_addr) {
  const uint16_t *udp_packet_words = udp_packet;
  uint32_t checksum = 0;
  size_t length = udp_packet_length;

  // Process UDP packet
  while(length > 1) {
    checksum += *udp_packet_words++;
    if(checksum & 0x8000000) {
      checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }
    length -= 2;
  }

  if(0 != length % 2) {
    // Add padding if packet length is odd
    checksum += *( (uint8_t *)udp_packet_words );
  }

  //Process IP pseudo header
  uint16_t *source_addr_as_words = (void *)&source_addr;
  checksum += *source_addr_as_words + *(source_addr_as_words + 1);

  uint16_t *destination_addr_as_words = (void *)&destination_addr;
  checksum += *destination_addr_as_words + *(destination_addr_as_words + 1);

  checksum += htons(IPPROTO_UDP);
  checksum += htons(udp_packet_length);

  //Add the carries
  while(0 != checksum >> 16) {
    checksum = (checksum & 0xFFFF) + (checksum >> 16);
  }

  // Return one's complement
  return (uint16_t)(~checksum);
}
