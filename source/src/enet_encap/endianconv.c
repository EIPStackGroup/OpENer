/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "endianconv.h"

OpenerEndianess g_opener_platform_endianess = kOpenerEndianessUnknown;

/* THESE ROUTINES MODIFY THE BUFFER POINTER*/

/**
 *   @brief Reads EIP_UINT8 from *buffer and converts little endian to host.
 *   @param buffer pointer where data should be reed.
 *   @return EIP_UINT8 data value
 */
EipUint8 GetSintFromMessage(const EipUint8 **const buffer) {
  const unsigned char *const buffer_address = (unsigned char *) *buffer;
  EipUint8 data = buffer_address[0];
  *buffer += 1;
  return data;
}

CipByte GetByteFromMessage(const CipOctet **const buffer_address) {
  const CipOctet *buffer = *buffer_address;
  CipByte data = buffer[0];
  *buffer_address += 1;
  return data;
}

CipUsint GetUsintFromMessage(const CipOctet **const buffer_address) {
  const CipOctet *buffer = *buffer_address;
  CipUsint data = buffer[0];
  *buffer_address += 1;
  return data;
}

/* little-endian-to-host unsigned 16 bit*/

/**
 *   @brief Reads EIP_UINT16 from *buffer and converts little endian to host.
 *   @param buffer pointer where data should be reed.
 *   @return EIP_UINT16 data value
 */
EipUint16 GetIntFromMessage(const EipUint8 **const buffer) {
  const unsigned char *const buffer_address = (unsigned char *) *buffer;
  EipUint16 data = buffer_address[0] | buffer_address[1] << 8;
  *buffer += 2;
  return data;
}

CipUint GetUintFromMessage(const CipOctet **const buffer_address) {
  const CipOctet *buffer = *buffer_address;
  EipUint16 data = buffer[0] | buffer[1] << 8;
  *buffer_address += 2;
  return data;
}

CipWord GetWordFromMessage(const CipOctet **const buffer_address) {
  const CipOctet *buffer = *buffer_address;
  EipUint16 data = buffer[0] | buffer[1] << 8;
  *buffer_address += 2;
  return data;
}

/**
 *   @brief Reads EIP_UINT32 from *buffer and converts little endian to host.
 *   @param buffer pointer where data should be reed.
 *   @return EIP_UNÍT32 value
 */
EipUint32 GetDintFromMessage(const EipUint8 **const buffer) {
  const unsigned char *p = (unsigned char *) *buffer;
  EipUint32 data = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
  *buffer += 4;
  return data;
}

CipUdint GetUdintFromMessage(const CipOctet **const buffer_address) {
  const CipOctet *buffer = *buffer_address;
  CipUdint data = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] <<
                  24;
  *buffer_address += 4;
  return data;
}

/**
 * @brief converts UINT8 data from host to little endian an writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
int AddSintToMessage(const EipUint8 data,
                     EipUint8 **const buffer) {
  unsigned char *p = (unsigned char *) *buffer;

  p[0] = (unsigned char) data;
  *buffer += 1;
  return 1;
}

/**
 * @brief converts UINT16 data from host to little endian an writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
int AddIntToMessage(const EipUint16 data,
                    EipUint8 **const buffer) {
  unsigned char *p = (unsigned char *) *buffer;

  p[0] = (unsigned char) data;
  p[1] = (unsigned char) (data >> 8);
  *buffer += 2;
  return 2;
}

/**
 * @brief Converts UINT32 data from host to little endian and writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
int AddDintToMessage(const EipUint32 data,
                     EipUint8 **const buffer) {
  unsigned char *p = (unsigned char *) *buffer;

  p[0] = (unsigned char) data;
  p[1] = (unsigned char) (data >> 8);
  p[2] = (unsigned char) (data >> 16);
  p[3] = (unsigned char) (data >> 24);
  *buffer += 4;

  return 4;
}

#ifdef OPENER_SUPPORT_64BIT_DATATYPES

/**
 *   @brief Reads EipUint64 from *pa_buf and converts little endian to host.
 *   @param pa_buf pointer where data should be reed.
 *   @return EipUint64 value
 */
EipUint64 GetLintFromMessage(const EipUint8 **const buffer) {
  const EipUint8 *buffer_address = *buffer;
  EipUint64 data = ( ( ( (EipUint64) buffer_address[0] ) << 56 )
                     & 0xFF00000000000000LL )
                   + ( ( ( (EipUint64) buffer_address[1] ) << 48 ) &
                       0x00FF000000000000LL )
                   + ( ( ( (EipUint64) buffer_address[2] ) << 40 ) &
                       0x0000FF0000000000LL )
                   + ( ( ( (EipUint64) buffer_address[3] ) << 32 ) &
                       0x000000FF00000000LL )
                   + ( ( ( (EipUint64) buffer_address[4] ) << 24 ) &
                       0x00000000FF000000 )
                   + ( ( ( (EipUint64) buffer_address[5] ) << 16 ) &
                       0x0000000000FF0000 )
                   + ( ( ( (EipUint64) buffer_address[6] ) << 8 ) &
                       0x000000000000FF00 )
                   + ( ( (EipUint64) buffer_address[7] ) & 0x00000000000000FF );
  *buffer += 8;
  return data;
}

/**
 * @brief Converts EipUint64 data from host to little endian and writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
int AddLintToMessage(const EipUint64 data,
                     EipUint8 **const buffer) {
  EipUint8 *buffer_address = *buffer;
  buffer_address[0] = (EipUint8) (data >> 56) & 0xFF;
  buffer_address[1] = (EipUint8) (data >> 48) & 0xFF;
  buffer_address[2] = (EipUint8) (data >> 40) & 0xFF;
  buffer_address[3] = (EipUint8) (data >> 32) & 0xFF;
  buffer_address[4] = (EipUint8) (data >> 24) & 0xFF;
  buffer_address[5] = (EipUint8) (data >> 16) & 0xFF;
  buffer_address[6] = (EipUint8) (data >> 8) & 0xFF;
  buffer_address[7] = (EipUint8) (data) & 0xFF;
  (*buffer) += 8;

  return 8;
}

#endif


int EncapsulateIpAddress(EipUint16 port,
                         EipUint32 address,
                         EipByte **communication_buffer) {
  int size = 0;
  if (kOpENerEndianessLittle == g_opener_platform_endianess) {
    size += AddIntToMessage(htons(AF_INET), communication_buffer);
    size += AddIntToMessage(port, communication_buffer);
    size += AddDintToMessage(address, communication_buffer);

  } else {
    if (kOpENerEndianessBig == g_opener_platform_endianess) {
      (*communication_buffer)[0] = (unsigned char) (AF_INET >> 8);
      (*communication_buffer)[1] = (unsigned char) AF_INET;
      *communication_buffer += 2;
      size += 2;

      (*communication_buffer)[0] = (unsigned char) (port >> 8);
      (*communication_buffer)[1] = (unsigned char) port;
      *communication_buffer += 2;
      size += 2;

      (*communication_buffer)[3] = (unsigned char) address;
      (*communication_buffer)[2] = (unsigned char) (address >> 8);
      (*communication_buffer)[1] = (unsigned char) (address >> 16);
      (*communication_buffer)[0] = (unsigned char) (address >> 24);
      *communication_buffer += 4;
      size += 4;
    } else {
      fprintf(stderr,
              "No endianess detected! Probably the DetermineEndianess function was not executed!");
      exit (EXIT_FAILURE);
    }
  }
  return size;
}

/**
 * @brief Detects Endianess of the platform and sets global g_nOpENerPlatformEndianess variable accordingly
 *
 * Detects Endianess of the platform and sets global variable g_nOpENerPlatformEndianess accordingly,
 * whereas 0 equals little endian and 1 equals big endian
 */
void DetermineEndianess() {
  int i = 1;
  char *p = (char *) &i;
  if (p[0] == 1) {
    g_opener_platform_endianess = kOpENerEndianessLittle;
  } else {
    g_opener_platform_endianess = kOpENerEndianessBig;
  }
}

/**
 * @brief Returns global variable g_nOpENerPlatformEndianess, whereas 0 equals little endian and 1 equals big endian
 *
 * @return 0 equals little endian and 1 equals big endian
 */
int GetEndianess() {
  return g_opener_platform_endianess;
}

void MoveMessageNOctets(int amount_of_bytes_moved,
                        const CipOctet **message_runner) {
  (*message_runner) += amount_of_bytes_moved;
}

int FillNextNMessageOctetsWith(CipOctet value,
                               unsigned int amount_of_bytes_written,
                               CipOctet **message) {
  memset(*message, value, amount_of_bytes_written);
  return amount_of_bytes_written;
}

int FillNextNMessageOctetsWithValueAndMoveToNextPosition(CipOctet value,
                                                         unsigned int amount_of_filled_bytes,
                                                         CipOctet **message) {
  FillNextNMessageOctetsWith(value, amount_of_filled_bytes, message);
  MoveMessageNOctets(amount_of_filled_bytes, (const CipOctet **)message);
  return amount_of_filled_bytes;
}

