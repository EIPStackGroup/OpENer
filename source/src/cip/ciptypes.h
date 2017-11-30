/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPTYPES_H_
#define OPENER_CIPTYPES_H_

#include "typedefs.h"
#include "networkhandler.h"

/** @brief Enum containing the encoding values for CIP data types for CIP
 * Messages */
typedef enum cip_data_types {
  kCipAny = 0x00, /**< data type that can not be directly encoded */
  kCipBool = 0xC1, /**< boolean data type */
  kCipSint = 0xC2, /**< 8-bit signed integer */
  kCipInt = 0xC3, /**< 16-bit signed integer */
  kCipDint = 0xC4, /**< 32-bit signed integer */
  kCipLint = 0xC5, /**< 64-bit signed integer */
  kCipUsint = 0xC6, /**< 8-bit unsigned integer */
  kCipUint = 0xC7, /**< 16-bit unsigned integer */
  kCipUdint = 0xC8, /**< 32-bit unsigned integer */
  kCipUlint = 0xC9, /**< 64-bit unsigned integer */
  kCipReal = 0xCA, /**< Single precision floating point */
  kCipLreal = 0xCB, /**< Double precision floating point*/
  kCipStime = 0xCC, /**< Synchronous time information*, type of DINT */
  kCipDate = 0xCD, /**< Date only*/
  kCipTimeOfDay = 0xCE, /**< Time of day */
  kCipDateAndTime = 0xCF, /**< Date and time of day */
  kCipString = 0xD0, /**< Character string, 1 byte per character */
  kCipByte = 0xD1, /**< 8-bit bit string */
  kCipWord = 0xD2, /**< 16-bit bit string */
  kCipDword = 0xD3, /**< 32-bit bit string */
  kCipLword = 0xD4, /**< 64-bit bit string */
  kCipString2 = 0xD5, /**< Character string, 2 byte per character */
  kCipFtime = 0xD6, /**< Duration in micro-seconds, high resolution; range of DINT */
  kCipLtime = 0xD7, /**< Duration in micro-seconds, high resolution, range of LINT */
  kCipItime = 0xD8, /**< Duration in milli-seconds, short; range of INT*/
  kCipStringN = 0xD9, /**< Character string, N byte per character */
  kCipShortString = 0xDA, /**< Character string, 1 byte per character, 1 byte
                             length indicator */
  kCipTime = 0xDB, /**< Duration in milli-seconds; range of DINT */
  kCipEpath = 0xDC, /**< CIP path segments*/
  kCipEngUnit = 0xDD, /**< Engineering Units*/
  /* definition of some CIP structs */
  /* need to be validated in IEC 61131-3 subclause 2.3.3 */
  /* TODO: Check these codes */
  kCipUsintUsint = 0xA0, /**< Used for CIP Identity attribute 4 Revision*/
  kCipUdintUdintUdintUdintUdintString = 0xA1, /**< TCP/IP attribute 5 - IP address, subnet mask, gateway, IP name
                                                 server 1, IP name server 2, domain name*/
  kCip6Usint = 0xA2, /**< Struct for MAC Address (six USINTs)*/
  kCipMemberList = 0xA3, /**< */
  kCipByteArray = 0xA4, /**< */
  kInternalUint6 = 0xF0 /**< bogus hack, for port class attribute 9, TODO
                           figure out the right way to handle it */
} CipDataType;


/** @brief Definition of CIP service codes
 *
 * An Enum with all CIP service codes. Common services codes range from 0x01 to
 *****0x1C
 *
 */
typedef enum {
  /* Start CIP common services */
  kGetAttributeAll = 0x01,
  kSetAttributeAll = 0x02,
  kGetAttributeList = 0x03,
  kSetAttributeList = 0x04,
  kReset = 0x05,
  kStart = 0x06,
  kStop = 0x07,
  kCreate = 0x08,
  kDelete = 0x09,
  kMultipleServicePacket = 0x0A,
  kApplyAttributes = 0x0D,
  kGetAttributeSingle = 0x0E,
  kSetAttributeSingle = 0x10,
  kFindNextObjectInstance = 0x11,
  kRestore = 0x15,
  kSave = 0x16,
  kNoOperation = 0x17,
  kGetMember = 0x18,
  kSetMember = 0x19,
  kInsertMember = 0x1A,
  kRemoveMember = 0x1B,
  kGroupSync = 0x1C,
  /* End CIP common services */

  /* Start CIP object-specific services */
  kForwardOpen = 0x54,
  kForwardClose = 0x4E,
  kUnconnectedSend = 0x52,
  kGetConnectionOwner = 0x5A
/* End CIP object-specific services */
} CIPServiceCode;

/** @brief Definition of Get and Set Flags for CIP Attributes */
typedef enum { /* TODO: Rework */
  kNotSetOrGetable = 0x00, /**< Neither set-able nor get-able */
  kGetableAll = 0x01, /**< Get-able, also part of Get Attribute All service */
  kGetableSingle = 0x02, /**< Get-able via Get Attribute */
  kSetable = 0x04, /**< Set-able via Set Attribute */
  /* combined for convenience */
  kSetAndGetAble = 0x07, /**< both set and get-able */
  kGetableSingleAndAll = 0x03 /**< both single and all */
} CIPAttributeFlag;

typedef enum {
  kIoConnectionEventOpened,
  kIoConnectionEventTimedOut,
  kIoConnectionEventClosed
} IoConnectionEvent;

/** @brief CIP Byte Array
 *
 */
typedef struct {
  EipUint16 length; /**< Length of the Byte Array */
  EipByte *data; /**< Pointer to the data */
} CipByteArray;

/** @brief CIP Short String
 *
 */
typedef struct {
  EipUint8 length; /**< Length of the String (8 bit value) */
  EipByte *string; /**< Pointer to the string data */
} CipShortString;

/** @brief CIP String
 *
 */
typedef struct {
  EipUint16 length; /**< Length of the String (16 bit value) */
  EipByte *string; /**< Pointer to the string data */
} CipString;

typedef struct {
  EipUint16 size;
  EipUint16 length; /**< Length of the String (16 bit value) */
  EipByte *string; /**< Pointer to the string data */
} CipStringN;

/** @brief Struct for padded EPATHs
 *
 */
typedef struct {
  EipUint8 path_size; /**< Path size in 16 bit words (path_size * 16 bit) */
  EipUint16 class_id; /**< Class ID of the linked object */
  EipUint16 instance_number; /**< Requested Instance Number of the linked object */
  EipUint16 attribute_number; /**< Requested Attribute Number of the linked object */
} CipEpath;

typedef enum connection_point_type {
  kConnectionPointTypeProducing = 0,
  kConnectionPointTypeConsuming,
  kConnectionPointTypeConfig,
  kConnectionPointTypeMaxValue
} ConnectionPointType;

/** @brief CIP Connection Path
 *
 */
typedef struct {
  EipUint8 path_size; /**< Path size in 16 bit words (path_size * 16 bit) */
  EipUint32 class_id; /**< Class ID of the linked object */
  EipUint32 connection_point[kConnectionPointTypeMaxValue];
  EipUint8 data_segment;
  EipUint8 *segment_data;
} CipConnectionPath;

/** @brief Struct representing the key data format of the electronic key segment
 *
 */
typedef struct {
  CipUint vendor_id; /**< Vendor ID */
  CipUint device_type; /**< Device Type */
  CipUint product_code; /**< Product Code */
  CipByte major_revision; /**< Major Revision and Compatibility (Bit 0-6 = Major
                             Revision) Bit 7 = Compatibility */
  CipUsint minor_revision; /**< Minor Revision */
} CipKeyData;

/** @brief Struct storing the CIP revision */
typedef struct {
  EipUint8 major_revision;
  EipUint8 minor_revision;
} CipRevision;

/** @brief CIP Electronic Key Segment struct
 *
 */
typedef struct {
  CipUsint segment_type; /**< Specifies the Segment Type */
  CipUsint key_format; /**< Key Format 0-3 reserved, 4 = see Key Format Table,
                          5-255 = Reserved */
  CipKeyData key_data; /**< Depends on key format used, usually Key Format 4 as
                          specified in CIP Specification, Volume 1*/
} CipElectronicKey;

/** @brief CIP Message Router Request
 *
 */
typedef struct {
  CipUsint service;
  CipEpath request_path;
  EipInt16 request_path_size;
  const CipOctet *data;
} CipMessageRouterRequest;

#define MAX_SIZE_OF_ADD_STATUS 2 /* for now we support extended status codes up to 2 16bit values there is mostly only one 16bit value used */

/** @brief CIP Message Router Response
 *
 */
typedef struct {
  CipUsint reply_service; /**< Reply service code, the requested service code +
                             0x80 */
  CipOctet reserved; /**< Reserved; Shall be zero */
  CipUsint general_status; /**< One of the General Status codes listed in CIP
                              Specification Volume 1, Appendix B */
  CipUsint size_of_additional_status; /**< Number of additional 16 bit words in
                                         Additional Status Array */
  EipUint16 additional_status[MAX_SIZE_OF_ADD_STATUS]; /**< Array of 16 bit words; Additional status;
                                                          If SizeOfAdditionalStatus is 0. there is no
                                                          Additional Status */
  EipInt16 data_length; /**< Supportative non-CIP variable, gives length of data segment */
  CipOctet *data; /**< Array of octet; Response data per object definition from
                     request */
} CipMessageRouterResponse;

typedef struct {
  EipUint16 attribute_number;
  EipUint8 type;
  CIPAttributeFlag attribute_flags; /*< 0 => getable_all, 1 => getable_single; 2 =>
                                       setable_single; 3 => get and setable; all other
                                       values reserved */
  void *data;
} CipAttributeStruct;

/* type definition of CIP service structure */

/* instances are stored in a linked list*/
typedef struct cip_instance {
  EipUint32 instance_number; /**< this instance's number (unique within the class) */
  CipAttributeStruct *attributes; /**< pointer to an array of attributes which
                                     is unique to this instance */
  struct cip_class *cip_class; /**< class the instance belongs to */
  struct cip_instance *next; /**< next instance, all instances of a class live
                                in a linked list */
} CipInstance;

/** @brief Class is a subclass of Instance */
typedef struct cip_class {
  CipInstance class_instance;
  /* the rest of these are specific to the Class class only. */
  EipUint32 class_id; /**< class ID */
  EipUint16 revision; /**< class revision*/
  EipUint16 number_of_instances; /**< number of instances in the class (not
                                    including instance 0)*/
  EipUint16 number_of_attributes; /**< number of attributes of each instance*/
  EipUint16 highest_attribute_number; /**< highest defined attribute number
                                         (attribute numbers are not necessarily
                                         consecutive)*/
  uint8_t *get_single_bit_mask; /**< Bitmask for GetAttributeSingle*/
  uint8_t *set_bit_mask;        /**< Bitmask for SetAttributeSingle*/
  uint8_t *get_all_bit_mask;    /**< Bitmask for GetAttributeAll*/

  EipUint32 get_attribute_all_mask; /**< mask indicating which attributes are
                                       returned by getAttributeAll*/
  EipUint16 number_of_services; /**< number of services supported*/
  CipInstance *instances; /**< pointer to the list of instances*/
  struct cip_service_struct *services; /**< pointer to the array of services*/
  char *class_name; /**< class name */
} CipClass;

/** @ingroup CIP_API
 *  @typedef  EIP_STATUS (*TCIPServiceFunc)(S_CIP_Instance *pa_pstInstance,
 *   S_CIP_MR_Request *pa_MRRequest, S_CIP_MR_Response *pa_MRResponse)
 *  @brief Signature definition for the implementation of CIP services.
 *
 *  CIP services have to follow this signature in order to be handled correctly
 *   by the stack.
 *  @param pa_pstInstance the instance which was referenced in the service
 *   request
 *  @param pa_MRRequest request data
 *  @param pa_MRResponse storage for the response data, including a buffer for
 *   extended data
 *  @return EIP_OK_SEND if service could be executed successfully and a response
 *   should be sent
 */
typedef EipStatus (*CipServiceFunction)(
  CipInstance *const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  struct sockaddr *originator_address);

/** @brief Service descriptor. These are stored in an array */
typedef struct cip_service_struct {
  EipUint8 service_number; /**< service number*/
  CipServiceFunction service_function; /**< pointer to a function call*/
  char *name; /**< name of the service */
} CipServiceStruct;

/**
 * @brief Struct for saving TCP/IP interface information
 */
typedef struct {
  CipUdint ip_address;
  CipUdint network_mask;
  CipUdint gateway;
  CipUdint name_server;
  CipUdint name_server_2;
  CipString domain_name;
} CipTcpIpNetworkInterfaceConfiguration;

typedef struct {
  EipUint8 path_size;
  EipUint32 port; /* support up to 32 bit path*/
  EipUint32 address;
} CipRoutePath;

typedef struct {
  EipByte priority;
  EipUint8 timeout_ticks;
  EipUint16 message_request_size;
  CipMessageRouterRequest message_request;
  CipMessageRouterResponse *message_response;
  EipUint8 reserved;
  CipRoutePath route_path;
  void *data;
} CipUnconnectedSendParameter;

/** @brief Data of an CIP Ethernet Link object */
//typedef struct {
//  EipUint32 interface_speed; /**< 10/100/1000 Mbit/sec */
//  EipUint32 interface_flags; /**< Inferface flags as defined in the CIP specification */
//  EipUint8 physical_address[6]; /**< MAC address of the Ethernet link */
//} CipEthernetLinkObject;




typedef struct {
  CipUint num_conn_entries;
  CipBool *conn_open_bits;
} CipConnectionManagerConnectionEntryList;


/* these are used for creating the getAttributeAll masks
   TODO there might be a way simplifying this using __VARARGS__ in #define */
#define MASK1(a) ( 1 << (a) )
#define MASK2(a, b) ( 1 << (a) | 1 << (b) )
#define MASK3(a, b, c) ( 1 << (a) | 1 << (b) | 1 << (c) )
#define MASK4(a, b, c, d) ( 1 << (a) | 1 << (b) | 1 << (c) | 1 << (d) )
#define MASK5(a, b, c, d, e) \
  ( 1 << (a) | 1 << (b) | 1 << (c) | 1 << (d) | 1 << (e) )
#define MASK6(a, b, c, d, e, f) \
  ( 1 << (a) | 1 << (b) | 1 << (c) | 1 << (d) | 1 << (e) | 1 << (f) )
#define MASK7(a, b, c, d, e, f, g) \
  ( 1 << (a) | 1 << (b) | 1 << (c) | 1 << (d) | 1 << (e) | 1 << (f) | 1 << (g) )
#define MASK8(a, b, c, d, e, f, g, h)                                \
  ( 1 << (a) | 1 << (b) | 1 << (c) | 1 << (d) | 1 << (e) | 1 << (f) | \
    1 << (g) | 1 << (h) )

#endif /* OPENER_CIPTYPES_H_ */
