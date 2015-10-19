/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <opener_user_conf.h>
#include <inttypes.h>
#include <stddef.h>

/**
 Do not use interface types for internal variables, such as "int i;", which is
 commonly used for loop counters or counting things.

 Do not over-constrain data types. Prefer the use of the native "int" and
 "unsigned" types.

 Use char for native character strings.

 Do not use "char" for data buffers - use "unsigned char" instead. Using char
 for data buffers can occasionally blow up in your face rather nastily.
 */

#define EIP_BYTE 	uint8_t
#define EIP_INT8 	int8_t
#define EIP_INT16	int16_t
#define EIP_INT32	int32_t
#define EIP_UINT8	uint8_t
#define EIP_UINT16	uint16_t
#define EIP_UINT32	uint32_t
#define EIP_FLOAT	float
#define EIP_DFLOAT	double
#define EIP_BOOL8	int

#ifdef OPENER_SUPPORT_64BIT_DATATYPES
#define EIP_INT64       int64_t
#define EIP_UINT64      uint64_t
#endif
#endif

/*! Constant identifying if a socket descriptor is invalid
 */
#define EIP_INVALID_SOCKET      -1

/**

 The following are generally true regarding return status:
 -1 ... an error occurred
 0 ... success

 Occasionally there is a variation on this:
 -1 ... an error occurred
 0 ..  success and there is no reply to send
 1 ... success and there is a reply to send

 For both of these cases EIP_STATUS is the return type.

 Other return type are:
 -- return pointer to thing, 0 if error (return type is "pointer to thing")
 -- return count of something, -1 if error, (return type is int)

 */

typedef enum
{
  EIP_OK = 0,
  EIP_OK_SEND = 1,
  EIP_ERROR = -1
} EIP_STATUS;

#ifndef __cplusplus
/*! If we don't have C++ define a C++ -like "bool" keyword defines*/
#define false 0
#define true 1

#endif

#endif /*TYPEDEFS_H_*/
