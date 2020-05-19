/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file udp_protocol.h
 *  @author Martin Melik Merkumians
 *  @brief Includes a basic set of operations for UDP header creation and checksum calculation
 *
 *  In order to send UDP packets from a specified source port, the UDP header creation has to
 *  be done by hand. This file specifies the interface for OpENer's UDP header management,
 *  creation, and checksum calculation
 */

#ifndef SRC_PORTS_UDP_PROTOCOL_H_
#define SRC_PORTS_UDP_PROTOCOL_H_

#include <stdint.h>
#include <stddef.h>
#ifdef OPENER_POSIX
#include <netinet/in.h>
#elif WIN32
typedef uint32_t in_addr_t;
#endif

#define OPENER_UDP_HEADER_LENGTH (8U)
static const size_t kUdpHeaderLength = OPENER_UDP_HEADER_LENGTH; /**< UDP header length in bytes */

/** @brief Representing the needed information for the UDP header
 *
 * This struct represents the UDP header information
 */
typedef struct {
  uint16_t source_port;   /**< UDP source port */
  uint16_t destination_port;   /**< UDP destination port */
  uint16_t packet_length;   /**< UDP packet length (data + header) */
  uint16_t checksum;   /**< UDP checksum */
} UDPHeader;

/** @brief Sets source port field
 *
 * @param header The UDP header struct instance
 * @param source_port Source port value to be set
 */
void UDPHeaderSetSourcePort(UDPHeader *const header,
                            const uint16_t source_port);

/** @brief Gets source port field
 *
 * @param header The header struct instance
 * @return The source port
 */
uint16_t UDPHeaderGetSourcePort(const UDPHeader *const header);

/** @brief Sets destination port field
 *
 * @param header The UDP header struct instance
 * @param destination_port Destination port value to be set
 */
void UDPHeaderSetDestinationPort(UDPHeader *const header,
                                 const uint16_t destination_port);

/** @brief Gets destination port field
 *
 * @param header The header struct instance
 * @return The destination port
 */
uint16_t UDPHeaderGetDestinationPort(const UDPHeader *const header);

/** @brief Sets packet length field
 *
 * @param header The UDP header struct instance
 * @param packet_length Length value to be set
 */
void UDPHeaderSetPacketLength(UDPHeader *const header,
                              const uint16_t packet_length);

/** @brief Gets packet length field
 *
 * @param header The header struct instance
 * @return The packet length
 */
uint16_t UDPHeaderGetPacketLength(const UDPHeader *const header);

/** @brief Sets checksum field
 *
 * @param header The UDP header struct instance
 * @param checksum Checksum value to be set
 */
void UDPHeaderSetChecksum(UDPHeader *const header,
                          const uint16_t checksum);

/** @brief Gets checksum field
 *
 * @param header The UDP header struct instance
 * @return The packet length
 */
uint16_t UDPHeaderGetChecksum(const UDPHeader *const header);

/** @brief Calculates the checksum based on the set UDP packet data and pseudo IP header
 *
 * @param udp_packet the UDP packet including the UDP header
 * @param udp_packet_length UPD packet length
 * @param source_addr The IP source address
 * @param destination_addr The IP destination address
 * @return The calculated checksum
 */
uint16_t UDPHeaderCalculateChecksum(const void *udp_packet,
                                    const size_t udp_packet_length,
                                    const in_addr_t source_addr,
                                    const in_addr_t destination_addr);

/** @brief Generate the UDP header in the message according to the header
 *
 * The function generates the UDP header according to the header struct
 * overwriting the first 8 bytes
 *
 * @param header The UDP header struct instance
 * @param message The message buffer
 */
void UDPHeaderGenerate(const UDPHeader *header,
                       char *message);

#endif /* SRC_PORTS_UDP_PROTOCOL_H_ */
