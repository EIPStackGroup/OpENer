/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPEPATH_H_
#define SRC_CIP_CIPEPATH_H_

#include <stdbool.h>

#include "ciptypes.h"

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

#define LOGICAL_SEGMENT_FORMAT_EIGHT_BIT_MESSAGE_VALUE 0x00
#define LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT_MESSAGE_VALUE 0x01
#define LOGICAL_SEGMENT_FORMAT_THIRTY_TWO_BIT_MESSAGE_VALUE 0x02

#define LOGICAL_SEGMENT_EXTENDED_TYPE_RESERVED_MESSAGE_VALUE 0x00
#define LOGICAL_SEGMENT_EXTENDED_TYPE_ARRAY_INDEX_MESSAGE_VALUE 0x01
#define LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_ARRAY_INDEX_MESSAGE_VALUE 0x02
#define LOGICAL_SEGMENT_EXTENDED_TYPE_BIT_INDEX_MESSAGE_VALUE 0x03
#define LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_BIT_INDEX_MESSAGE_VALUE 0x04
#define LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_NUMBER_MESSAGE_VALUE 0x05
#define LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_HANDLE_MESSAGE_VALUE 0x06

#define LOGICAL_SEGMENT_SPECIAL_TYPE_FORMAT_ELECTRONIC_KEY_MESSAGE_VALUE 0x00

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

SegmentType GetPathSegmentType(const char *const cip_path);

void SetPathSegmentType(SegmentType segment_type, char *const cip_path);

bool GetPathPortSegmentExtendedLinkAddressSizeBit(const char *const cip_path);

unsigned int GetPathPortSegmentPortIdentifier(const char *const cip_path);

void SetPathPortSegmentPortIdentifier(const unsigned int port_identifier,
                                      char *const cip_path);

unsigned int GetPathPortSegmentLinkAddressSize(const char *const cip_path);

unsigned int GetPathPortSegmentExtendedPortNumber(const char *const cip_path);

void SetPathPortSegmentExtendedPortIdentifier(
    const unsigned int extended_port_identifier, char *const cip_path);

LogicalSegmentLogicalType GetPathLogicalSegmentLogicalType(const char *const cip_path);

LogicalSegmentLogicalFormat GetPathLogicalSegmentLogicalFormat(
    const char *const cip_path);

LogicalSegmentExtendedLogicalType GetPathLogicalSegmentExtendedLogicalType(const char *const cip_path);

LogicalSegmentSpecialTypeLogicalFormat GetPathLogicalSegmentSpecialTypeLogicalType(const char *const cip_path);

#endif /* SRC_CIP_CIPEPATH_H_ */
