/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPEPATH_H_
#define SRC_CIP_CIPEPATH_H_

#include <stdbool.h>

#include "ciptypes.h"
#include "cipelectronickey.h"

#define SEGMENT_TYPE_PORT_SEGMENT_MESSAGE_VALUE 0x00 /**< Message value of the Port segment */
#define SEGMENT_TYPE_LOGICAL_SEGMENT_MESSAGE_VALUE 0x20 /**< Message value of the Logical segment */
#define SEGMENT_TYPE_NETWORK_SEGMENT_MESSAGE_VALUE 0x40 /**< Message value of the Network segment */
#define SEGMENT_TYPE_SYMBOLIC_SEGMENT_MESSAGE_VALUE 0x60 /**< Message value of the Symbolic segment */
#define SEGMENT_TYPE_DATA_SEGMENT_MESSAGE_VALUE 0x80 /**< Message value of the Data segment */
#define SEGMENT_TYPE_DATA_TYPE_CONSTRUCTED_MESSAGE_VALUE 0xA0 /**< Message value of the Data type constructed */
#define SEGMENT_TYPE_DATA_TYPE_ELEMENTARTY_MESSAGE_VALUE 0xC0 /**< Message value of the Data type elementary */
#define SEGMENT_TYPE_SEGMENT_RESERVED_MESSAGE_VALUE 0xE0 /**< Reserved value */

#define LOGICAL_SEGMENT_TYPE_CLASS_ID_MESSAGE_VALUE 0x00 /**< Message value of the logical segment/logical type Class ID */
#define LOGICAL_SEGMENT_TYPE_INSTANCE_ID_MESSAGE_VALUE 0x04 /**< Message value of the logical segment/logical type Instance ID */
#define LOGICAL_SEGMENT_TYPE_MEMBER_ID_MESSAGE_VALUE 0x08 /**< Message value of the logical segment/logical type Member ID */
#define LOGICAL_SEGMENT_TYPE_CONNECTION_POINT_MESSAGE_VALUE 0x0C /**< Message value of the logical segment/logical type Connection Point */
#define LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID_MESSAGE_VALUE 0x10 /**< Message value of the logical segment/logical type Attribute ID */
#define LOGICAL_SEGMENT_TYPE_SPECIAL_MESSAGE_VALUE 0x14 /**< Message value of the logical segment/logical type Special */
#define LOGICAL_SEGMENT_TYPE_SERVICE_ID_MESSAGE_VALUE 0x18 /**< Message value of the logical segment/logical type Service ID */
#define LOGICAL_SEGMENT_TYPE_EXTENDED_LOGICAL_MESSAGE_VALUE 0x1C /**< Message value of the logical segment/logical type Extended Logical */

#define LOGICAL_SEGMENT_FORMAT_EIGHT_BIT_MESSAGE_VALUE 0x00 /**< Message value indicating an 8 bit value */
#define LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT_MESSAGE_VALUE 0x01 /**< Message value indicating an 16 bit value */
#define LOGICAL_SEGMENT_FORMAT_THIRTY_TWO_BIT_MESSAGE_VALUE 0x02 /**< Message value indicating an 32 bit value */

#define LOGICAL_SEGMENT_EXTENDED_TYPE_RESERVED_MESSAGE_VALUE 0x00 /**< Message value indicating an reserved/unused Extended Logical Segment type */
#define LOGICAL_SEGMENT_EXTENDED_TYPE_ARRAY_INDEX_MESSAGE_VALUE 0x01 /**< Message value indicating the Array Index Extended Logical Segment type */
#define LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_ARRAY_INDEX_MESSAGE_VALUE 0x02 /**< Message value indicating the Indirect Array Index Extended Logical Segment type */
#define LOGICAL_SEGMENT_EXTENDED_TYPE_BIT_INDEX_MESSAGE_VALUE 0x03 /**< Message value indicating the Bit Index Extended Logical Segment type */
#define LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_BIT_INDEX_MESSAGE_VALUE 0x04 /**< Message value indicating the Indirect Bit Index Extended Logical Segment type */
#define LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_NUMBER_MESSAGE_VALUE 0x05 /**< Message value indicating the Structured Member Number Extended Logical Segment type */
#define LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_HANDLE_MESSAGE_VALUE 0x06 /**< Message value indicating the Structured Member Handler Extended Logical Segment type */

#define LOGICAL_SEGMENT_SPECIAL_TYPE_FORMAT_ELECTRONIC_KEY_MESSAGE_VALUE 0x00 /**< Message value indicating an electronic key */

#define NETWORK_SEGMENT_SUBTYPE_SCHEDULE_MESSAGE_VALUE 0x01
#define NETWORK_SEGMENT_SUBTYPE_FIXED_TAG_MESSAGE_VALUE 0x02
#define NETWORK_SEGMENT_SUBTYPE_PRODUCTION_INHIBIT_TIME_IN_MILLISECONDS_MESSAGE_VALUE 0x03
#define NETWORK_SEGMENT_SUBTYPE_SAFETY_MESSAGE_VALUE 0x04
#define NETWORK_SEGMENT_SUBTYPE_PRODUCTION_INHIBIT_TIME_IN_MICROSECONDS_MESSAGE_VALUE 0x10
#define NETWORK_SEGMENT_SUBTYPE_EXTENDED_NETWORK_MESSAGE_VALUE 0x1F

typedef enum network_segment_subtype {
  kNetworkSegmentSubtypeReserved,
  kNetworkSegmentSubtypeScheduleSegment,
  kNetworkSegmentSubtypeFixedTagSegment,
  kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds,
  kNetworkSegmentSubtypeSafetySegment,
  kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds,
  kNetworkSegmentSubtypeExtendedNetworkSegment
} NetworkSegmentSubtype;

typedef enum electronic_key_segment_format {
  kElectronicKeySegmentFormatReserved,
  kElectronicKeySegmentFormatKeyFormat4
} ElectronicKeySegmentFormat;

typedef enum data_segment_subtype {
  kDataSegmentSubtypeReserved,
  kDataSegmentSubtypeSimpleData,
  kDataSegmentSubtypeANSIExtendedSymbol
} DataSegmentSubtype;

typedef enum symbolic_segment_format {
  kSymbolicSegmentFormatASCII,
  kSymbolicSegmentFormatExtendedString
} SymbolicSegmentFormat;

typedef enum symbolic_segment_extended_format {
  kSymbolicSegmentExtendedFormatDoubleByteChars,
  kSymbolicSegmentExtendedFormatTripleByteChars,
  kSymbolicSegmentExtendedFormatNumericSymbolUSINT,
  kSymbolicSegmentExtendedFormatNumericSymbolUINT,
  kSymbolicSegmentExtendedFormatNumericSymbolUDINT,
  kSymbolicSegmentExtendedFormatReserved
} SymbolicSegmentExtendedFormat;

/** @brief Gets the basic segment type of a CIP EPath
 *
 * @param cip_path The start of the EPath message
 * @return The basic segment type
 */
SegmentType GetPathSegmentType(const unsigned char *const cip_path);

/** @brief Sets the basic segment type of an CIP EPath to be sent
 *
 * @param segment_type The segment type
 * @param cip_path A message buffer - Will be written on!
 */
void SetPathSegmentType(SegmentType segment_type, unsigned char *const cip_path);

/*********************************************************
 * Port Segment functions
 *********************************************************/

/** @brief Only to be used on Port Segments. Returns if the Port Segment has the extended link address size bit set
 *
 * @param cip_path The start of the EPath message
 * @return True if extended link addres size bit set, false otherwise
 */
bool GetPathPortSegmentExtendedLinkAddressSizeBit(const unsigned char *const cip_path);

/** @brief Only to be used on Port Segments. Returns the Port Identifier
 *
 * @param cip_path The start of the EPath message
 * @return The Port Identifier
 */
unsigned int GetPathPortSegmentPortIdentifier(const unsigned char *const cip_path);

/** @brief Sets the Port Identifier form an Port Segment EPath to be sent.
 *
 * @param port_identifier The port identifier
 * @param cip_path A message buffer - Will be written on!
 */
void SetPathPortSegmentPortIdentifier(const unsigned int port_identifier,
                                      unsigned char *const cip_path);

/** @brief Only to be used on Port Segments. Gets the Link Address Size
 *
 * @param cip_path The start of the EPath message
 * @return The Link Address Size
 */
unsigned int GetPathPortSegmentLinkAddressSize(const unsigned char *const cip_path);

/** @brief Only to be used on Port Segments with Extended Port Number. Gets the Extended Port Number
 *
 * @param cip_path The start of the EPath message
 * @return The Link Address Size
 */
unsigned int GetPathPortSegmentExtendedPortNumber(const unsigned char *const cip_path);

void SetPathPortSegmentExtendedPortIdentifier(
    const unsigned int extended_port_identifier, unsigned char *const cip_path);

LogicalSegmentLogicalType GetPathLogicalSegmentLogicalType(const unsigned char *const cip_path);

LogicalSegmentLogicalFormat GetPathLogicalSegmentLogicalFormat(
    const unsigned char *const cip_path);

LogicalSegmentExtendedLogicalType GetPathLogicalSegmentExtendedLogicalType(const unsigned char *const cip_path);

LogicalSegmentSpecialTypeLogicalFormat GetPathLogicalSegmentSpecialTypeLogicalType(const unsigned char *const cip_path);

ElectronicKeySegmentFormat GetPathLogicalSegmentElectronicKeyFormat(const unsigned char *const cip_path);

void GetPathLogicalSegmentElectronicKeyFormat4(const unsigned char *const cip_path, ElectronicKeyFormat4 *key);

NetworkSegmentSubtype GetPathNetworkSegmentSubtype(const unsigned char *const cip_path);

CipUsint GetPathNetworkSegmentProductionInhibitTimeInMilliseconds(const unsigned char *const cip_path);

CipUdint GetPathNetworkSegmentProductionInhibitTimeInMicroseconds(const unsigned char *const cip_path);

DataSegmentSubtype GetPathDataSegmentSubtype(const unsigned char *const cip_path);

CipUsint GetPathDataSegmentSimpleDataWordLength(const unsigned char *const cip_path);

SymbolicSegmentFormat GetPathSymbolicSegmentFormat(const unsigned char *const cip_path);

SymbolicSegmentExtendedFormat GetPathSymbolicSegmentNumericType(const unsigned char *const cip_path);

SymbolicSegmentExtendedFormat GetPathSymbolicSegmentExtendedFormat(const unsigned char *const cip_path);

#endif /* SRC_CIP_CIPEPATH_H_ */
