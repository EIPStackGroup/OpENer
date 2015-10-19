/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef OpENer_ENDIANCONV_H_
#define OpENer_ENDIANCONV_H_

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
 */
void AddIntToMessage(EipUint16 data, EipUint8 **buffer);

/** @ingroup ENCAP
 *
 * @brief Write an 32Bit integer to the network buffer.
 * @param data value to write
 * @param buffer pointer to the network buffer array. This pointer will be incremented by 4!
 */
void AddDintToMessage(EipUint32 data, EipUint8 **buffer);

#ifdef OPENER_SUPPORT_64BIT_DATATYPES

EipUint64 GetLintFromMessage(EipUint8 **buffer);

void AddLintToMessage(EipUint64 pa_unData, EipUint8 **buffer);

#endif

void EncapsulateIpAddress(EipUint16 port, EipUint32 address,
                          EipByte *communication_buffer);

/** @brief Encapsulate the sockaddr information as necessary for the Common Paket Format data items
 */
void EncapsulateIpAddressCommonPaketFormat(EipUint16 port, EipUint32 address,
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

#endif /*OpENer_ENDIANCONV_H_*/
