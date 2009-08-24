/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#ifndef ENDIANCONV_H_
#define ENDIANCONV_H_

#include "typedefs.h"

EIP_UINT16 ltohs(EIP_UINT8 **pa_buf);
EIP_UINT32 ltohl(EIP_UINT8 **pa_buf);
void htols(EIP_UINT16 data, EIP_UINT8 **pa_buf);
void htoll(EIP_UINT32 data, EIP_UINT8 **pa_buf);

#endif /*ENDIANCONV_H_*/
