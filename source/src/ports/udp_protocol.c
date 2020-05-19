/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <assert.h>
#include "udp_protocol.h"
#include "trace.h"
#include "opener_user_conf.h"

#ifdef WIN32
#include <winsock2.h>
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
  /* Checksum is calculated for 16-bit words */
  const uint16_t *udp_packet_words = udp_packet;
  uint_fast32_t checksum = 0; /**< Carry bit access is needed (17th bit) */

  uint16_t i = udp_packet_length;
  for(; i > 1; i = i - 2 ) {
    checksum +=  *udp_packet_words;
    udp_packet_words++;
  }

  OPENER_ASSERT( (0 == i) || (1 == i) ); /* data processed */

  if (i > 0) {
    checksum += (*( (uint8_t *)udp_packet_words ) << 8);
    i--;
  }
  OPENER_ASSERT(0 == i); /* data processed */

  const uint16_t *const source_addr_as_words =
    (const uint16_t *const )&source_addr;
  checksum += *source_addr_as_words + *(source_addr_as_words + 1);

  const uint16_t *const destination_addr_as_words =
    (const uint16_t *const )&destination_addr;
  checksum += *destination_addr_as_words + *(destination_addr_as_words + 1);
  checksum += htons(IPPROTO_UDP);
  checksum += htons(udp_packet_length);

  while(0xFFFF0000 & checksum) {
    checksum = (checksum & 0xFFFF) + (checksum >> 16);
  }

  // Return one's complement
  return (uint16_t)(~checksum);
}

