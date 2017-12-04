/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_ENDIANCONV_H_
#define OPENER_ENDIANCONV_H_

#include "typedefs.h"
#include "ciptypes.h"

/** @file endianconv.h
 * @brief Responsible for Endianess conversion
 */

typedef enum {
  kOpenerEndianessUnknown = -1,
  kOpENerEndianessLittle = 0,
  kOpENerEndianessBig = 1
} OpenerEndianess;

/** @ingroup ENCAP
 *   @brief Reads EIP_UINT8 from *buffer and converts little endian to host.
 *   @param buffer pointer where data should be reed.
 *   @return EIP_UINT8 data value
 */
EipUint8 GetSintFromMessage(const EipUint8 **const buffer);

CipByte GetByteFromMessage(const CipOctet **const buffer_address);

CipUsint GetUsintFromMessage(const CipOctet **const buffer_address);

/** @ingroup ENCAP
 *
 * @brief Get an 16Bit integer from the network buffer, and moves pointer beyond the 16 bit value
 * @param buffer Pointer to the network buffer array. This pointer will be incremented by 2!
 * @return Extracted 16 bit integer value
 */
EipUint16 GetIntFromMessage(const EipUint8 **const buffer);

CipUint GetUintFromMessage(const CipOctet **const buffer_address);

CipWord GetWordFromMessage(const CipOctet **const buffer_address);

/** @ingroup ENCAP
 *
 * @brief Get an 32Bit integer from the network buffer.
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 4!
 * @return Extracted 32 bit integer value
 */
EipUint32 GetDintFromMessage(const EipUint8 **const buffer);

CipUdint GetUdintFromMessage(const CipOctet **const buffer_address);
CipUdint GetUdintFromMessageTcpIp(const CipOctet **const buffer_address);

void GetCipStringFromMessageToLocation(const CipOctet **const buffer_address,
                                       CipString *cip_string);
/** @ingroup ENCAP
 *
 * @brief converts UINT8 data from host to little endian an writes it to buffer.
 * @param data value to be written
 * @param buffer pointer where data should be written.
 */
int AddSintToMessage(const EipUint8 data,
                     EipUint8 **const buffer);

/** @ingroup ENCAP
 *
 * @brief Write an 16Bit integer to the network buffer.
 * @param data value to write
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 2!
 *
 * @return Length in bytes of the encoded message
 */
int AddIntToMessage(const EipUint16 data,
                    EipUint8 **const buffer);

/** @ingroup ENCAP
 *
 * @brief Write an 32Bit integer to the network buffer.
 * @param data value to write
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 4!
 *
 * @return Length in bytes of the encoded message
 */
int AddDintToMessage(const EipUint32 data,
                     EipUint8 **const buffer);

#ifdef OPENER_SUPPORT_64BIT_DATATYPES

EipUint64 GetLintFromMessage(const EipUint8 **const buffer);

/** @ingroup ENCAP
 *
 * @brief Write an 64Bit integer to the network buffer.
 * @param data value to write
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 8!
 *
 * @return Length in bytes of the encoded message
 */
int AddLintToMessage(const EipUint64 pa_unData,
                     EipUint8 **const buffer);

#endif

/** @brief Encapsulate the sockaddr information as necessary for the Common Packet Format data items
 *
 * Converts and adds the provided port and IP address into an common packet format message
 *
 * @param port Port of the socket, has to be provided in big-endian
 * @param address IP address of the socket, has to be provided in big-endian
 * @param communcation_buffer The message buffer for sending the message
 */
int EncapsulateIpAddress(EipUint16 port,
                         EipUint32 address,
                         EipByte **communication_buffer);

/** Identify if we are running on a big or little endian system and set
 * variable.
 */
void DetermineEndianess(void);

/** @brief Return the endianess identified on system startup
 * @return
 *    - -1 endianess has not been identified up to now
 *    - 0  little endian system
 *    - 1  big endian system
 */
int GetEndianess(void);

void MoveMessageNOctets(int n,
                        CipOctet **message_runner);

int FillNextNMessageOctetsWith(CipOctet value,
                               unsigned int n,
                               CipOctet **message);

int FillNextNMessageOctetsWithValueAndMoveToNextPosition(CipOctet value,
                                                         unsigned int n,
                                                         CipOctet **message);
#endif /* OPENER_ENDIANCONV_H_ */
