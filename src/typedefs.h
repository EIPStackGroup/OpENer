/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

/*
 Do not use interface types for internal variables, such as "int i;", which is
 commonly used for loop counters or counting things.

 Do not over-constrain data types. Prefer the use of the native "int" and
 "unsigned" types.

 Use char for native character strings.

 Do not use "char" for data buffers - use "unsigned char" instead. Using char
 for data buffers can occasionally blow up in your face rather nastily.
 */

#define EIP_BYTE 	unsigned char
#define EIP_INT8 	char
#define EIP_INT16	short
#define EIP_INT32	long
#define EIP_UINT8	unsigned char
#define EIP_UINT16	unsigned short
#define EIP_UINT32	unsigned long
#define EIP_FLOAT	float
#define EIP_DFLOAT	double
#define EIP_BOOL8	bool

#ifdef OPENER_SUPPORT_64BIT_DATATYPES
#define EIP_INT64       long long
#define EIP_UINT64       unsigned long long
#endif

/*! Constant identifying if a socket descriptor is invalid
 */
#define EIP_INVALID_SOCKET      -1

/*

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
  EIP_OK = 0, EIP_OK_SEND = 1, EIP_ERROR = -1
} EIP_STATUS;

#ifndef __cplusplus
/*! If we don't have C++ define a C++ -like "bool"*/
typedef enum
{
  false = 0, true = 1
} bool;

#endif

/* by default an enum is 32 bits
 __attribute((packed)) allows the compiler to use a shorter data type
 the following forces an enum to a specified minimum length, it also documents the size of the enum

 example:

 typedef enum { A, B, C, FOO_PACKED_SIZE=ENUM_UINT16} PACKED FOO;		 this forces the field to be 16 bits long, even though the defined values could be contained in 8 bits
 the definition FOO_PACKED_SIZE is a dummy, but it forces the minimum size
 */

/* TODO -- find some portable way of dealing with packed structs and typed enums */
#ifdef __GNUC__
#define PACKED __attribute__((packed))

#define ENUM_INT8 0x7f
#define ENUM_INT16  0x7fff
#define ENUM_INT32  0x7fffffff

#endif

#endif /*TYPEDEFS_H_*/
