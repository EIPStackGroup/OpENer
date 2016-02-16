/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef OPENER_ENDIANCONV_H_
#define OPENER_ENDIANCONV_H_

#include "typedefs.h"

/** @file endianconv.h
 * @brief Responsible for Endianess conversion
 */

typedef enum {
  kOpenerEndianessUnknown = -1,
  kOpENerEndianessLittle = 0,
  kOpENerEndianessBig = 1
} OpenerEndianess;

/** @ingroup ENCAP
 *
 * @brief Get an 16Bit integer from the network buffer, and moves pointer beyond the 16 bit value
 * @param buffer Pointer to the network buffer array. This pointer will be incremented by 2!
 * @return Extracted 16 bit integer value
 */
EipUint16 GetIntFromMessage(EipUint8 **buffer);

/** @ingroup ENCAP
 *
 * @brief Get an 32Bit integer from the network buffer.
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 4!
 * @return Extracted 32 bit integer value
 */
EipUint32 GetDintFromMessage(EipUint8 **buffer);

/** @ingroup ENCAP
 *
 * @brief Write an 16Bit integer to the network buffer.
 * @param data value to write
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 2!
 *
 * @return Length in bytes of the encoded message
 */
int AddIntToMessage(EipUint16 data, EipUint8 **buffer);

/** @ingroup ENCAP
 *
 * @brief Write an 32Bit integer to the network buffer.
 * @param data value to write
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 4!
 *
 * @return Length in bytes of the encoded message
 */
int AddDintToMessage(EipUint32 data, EipUint8 **buffer);

#ifdef OPENER_SUPPORT_64BIT_DATATYPES

EipUint64 GetLintFromMessage(EipUint8 **buffer);

/** @ingroup ENCAP
 *
 * @brief Write an 64Bit integer to the network buffer.
 * @param data value to write
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 8!
 *
 * @return Length in bytes of the encoded message
 */
int AddLintToMessage(EipUint64 pa_unData, EipUint8 **buffer);

#endif

/** @brief Encapsulate the sockaddr information as necessary for the Common Packet Format data items
 *
 * Converts and adds the provided port and IP address into an common packet format message
 *
 * @param port Port of the socket, has to be provided in big-endian
 * @param address IP address of the socket, has to be provided in big-endian
 * @param communcation_buffer The message buffer for sending the message
 */
int EncapsulateIpAddress(EipUint16 port, EipUint32 address,
                                           EipByte *communication_buffer);

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

#endif /* OPENER_ENDIANCONV_H_ */
