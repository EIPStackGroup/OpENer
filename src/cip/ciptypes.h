/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef CIPTYPES_H_
#define CIPTYPES_H_

#include "typedefs.h"

/* TODO -- find some portable way of defining all these with enums rather than #defines so that the names rather than hex number are displayed in the debugger*/
#ifdef __GNUC__
typedef enum
  {
    SEG_PORT = 0x00,
    SEG_EXTPORT = 0x10,
    SEG_CLASS = 0x20,
    SEG_INSTANCE = 0x24,
    SEG_ATTRIBUTE = 0x30,
    SEG_NETWORK = 0x40,
    SEG_PACKED_SIZE = ENUM_INT8
  }PACKED SEG_TYPE;
#else
#define SEG_PORT  0x00
#define SEG_EXTPORT 0x10
#define SEG_CLASS 0x20
#define SEG_INSTANCE  0x24
#define SEG_ATTRIBUTE 0x30
#define SEG_NETWORK 0x40
#endif

/* definition of CIP basic data types */
#define CIP_BOOL 			0xC1
#define CIP_SINT 			0xC2
#define CIP_INT 			0xC3
#define CIP_DINT 			0xC4
#define CIP_LINT 			0xC5
#define CIP_USINT 			0xC6
#define CIP_UINT 			0xC7
#define CIP_UDINT 			0xC8
#define CIP_ULINT 			0xC9
#define CIP_REAL 			0xCA
#define CIP_LREAL 			0xCB
#define CIP_STIME 			0xCC
#define CIP_DATE 			0xCD
#define CIP_TIME_OF_DAY 		0xCE
#define CIP_DATE_AND_TIME 		0xCF
#define CIP_STRING 			0xD0
#define CIP_BYTE 			0xD1
#define CIP_WORD 			0xD2
#define CIP_DWORD 			0xD3
#define CIP_LWORD 			0xD4
#define CIP_STRING2 			0xD5
#define CIP_FTIME 			0xD6
#define CIP_LTIME 			0xD7
#define CIP_ITIME 			0xD8
#define CIP_STRINGN 			0xD9
#define CIP_SHORT_STRING 		0xDA
#define CIP_TIME 			0xDB
#define CIP_EPATH 			0xDC
#define CIP_ENGUNIT 			0xDD

/* definition of some CIP structs */
/* need to be validated in IEC 1131-3 subclause 2.3.3 */
#define CIP_USINT_USINT 		0xA0
#define CIP_UDINT_UDINT_UDINT_UDINT_UDINT_STRING 0xA1
#define CIP_6USINT			0xA2 /* for MAC Address*/
#define CIP_MEMBER_LIST			0xA3
#define CIP_BYTE_ARRAY			0xA4

#define INTERNAL_UINT16_6		0xf0				/* bogus hack, for port class attribute 9, TODO figure out the right way to handle it */

/* definition of CIP service codes */
#define CIP_GET_ATTRIBUTE_SINGLE	0x0E
#define CIP_SET_ATTRIBUTE_SINGLE	0x10
#define CIP_RESET			0x05
#define CIP_CREATE                      0x08
#define CIP_GET_ATTRIBUTE_ALL		0x01
#define CIP_FORWARD_OPEN		0x54
#define CIP_FORWARD_CLOSE		0x4E
#define CIP_UNCONNECTED_SEND		0x52
#define CIP_GET_CONNECTION_OWNER	0x5A

typedef enum
{
  enOpened, enTimedOut, enClosed
} EIOConnectionEvent;

typedef struct
{
  EIP_UINT16 len;
  EIP_BYTE *Data;
} S_CIP_Byte_Array;

typedef struct
{
  EIP_UINT8 Length;
  EIP_INT8 *String;
} S_CIP_Short_String;

typedef struct
{
  EIP_INT16 Length;
  EIP_INT8 *String;
} S_CIP_String;

typedef struct
{
  EIP_UINT8 PathSize;
  EIP_UINT32 ClassID; /* support up to 32 bit path*/
  EIP_UINT32 InstanceNr;
  EIP_UINT16 AttributNr;
} S_CIP_EPATH;

typedef struct
{
  EIP_UINT8 PathSize;
  EIP_UINT32 ClassID;
  EIP_UINT32 ConnectionPoint[3];
  EIP_UINT8 DataSegment;
  EIP_UINT8 *SegmentData;
} S_CIP_ConnectionPath;

typedef struct
{
  EIP_UINT16 VendorID;
  EIP_UINT16 DeviceType;
  EIP_UINT16 ProductCode;
  EIP_BYTE MajorRevision;
  EIP_UINT8 MinorRevision;
} S_CIP_KeyData;

typedef struct
{
  EIP_UINT8 MajorRevision;
  EIP_UINT8 MinorRevision;
} S_CIP_Revision;

typedef struct
{
  EIP_UINT8 SegmentType;
  EIP_UINT8 KeyFormat;
  S_CIP_KeyData KeyData;
} S_CIP_ElectronicKey;

typedef struct
{
  EIP_UINT8 Service;
  S_CIP_EPATH RequestPath;
  EIP_INT16 DataLength;
  EIP_UINT8 *Data;
} S_CIP_MR_Request;

#define MAX_SIZE_OF_ADD_STATUS 2 /* for now we support extended status codes up to 2 16bit values 
									there is mostly only one 16bit value used */
typedef struct
{
  EIP_UINT8 ReplyService;
  EIP_UINT8 Reserved;
  EIP_UINT8 GeneralStatus;
  EIP_UINT8 SizeofAdditionalStatus;
  EIP_UINT16 AdditionalStatus[MAX_SIZE_OF_ADD_STATUS];
  EIP_INT16 DataLength;
  EIP_UINT8 *Data;
} S_CIP_MR_Response;

typedef struct
{
  EIP_UINT16 CIP_AttributNr;
  EIP_UINT8 CIP_Type;
  void *pt2data;
} S_CIP_attribute_struct;

/* type definition of CIP service sructure */

/* instances are stored in a linked list*/
typedef struct CIP_Instance
{
  EIP_UINT32 nInstanceNr; /*!> this instance's number (unique within the class) */
  S_CIP_attribute_struct *pstAttributes; /* pointer to an array of attributes which is unique to this instance */
  struct CIP_Class *pstClass; /*!> class the instance belongs to */
  struct CIP_Instance *pstNext; /*!> next instance, all instances of a class live in a linked list */
} S_CIP_Instance;

typedef struct CIP_Class
{ /* Class is a subclass of Instance: the following group of fields must match CIP_Instance */
  EIP_UINT32 nInstanceNr; /*!> this instance's number (unique within the class)*/
  S_CIP_attribute_struct *pstAttributes; /*!> pointer to an array of attributes which is unique to this instance */
  struct CIP_Class *pstClass; /*!> class the instance belongs to*/
  struct CIP_Instance *pstNext; /*!> next instance, all instances of a class live in a linked list*/

  /* the rest of theswe are specific to the Class class only. */
  EIP_UINT32 nClassID; /*!> class ID */
  EIP_UINT16 nRevision; /*!> class revision*/
  EIP_UINT16 nNr_of_Instances; /*!> number of instances in the class (not including instance 0)*/
  EIP_UINT16 nNr_of_Attributes; /*!> number of attributes of each instance*/
  EIP_UINT16 nMaxAttribute; /*!> highest defined attribute number (attribute numbers are not necessarily consecutive)*/
  EIP_UINT32 nGetAttrAllMask; /*!> mask indicating which attributes are returned by getAttributeAll*/
  EIP_UINT16 nNr_of_Services; /*!> number of services supported*/
  S_CIP_Instance *pstInstances; /*!> pointer to the list of instances*/
  struct CIP_service_struct *pstServices; /*!> pointer to the array of services*/
  char *acName; /*!> class name */
} S_CIP_Class;

typedef EIP_STATUS
(*TCIPServiceFunc)(S_CIP_Instance *pa_pstInstance,
    S_CIP_MR_Request *pa_MRRequest, S_CIP_MR_Response *pa_MRResponse,
    EIP_UINT8 *pa_msg);

/* service descriptor. These are stored in an array*/
typedef struct CIP_service_struct
{
  EIP_UINT8 CIP_ServiceNr; /*!> service number*/
  TCIPServiceFunc m_ptfuncService; /*!> pointer to a function call*/
  char *name; /*!> name of the service */
} S_CIP_service_struct;

typedef struct
{
  EIP_UINT32 IPAddress;
  EIP_UINT32 NetworkMask;
  EIP_UINT32 Gateway;
  EIP_UINT32 NameServer;
  EIP_UINT32 NameServer2;
  S_CIP_String DomainName;
} S_CIP_TCPIPNetworkInterfaceConfiguration;

typedef struct
{
  EIP_UINT8 PathSize;
  EIP_UINT32 Port; /* support up to 32 bit path*/
  EIP_UINT32 Address;
} S_CIP_RPATH;

typedef struct CIP_UnconnectedSend_Param_Struct
{
  EIP_BYTE Priority;
  EIP_UINT8 Timeout_Ticks;
  EIP_UINT16 Message_Request_Size;
  S_CIP_MR_Request Message_Request;
  S_CIP_MR_Response *Message_Response;
  EIP_UINT8 Reserved;
  S_CIP_RPATH Route_Path;
  void *CPFdata;
} S_CIP_UnconnectedSend_Param_Struct;

/* these are used for creating the getAttributeAll masks
 TODO there might be a way simplifying this using __VARARGS__ in #define */
#define MASK1(a)               (1<<(a))
#define MASK2(a,b)             (1<<(a) | 1<<(b))
#define MASK3(a,b,c)           (1<<(a) | 1<<(b) | 1<<(c))
#define MASK4(a,b,c,d)         (1<<(a) | 1<<(b) | 1<<(c) | 1<<(d))
#define MASK5(a,b,c,d,e)       (1<<(a) | 1<<(b) | 1<<(c) | 1<<(d) | 1<<(e))
#define MASK6(a,b,c,d,e,f)     (1<<(a) | 1<<(b) | 1<<(c) | 1<<(d) | 1<<(e) | 1<<(f))
#define MASK7(a,b,c,d,e,f,g)   (1<<(a) | 1<<(b) | 1<<(c) | 1<<(d) | 1<<(e) | 1<<(f) | 1<<(g))
#define MASK8(a,b,c,d,e,f,g,h) (1<<(a) | 1<<(b) | 1<<(c) | 1<<(d) | 1<<(e) | 1<<(f) | 1<<(g) | 1<<(h))

#endif /*CIPTYPES_H_*/
