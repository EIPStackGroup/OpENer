/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef ENDIANCONV_H_
#define ENDIANCONV_H_

#include "typedefs.h"

#define OPENER_LITTLE_ENDIAN_PLATFORM 0
#define OPENER_BIG_ENDIAN_PLATFORM    1

/*! \ingroup ENCAP
 * Get an 16Bit integer from the network buffer.
 * @param pa_buf pointer to the network buffer array. This pointer will be incremented by 2!
 */EIP_UINT16
ltohs(EIP_UINT8 **pa_buf);

/*! \ingroup ENCAP
 * Get an 32Bit integer from the network buffer.
 * @param pa_buf pointer to the network buffer array. This pointer will be incremented by 4!
 */EIP_UINT32
ltohl(EIP_UINT8 **pa_buf);

/*! \ingroup ENCAP
 * Write an 16Bit integer to the network buffer.
 * @param data value to write
 * @param pa_buf pointer to the network buffer array. This pointer will be incremented by 2!
 */
void
htols(EIP_UINT16 data, EIP_UINT8 **pa_buf);

/*! \ingroup ENCAP
 * Write an 32Bit integer to the network buffer.
 * @param data value to write
 * @param pa_buf pointer to the network buffer array. This pointer will be incremented by 4!
 */
void
htoll(EIP_UINT32 data, EIP_UINT8 **pa_buf);

#ifdef OPENER_SUPPORT_64BIT_DATATYPES

EIP_UINT64
ltoh64(EIP_UINT8 ** pa_pnBuf);

void
htol64(EIP_UINT64 pa_unData, EIP_UINT8 ** pa_pnBuf);

#endif

void
encapsulateIPAdress(EIP_UINT16 pa_unPort, EIP_UINT32 pa_unAddr,
    EIP_BYTE *pa_acCommBuf);


/** Identify if we are running on a big or little endian system and set
 * variable.
 */
void determineEndianess();

/** Return the endianess identified on system startup
 * \return
 *    - -1 endianess has not been identified up to now
 *    - 0  little endian system
 *    - 1  big endian system
 */
int getEndianess();

#endif /*ENDIANCONV_H_*/
