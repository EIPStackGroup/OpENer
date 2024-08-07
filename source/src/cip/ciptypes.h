/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPTYPES_H_
#define OPENER_CIPTYPES_H_

#include "typedefs.h"
#include "networkhandler.h"
#include "enipmessage.h"

#include "opener_user_conf.h"

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
  kCipEngUnit = 0xDD, /**< Engineering Units, range of UINT*/
  /* definition of some CIP structs */
  /* need to be validated in IEC 61131-3 subclause 2.3.3 */
  /* TODO: Check these codes */
  kCipUsintUsint = 0xA0, /**< Used for CIP Identity attribute 4 Revision*/
  kCipUdintUdintUdintUdintUdintString = 0xA1, /**< TCP/IP attribute 5 - IP address, subnet mask, gateway, IP name
                                                 server 1, IP name server 2, domain name*/
  kCip6Usint = 0xA2, /**< Struct for MAC Address (six USINTs)*/
  kCipMemberList = 0xA3, /**< */
  kCipByteArray = 0xA4, /**< */
  kInternalUint6 = 0xF0, /**< bogus hack, for port class attribute 9, TODO
                            figure out the right way to handle it */
  kCipStringI
} CipDataType;

/** @brief returns the size of CIP data types in bytes
 * @param type CIP data type
 * @param data use data pointer if data length is variable, else set NULL
 * @return size of CIP data type in bytes
 * */
size_t GetCipDataTypeLength(EipUint8 type,
                            const EipUint8 *data);

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
  kGetConnectionPointMemberList = 0x1D,
  /* End CIP common services */

  /* Start CIP object-specific services */
  kEthLinkGetAndClear = 0x4C, /**< Ethernet Link object's Get_And_Clear service */
  kForwardOpen = 0x54,
  kLargeForwardOpen = 0x5B,
  kForwardClose = 0x4E,
  kUnconnectedSend = 0x52,
  kGetConnectionOwner = 0x5A,
  kGetConnectionData = 0x56,
  kSearchConnectionData = 0x57
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
  kGetableSingleAndAll = 0x03, /**< both single and all */
  /* Flags to control the usage of callbacks per attribute from the Get* and Set* services */
  kGetableAllDummy = 0x08, /**< Get-able but a dummy Attribute */
  kPreGetFunc = 0x10, /**< enable pre get callback */
  kPostGetFunc = 0x20, /**< enable post get callback */
  kPreSetFunc = 0x40, /**< enable pre set callback */
  kPostSetFunc = 0x80, /**< enable post set callback */
  kNvDataFunc = 0x80, /**< enable Non Volatile data callback, is the same as @ref kPostSetFunc */
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
  EipUint16 length;   /**< Length of the Byte Array */
  EipByte *data;   /**< Pointer to the data */
} CipByteArray;

/** @brief CIP Short String
 *
 */
typedef struct {
  EipUint8 length;   /**< Length of the String (8 bit value) */
  EipByte *string;   /**< Pointer to the string data */
} CipShortString;

/** @brief CIP String
 *
 */
typedef struct {
  EipUint16 length;   /**< Length of the String (16 bit value) */
  CipByte *string;   /**< Pointer to the string data */
} CipString;

/** @brief CIP String2
 *
 */
typedef struct {
  EipUint16 length;   /**< Length of the String (16 bit value) */
  CipWord *string;   /**< Pointer to the string data */
} CipString2;

/** @brief CIP String with variable symbol size
 *
 */
typedef struct {
  EipUint16 size;   /**< Amount of bytes per symbol */
  EipUint16 length;   /**< Length of the String (16 bit value) */
  EipByte *string;   /**< Pointer to the string data */
} CipStringN;

/** @brief STRINGI definition
 *
 */
typedef struct cip_type_string_i_struct CipStringIStruct;

typedef struct cip_string_i {
  CipUsint number_of_strings;
  CipStringIStruct *array_of_string_i_structs;
} CipStringI;

typedef enum cip_type_string_i_character_set {
  kCipStringICharSet_ISO_8859_1_1987 = 4,
  kCipStringICharSet_ISO_8859_2_1987 = 5,
  kCipStringICharSet_ISO_8859_3_1988 = 6,
  kCipStringICharSet_ISO_8859_4_1988 = 7,
  kCipStringICharSet_ISO_8859_5_1988 = 8,
  kCipStringICharSet_ISO_8859_6_1987 = 9,
  kCipStringICharSet_ISO_8859_7_1987 = 10,
  kCipStringICharSet_ISO_8859_8_1989 = 11,
  kCipStringICharSet_ISO_8859_9_1989 = 12,
  kCipStringICharSet_ISO_10646_UCS_2 = 1000,
  kCipStringICharSet_ISO_10646_UCS_4 = 1001
} CipStringICharacterSet;

typedef struct cip_type_string_i_struct {
  CipUsint language_char_1;
  CipUsint language_char_2;
  CipUsint language_char_3;
  CipUint char_string_struct;   /**< EPath Either 0xD0, 0xD5, 0xD9, or 0xDA */
  CipUint character_set;   /**< Character set of the string */
  CipOctet *string;   /**< Pointer to the string data */
} CipStringIStruct;


/** @brief Highest CIP instance number.
 *
 * Largest value that can be used to represent or count CIP instances;
 * intentended for use when validating instance numbers encoded as data types
 * that can contain illegal values.
 */
extern const CipInstanceNum kCipInstanceNumMax;


/** @brief Struct for padded EPATHs
 *
 * Here the class code is referenced as class ID - see Vol. 1 C-1.4.2
 *
 */
typedef struct {
  EipUint8 path_size;   /**< Path size in 16 bit words (path_size * 16 bit) */
  EipUint16 class_id;   /**< Class ID of the linked object */
  CipInstanceNum instance_number;   /**< Requested Instance Number of the linked object */
  EipUint16 attribute_number;   /**< Requested Attribute Number of the linked object */
} CipEpath;

typedef enum connection_point_type {
  kConnectionPointTypeProducing = 0,
  kConnectionPointTypeConsuming,
  kConnectionPointTypeConfig,
  kConnectionPointTypeMaxValue
} ConnectionPointType;

/** @brief CIP Connection Path
 *
 * As an EPath the naming scheme of Vol. 1 C-1.4.2. has been used
 *
 */
typedef struct {
  EipUint8 path_size;   /**< Path size in 16 bit words (path_size * 16 bit) */
  EipUint32 class_id;   /**< Class ID of the linked object */
  EipUint32 connection_point[kConnectionPointTypeMaxValue];
  EipUint8 data_segment;
  EipUint8 *segment_data;
} CipConnectionPath;

/** @brief Struct storing the CIP revision */
typedef struct {
  EipUint8 major_revision;
  EipUint8 minor_revision;
} CipRevision;

/** @brief CIP Message Router Request
 *
 */
typedef struct {
  CipUsint service;
  CipEpath request_path;
  size_t request_data_size;
  const CipOctet *data;
} CipMessageRouterRequest;

#define MAX_SIZE_OF_ADD_STATUS 2 /* for now we support extended status codes up to 2 16bit values there is mostly only one 16bit value used */

typedef struct enip_message ENIPMessage;

/** @brief CIP Message Router Response
 *
 */
typedef struct {
  CipUsint reply_service;   /**< Reply service code, the requested service code +
                               0x80 */
  CipOctet reserved;   /**< Reserved; Shall be zero */
  CipUsint general_status;   /**< One of the General Status codes listed in CIP
                                Specification Volume 1, Appendix B */
  CipUsint size_of_additional_status;   /**< Number of additional 16 bit words in
                                           Additional Status Array */
  EipUint16 additional_status[MAX_SIZE_OF_ADD_STATUS];   /**< Array of 16 bit words; Additional status;
                                                            If SizeOfAdditionalStatus is 0. there is no
                                                            Additional Status */
  ENIPMessage message;   /* The constructed message */
} CipMessageRouterResponse;

/** @brief self-describing data encoding for CIP types */
typedef void (*CipAttributeEncodeInMessage)(const void *const data,
                                            ENIPMessage *const outgoing_message);

/** @brief self-describing data decoding for CIP types */
typedef int (*CipAttributeDecodeFromMessage)(void *const data,
                                             CipMessageRouterRequest *
                                             const message_router_request,
                                             CipMessageRouterResponse *const
                                             message_router_response);

/** @brief Structure to describe a single CIP attribute of an object
 */
typedef struct {
  EipUint16 attribute_number;   /**< The attribute number of this attribute. */
  EipUint8 type;   /**< The @ref CipDataType of this attribute. */
  CipAttributeEncodeInMessage encode;   /**< Self-describing its data encoding */
  CipAttributeDecodeFromMessage decode;   /**< Self-describing its data decoding */
  CIPAttributeFlag attribute_flags;   /**< See @ref CIPAttributeFlag declaration for valid values. */
  void *data;
} CipAttributeStruct;

/** @brief Type definition of one instance of an Ethernet/IP object
 *
 *  All instances are stored in a linked list that originates from the CipClass::instances
 *  pointer of the @ref CipClass structure.
 */
typedef struct cip_instance {
  CipInstanceNum instance_number;   /**< this instance's number (unique within the class) */
  CipAttributeStruct *attributes;   /**< pointer to an array of attributes which
                                       is unique to this instance */
  struct cip_class *cip_class;   /**< class the instance belongs to */
  struct cip_instance *next;   /**< next instance, all instances of a class live
                                  in a linked list */
  void *data; /**< pointer to instance data struct */
} CipInstance;

/** @ingroup CIP_API
 *  @typedef EipStatus (*CipGetSetCallback)(
 *    CipInstance *const instance,
 *    CipAttributeStruct *const attribute,
 *    CipByte service
 *  )
 *  @brief Signature definition of callback functions for Set and Get services
 *
 *  @param  instance  CIP instance involved in the Set or Get service
 *  @param  attribute CIP attribute involved in the Set or Get service
 *  @param  service   service code of currently executed service
 *  @return           status of kEipStatusOk or kEipStatusError on failure
 */
typedef EipStatus (*CipGetSetCallback)(CipInstance *const instance,
                                       CipAttributeStruct *const attribute,
                                       CipByte service);

/** @ingroup CIP_API
 *  @typedef EipStatus (*CipCallback)(
 *    CipInstance *const instance,
 *    CipMessageRouterRequest *message_router_request,
 *    CipMessageRouterResponse *message_router_response
 *  )
 *  @brief Signature definition of callback functions for CIP services
 *
 *  @param  instance  CIP instance involved in common services
 *  @param  message_router_request pointer to request.
 *  @param  message_router_response pointer to response.
 *  @return           status of kEipStatusOk or kEipStatusError on failure
 */
typedef EipStatus (*CipCallback)(CipInstance *const instance,
                                 const CipMessageRouterRequest *const
                                 message_router_request,
                                 CipMessageRouterResponse *const
                                 message_router_response);

/** @brief Type definition of CipClass that is a subclass of CipInstance */
typedef struct cip_class {
  CipInstance class_instance;   /**< This is the instance that contains the
                                   class attributes of this class. */
  /* the rest of these are specific to the Class class only. */
  CipUdint class_code;   /**< class code */
  EipUint16 revision;   /**< class revision*/
  EipUint16 max_instance;   /**< largest instance number existing in the class */
  EipUint16 number_of_instances;   /**< number of instances in the class (not
                                      including instance 0) */
  EipUint16 number_of_attributes;   /**< number of attributes of each instance */
  EipUint16 highest_attribute_number;   /**< highest defined attribute number
                                           (attribute numbers are not necessarily
                                           consecutive) */
  uint8_t *get_single_bit_mask;   /**< bit mask for GetAttributeSingle */
  uint8_t *set_bit_mask;   /**< bit mask for SetAttributeSingle */
  uint8_t *get_all_bit_mask;   /**< bit mask for GetAttributeAll */

  EipUint16 number_of_services;   /**< number of services supported */
  CipInstance *instances;   /**< pointer to the list of instances */
  struct cip_service_struct *services;   /**< pointer to the array of services */
  char *class_name;   /**< class name */
  /** Is called in GetAttributeSingle* before the response is assembled from
   * the object's attributes */
  CipGetSetCallback PreGetCallback;
  /** Is called in GetAttributeSingle* after the response has been sent. */
  CipGetSetCallback PostGetCallback;
  /** Is called in SetAttributeSingle* before the received data is moved
   * to the object's attributes */
  CipGetSetCallback PreSetCallback;
  /** Is called in SetAttributeSingle* after the received data was set
   * in the object's attributes. */
  CipGetSetCallback PostSetCallback;

  /** Is called in Create before the instance is created. */
  CipCallback PreCreateCallback;
  /** Is called in Create after the instance has been created. */
  CipCallback PostCreateCallback;
  /** Is called in Delete before the instance is deleted. */
  CipCallback PreDeleteCallback;
  /** Is called in Delete after the instance has been deleted. */
  CipCallback PostDeleteCallback;
  /** Is called in Reset service */
  CipCallback PreResetCallback;
  /** Is called in Reset service. */
  CipCallback PostResetCallback;

} CipClass;

/** @ingroup CIP_API
 *  @typedef  EipStatus (*CipServiceFunction)(CipInstance *const instance,
 *    CipMessageRouterRequest *const message_router_request,
 *    CipMessageRouterResponse *const message_router_response,
 *    const struct sockaddr *originator_address, const CipSessionHandle encapsulation_session)
 *  @brief Signature definition for the implementation of CIP services.
 *
 *  CIP services have to follow this signature in order to be handled correctly
 *   by the stack.
 *  @param instance the instance which was referenced in the service
 *   request
 *  @param message_router_request request data
 *  @param message_router_response storage for the response data, including a buffer for
 *   extended data
 *  @param originator_address  address of the originator as received from socket
 *  @param encapsulation_session associated encapsulation session of the explicit message
 *  @return kEipOkSend if service could be executed successfully and a response
 *   should be sent
 */
typedef EipStatus (*CipServiceFunction)(CipInstance *const instance,
                                        CipMessageRouterRequest *const
                                        message_router_request,
                                        CipMessageRouterResponse *const
                                        message_router_response,
                                        const struct sockaddr *
                                        originator_address,
                                        const CipSessionHandle encapsulation_session);

/** @brief Service descriptor. These are stored in an array */
typedef struct cip_service_struct {
  EipUint8 service_number;   /**< service number*/
  CipServiceFunction service_function;   /**< pointer to a function call*/
  char *name;   /**< name of the service */
} CipServiceStruct;

/**
 * @brief Struct for saving TCP/IP interface information
 *
 * All addresses are stored in network byte order.
 */
typedef struct {
  CipUdint ip_address;
  CipUdint network_mask;
  CipUdint gateway;
  CipUdint name_server;
  CipUdint name_server_2;
  CipString domain_name;
} CipTcpIpInterfaceConfiguration;

typedef struct {
  EipUint8 path_size;
  EipUint32 port;   /* support up to 32 bit path*/
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
