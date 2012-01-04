/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef ENDIANCONV_H_
#define ENDIANCONV_H_

#include "typedefs.h"

/*! \ingroup ENCAP
 * Get an 16Bit integer from the network buffer.
 * @param pa_buf pointer to the network buffer array. This pointer will be incremented by 2!
 */
EIP_UINT16 ltohs(EIP_UINT8 **pa_buf);

/*! \ingroup ENCAP
 * Get an 32Bit integer from the network buffer.
 * @param pa_buf pointer to the network buffer array. This pointer will be incremented by 4!
 */
EIP_UINT32 ltohl(EIP_UINT8 **pa_buf);

/*! \ingroup ENCAP
 * Write an 16Bit integer to the network buffer.
 * @param data value to write
 * @param pa_buf pointer to the network buffer array. This pointer will be incremented by 2!
 */
void htols(EIP_UINT16 data, EIP_UINT8 **pa_buf);

/*! \ingroup ENCAP
 * Write an 32Bit integer to the network buffer.
 * @param data value to write
 * @param pa_buf pointer to the network buffer array. This pointer will be incremented by 4!
 */
void htoll(EIP_UINT32 data, EIP_UINT8 **pa_buf);

#ifdef OPENER_SUPPORT_64BIT_DATATYPES

EIP_UINT64 ltoh64(EIP_UINT8 ** pa_pnBuf);

void htol64(EIP_UINT64 pa_unData, EIP_UINT8 ** pa_pnBuf);

#endif


#endif /*ENDIANCONV_H_*/
