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


/* Used to convert between 32-bit integers and floating-points. */
union int_float_32 {
  CipDint as_integer;
  CipReal as_float;
};

/* Used to convert between 64-bit integers and floating-points. */
union int_float_64 {
  CipLint as_integer;
  CipLreal as_float;
};


/* THESE ROUTINES MODIFY THE BUFFER POINTER*/

/**
 *   @brief Reads EIP_UINT8 from *buffer and converts little endian to host.
 *   @param buffer pointer where data should be reed.
 *   @return EIP_UINT8 data value
 */
CipSint GetSintFromMessage(const EipUint8 **const buffer) {
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

CipBool GetBoolFromMessage(const EipBool8 **const buffer_address) {
  const EipBool8 *buffer = *buffer_address;
  EipBool8 data = buffer[0];
  *buffer_address += 1;
  return data;
}

/* little-endian-to-host unsigned 16 bit*/

/**
 *   @brief Reads EIP_UINT16 from *buffer and converts little endian to host.
 *   @param buffer pointer where data should be reed.
 *   @return EIP_UINT16 data value
 */
CipInt GetIntFromMessage(const EipUint8 **const buffer) {
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
CipDint GetDintFromMessage(const EipUint8 **const buffer) {
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

CipUdint GetDwordFromMessage(const CipOctet **const buffer_address) {
  const CipOctet *buffer = *buffer_address;
  CipDword data = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] <<
                  24;
  *buffer_address += 4;
  return data;
}

CipReal GetRealFromMessage(const CipOctet **const buffer) {
  union int_float_32 convert;
  convert.as_integer = GetDintFromMessage(buffer);
  return convert.as_float;
}

CipLreal GetLrealFromMessage(const CipOctet **const buffer) {
  union int_float_64 convert;
  convert.as_integer = GetLintFromMessage(buffer);
  return convert.as_float;
}

/**
 * @brief converts UINT8 data from host to little endian an writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
void AddSintToMessage(const EipUint8 data,
                      ENIPMessage *const outgoing_message) {

  outgoing_message->current_message_position[0] = (unsigned char) data;
  outgoing_message->current_message_position += 1;
  outgoing_message->used_message_length += 1;
}

/**
 * @brief converts UINT16 data from host to little endian an writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
void AddIntToMessage(const EipUint16 data,
                     ENIPMessage *const outgoing_message) {

  outgoing_message->current_message_position[0] = (unsigned char) data;
  outgoing_message->current_message_position[1] = (unsigned char) (data >> 8);
  outgoing_message->current_message_position += 2;
  outgoing_message->used_message_length += 2;
}

/**
 * @brief Converts UINT32 data from host to little endian and writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
void AddDintToMessage(const EipUint32 data,
                      ENIPMessage *const outgoing_message) {
  outgoing_message->current_message_position[0] = (unsigned char) data;
  outgoing_message->current_message_position[1] = (unsigned char) (data >> 8);
  outgoing_message->current_message_position[2] = (unsigned char) (data >> 16);
  outgoing_message->current_message_position[3] = (unsigned char) (data >> 24);

  outgoing_message->current_message_position += 4;
  outgoing_message->used_message_length += 4;
}

/**
 *   @brief Reads EipUint64 from *pa_buf and converts little endian to host.
 *   @param pa_buf pointer where data should be reed.
 *   @return EipUint64 value
 */
EipUint64 GetLintFromMessage(const EipUint8 **const buffer) {
  const EipUint8 *buffer_address = *buffer;
  EipUint64 data = ( (EipUint64)buffer_address[7] << 56
                     | (EipUint64)buffer_address[6] << 48
                     | (EipUint64)buffer_address[5] << 40
                     | (EipUint64)buffer_address[4] << 32
                     | (EipUint64)buffer_address[3] << 24
                     | (EipUint64)buffer_address[2] << 16
                     | (EipUint64)buffer_address[1] << 8
                     | (EipUint64)buffer_address[0] );
  *buffer += 8;
  return data;
}

/**
 * @brief Converts EipUint64 data from host to little endian and writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
void AddLintToMessage(const EipUint64 data,
                      ENIPMessage *const outgoing_message) {

  outgoing_message->current_message_position[0] = (EipUint8) (data);
  outgoing_message->current_message_position[1] = (EipUint8) (data >> 8);
  outgoing_message->current_message_position[2] = (EipUint8) (data >> 16);
  outgoing_message->current_message_position[3] = (EipUint8) (data >> 24);
  outgoing_message->current_message_position[4] = (EipUint8) (data >> 32);
  outgoing_message->current_message_position[5] = (EipUint8) (data >> 40);
  outgoing_message->current_message_position[6] = (EipUint8) (data >> 48);
  outgoing_message->current_message_position[7] = (EipUint8) (data >> 56);
  outgoing_message->current_message_position += 8;
  outgoing_message->used_message_length += 8;
}

void EncapsulateIpAddress(EipUint16 port,
                          EipUint32 address,
                          ENIPMessage *const outgoing_message) {
  if(kOpENerEndianessLittle == g_opener_platform_endianess) {
    AddIntToMessage(htons(AF_INET), outgoing_message);
    AddIntToMessage(port, outgoing_message);
    AddDintToMessage(address, outgoing_message);

  } else {
    if(kOpENerEndianessBig == g_opener_platform_endianess) {

      AddIntToMessage(htons(AF_INET), outgoing_message);

      AddSintToMessage( (unsigned char) (port >> 8), outgoing_message );
      AddSintToMessage( (unsigned char) port, outgoing_message );

      AddSintToMessage( (unsigned char) address, outgoing_message );
      AddSintToMessage( (unsigned char) (address >> 8), outgoing_message );
      AddSintToMessage( (unsigned char) (address >> 16), outgoing_message );
      AddSintToMessage( (unsigned char) (address >> 24), outgoing_message );

    } else {
      fprintf(stderr,
              "No endianess detected! Probably the DetermineEndianess function was not executed!");
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * @brief Detects Endianess of the platform and sets global g_nOpENerPlatformEndianess variable accordingly
 *
 * Detects Endianess of the platform and sets global variable g_nOpENerPlatformEndianess accordingly,
 * whereas 0 equals little endian and 1 equals big endian
 */
void DetermineEndianess() {
  int i = 1;
  const char *const p = (char *) &i;
  if(p[0] == 1) {
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

void MoveMessageNOctets(const int amount_of_bytes_moved,
                        ENIPMessage *const outgoing_message) {
  outgoing_message->current_message_position += amount_of_bytes_moved;
  outgoing_message->used_message_length += amount_of_bytes_moved;
}

void FillNextNMessageOctetsWith(CipOctet value,
                                unsigned int amount_of_bytes_written,
                                ENIPMessage *const outgoing_message) {
  memset(outgoing_message->current_message_position,
         value,
         amount_of_bytes_written);
}

void FillNextNMessageOctetsWithValueAndMoveToNextPosition(CipOctet value,
                                                          unsigned int amount_of_filled_bytes,
                                                          ENIPMessage *const outgoing_message)
{
  FillNextNMessageOctetsWith(value, amount_of_filled_bytes, outgoing_message);
  MoveMessageNOctets(amount_of_filled_bytes, outgoing_message);
}

