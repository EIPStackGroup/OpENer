/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <stdbool.h>
#include <stdlib.h>

#include "cipepath.h"

#include "endianconv.h"
#include "cipelectronickey.h"
#include "trace.h"

const unsigned int kPortSegmentExtendedPort = 15; /**< Reserved port segment port value, indicating the use of the extended port field */

/* Segments */
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

#define ELECTRONIC_KEY_SEGMENT_KEY_FORMAT_4_MESSAGE_VALUE 0x04

#define DATA_SEGMENT_SUBTYPE_SIMPLE_DATA_MESSAGE_VALUE 0x00
#define DATA_SEGMENT_SUBTYPE_ANSI_EXTENDED_SYMBOL_MESSAGE_VALUE 0x11



/*** Path Segment ***/
SegmentType GetPathSegmentType(const unsigned char *const cip_path) {
  const unsigned int kSegmentTypeMask = 0xE0;
  const unsigned int segment_type = *cip_path & kSegmentTypeMask;
  SegmentType result = kSegmentTypeReserved;
  switch (segment_type) {
    case SEGMENT_TYPE_PORT_SEGMENT_MESSAGE_VALUE:
      result = kSegmentTypePortSegment;
      break;
    case SEGMENT_TYPE_LOGICAL_SEGMENT_MESSAGE_VALUE:
      result = kSegmentTypeLogicalSegment;
      break;
    case SEGMENT_TYPE_NETWORK_SEGMENT_MESSAGE_VALUE:
      result = kSegmentTypeNetworkSegment;
      break;
    case SEGMENT_TYPE_SYMBOLIC_SEGMENT_MESSAGE_VALUE:
      result = kSegmentTypeSymbolicSegment;
      break;
    case SEGMENT_TYPE_DATA_SEGMENT_MESSAGE_VALUE:
      result = kSegmentTypeDataSegment;
      break;
    case SEGMENT_TYPE_DATA_TYPE_CONSTRUCTED_MESSAGE_VALUE:
      result = kSegmentTypeDataTypeConstructed;
      break;
    case SEGMENT_TYPE_DATA_TYPE_ELEMENTARTY_MESSAGE_VALUE:
      result = kSegmentTypeDataTypeElementary;
      break;
    case SEGMENT_TYPE_SEGMENT_RESERVED_MESSAGE_VALUE:
      result = kSegmentTypeReserved;
      break;
    default:
      OPENER_ASSERT(
          "Invalid Segment type in the message! We should never come here!\n");
      break;
  }
  return result;
}

void SetPathSegmentType(SegmentType segment_type, unsigned char *const cip_path) {
  switch (segment_type) {
    case kSegmentTypePortSegment:
      *cip_path |= SEGMENT_TYPE_PORT_SEGMENT_MESSAGE_VALUE;
      break;
    case kSegmentTypeLogicalSegment:
      *cip_path |= SEGMENT_TYPE_LOGICAL_SEGMENT_MESSAGE_VALUE;
      break;
    case kSegmentTypeNetworkSegment:
      *cip_path |= SEGMENT_TYPE_NETWORK_SEGMENT_MESSAGE_VALUE;
      break;
    case kSegmentTypeSymbolicSegment:
      *cip_path |= SEGMENT_TYPE_SYMBOLIC_SEGMENT_MESSAGE_VALUE;
      break;
    case kSegmentTypeDataSegment:
      *cip_path |= SEGMENT_TYPE_DATA_SEGMENT_MESSAGE_VALUE;
      break;
    case kSegmentTypeDataTypeConstructed:
      *cip_path |= SEGMENT_TYPE_DATA_TYPE_CONSTRUCTED_MESSAGE_VALUE;
      break;
    case kSegmentTypeDataTypeElementary:
      *cip_path |= SEGMENT_TYPE_DATA_TYPE_ELEMENTARTY_MESSAGE_VALUE;
      break;
    case kSegmentTypeReserved:
      *cip_path |= SEGMENT_TYPE_SEGMENT_RESERVED_MESSAGE_VALUE;
      break;
    default:
      OPENER_ASSERT(
          "Invalid Segment type chosen! We should never come here!\n");
  }
}

/*** Port Segment ***/
bool GetPathPortSegmentExtendedLinkAddressSizeBit(const unsigned char *const cip_path) {
  const unsigned int kExtendedLinkAddressSizeMask = 0x10;
  if (kExtendedLinkAddressSizeMask == (*cip_path & kExtendedLinkAddressSizeMask) ) {
    return true;
  }
  return false;
}

unsigned int GetPathPortSegmentPortIdentifier(const unsigned char *const cip_path) {
  const unsigned int kPortIdentifierMask = 0x0F;
  unsigned int port_identifier = *cip_path & kPortIdentifierMask;
//  OPENER_ASSERT(0 != port_identifier, "Use of reserved port identifier 0\n");
  OPENER_ASSERT(kSegmentTypePortSegment == GetPathSegmentType(cip_path));
  OPENER_ASSERT(0 != port_identifier);
  return port_identifier;
}

void SetPathPortSegmentPortIdentifier(const unsigned int port_identifier,
                                      unsigned char *const cip_path) {
//  OPENER_ASSERT(
//      port_identifier < 16,
//      "Port identifier too large for standard port identifier field\n");
  OPENER_ASSERT(port_identifier < 16);
  (*cip_path) |= port_identifier;
}

unsigned int GetPathPortSegmentLinkAddressSize(const unsigned char *const cip_path) {
//  OPENER_ASSERT(false == GetPathPortSegmentExtendedLinkAddressSizeBit(cip_path),
//                "Call to non existent extended link address size\n");
  OPENER_ASSERT(true == GetPathPortSegmentExtendedLinkAddressSizeBit(cip_path));
  return *(cip_path + 1);
}

unsigned int GetPathPortSegmentExtendedPortNumber(const unsigned char *const cip_path) {
//  OPENER_ASSERT(kPortSegmentExtendedPort == GetPathPortSegmentPortIdentifier(cip_path),
//                "There is no extended port available!\n");
  OPENER_ASSERT(kPortSegmentExtendedPort == GetPathPortSegmentPortIdentifier(cip_path));
  const unsigned int kExtendedPortSegmentPosition =
      GetPathPortSegmentExtendedLinkAddressSizeBit(cip_path) == true ? 2 : 1;
  return cip_path[kExtendedPortSegmentPosition]
      + (cip_path[kExtendedPortSegmentPosition + 1] << 8);
}

void SetPathPortSegmentExtendedPortIdentifier(
    const unsigned int extended_port_identifier, unsigned char *const cip_path) {
  SetPathPortSegmentPortIdentifier(kPortSegmentExtendedPort, cip_path);
  const unsigned int kExtendedPortSegmentPosition =
      GetPathPortSegmentExtendedLinkAddressSizeBit(cip_path) == true ? 2 : 1;
  cip_path[kExtendedPortSegmentPosition] = (char) (extended_port_identifier
      & 0x00FF);
  cip_path[kExtendedPortSegmentPosition + 1] =
      (char) ((extended_port_identifier & 0xFF00) >> 8);
}
/*** Port Segment ***/

/*** Logical Segment ***/

LogicalSegmentLogicalType GetPathLogicalSegmentLogicalType(
    const unsigned char *const cip_path) {
  OPENER_ASSERT(kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path));
  const unsigned int kLogicalTypeMask = 0x1C;
  const unsigned int logical_type = (*cip_path) & kLogicalTypeMask;
  LogicalSegmentLogicalType result = kLogicalSegmentLogicalTypeExtendedLogical;
  switch (logical_type) {
    case LOGICAL_SEGMENT_TYPE_CLASS_ID_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalTypeClassId;
      break;
    case LOGICAL_SEGMENT_TYPE_INSTANCE_ID_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalTypeInstanceId;
      break;
    case LOGICAL_SEGMENT_TYPE_MEMBER_ID_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalTypeMemberId;
      break;
    case LOGICAL_SEGMENT_TYPE_CONNECTION_POINT_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalTypeConnectionPoint;
      break;
    case LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalTypeAttributeId;
      break;
    case LOGICAL_SEGMENT_TYPE_SPECIAL_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalTypeSpecial;
      break;
    case LOGICAL_SEGMENT_TYPE_SERVICE_ID_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalTypeServiceId;
      break;
    case LOGICAL_SEGMENT_TYPE_EXTENDED_LOGICAL_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalTypeExtendedLogical;
      break;
    default:
      OPENER_ASSERT(
          "Logical segment/logical type: It is not possible to reach this point!\n");
      break;
  }
  return result;
}

LogicalSegmentLogicalFormat GetPathLogicalSegmentLogicalFormat(
    const unsigned char *const cip_path) {
  OPENER_ASSERT(kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path));
  const unsigned int kLogicalFormatMask = 0x03;
  const unsigned int logical_format = (*cip_path) & kLogicalFormatMask;
  LogicalSegmentLogicalFormat result = kLogicalSegmentLogicalFormatEightBit;
  switch (logical_format) {
    case LOGICAL_SEGMENT_FORMAT_EIGHT_BIT_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalFormatEightBit;
      break;
    case LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalFormatSixteenBit;
      break;
    case LOGICAL_SEGMENT_FORMAT_THIRTY_TWO_BIT_MESSAGE_VALUE:
      result = kLogicalSegmentLogicalFormatThirtyTwoBit;
      break;
    default:
      OPENER_ASSERT(
          "Logical segment/logical type: Invalid logical type detected!\n");
      break;
  }
  return result;
}

LogicalSegmentExtendedLogicalType GetPathLogicalSegmentExtendedLogicalType(const unsigned char *const cip_path) {
//  OPENER_ASSERT(LOGICAL_SEGMENT_TYPE_EXTENDED_kLogicalSegmentLogicalTypeExtendedLogicalMessageValue == GetPathLogicalSegmentLogicalType(cip_path),
//                "Trying to extract non-existent extended logical type");
  OPENER_ASSERT(kLogicalSegmentLogicalTypeExtendedLogical == GetPathLogicalSegmentLogicalType(cip_path));
  const unsigned int extended_logical_type = *(cip_path + 1);
  LogicalSegmentExtendedLogicalType result = kLogicalSegmentExtendedLogicalTypeReserved;
  switch(extended_logical_type) {
    case LOGICAL_SEGMENT_EXTENDED_TYPE_ARRAY_INDEX_MESSAGE_VALUE: result = kLogicalSegmentExtendedLogicalTypeArrayIndex; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_ARRAY_INDEX_MESSAGE_VALUE: result = kLogicalSegmentExtendedLogicalTypeIndirectArrayIndex; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_BIT_INDEX_MESSAGE_VALUE: result = kLogicalSegmentExtendedLogicalTypeBitIndex; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_BIT_INDEX_MESSAGE_VALUE: result = kLogicalSegmentExtendedLogicalTypeIndirectBitIndex; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_NUMBER_MESSAGE_VALUE: result = kLogicalSegmentExtendedLogicalTypeStructureMemberNumber; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_HANDLE_MESSAGE_VALUE: result = kLogicalSegmentExtendedLogicalTypeStructureMemberHandle; break;
    default: result = kLogicalSegmentExtendedLogicalTypeReserved;
  }
  return result;
}

LogicalSegmentSpecialTypeLogicalFormat GetPathLogicalSegmentSpecialTypeLogicalType(const unsigned char *const cip_path) {
//  OPENER_ASSERT(kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path), "Not a logical segment!\n");
  OPENER_ASSERT(kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path));
  OPENER_ASSERT(kLogicalSegmentLogicalTypeSpecial == GetPathLogicalSegmentLogicalType(cip_path));
  const unsigned int kLogicalFormatMask = 0x03;
  const unsigned int logical_format = (*cip_path) & kLogicalFormatMask;

  LogicalSegmentSpecialTypeLogicalFormat result = kLogicalSegmentSpecialTypeLogicalFormatReserved;
  switch(logical_format) {
    case LOGICAL_SEGMENT_SPECIAL_TYPE_FORMAT_ELECTRONIC_KEY_MESSAGE_VALUE:
      result = kLogicalSegmentSpecialTypeLogicalFormatElectronicKey; break;
    default: result = kLogicalSegmentSpecialTypeLogicalFormatReserved; break;
  }

  return result;
}

ElectronicKeySegmentFormat GetPathLogicalSegmentElectronicKeyFormat(const unsigned char *const cip_path) {
//  OPENER_ASSERT(kLogicalSegmentSpecialTypeLogicalFormatElectronicKey ==
//      GetPathLogicalSegmentSpecialTypeLogicalType(cip_path), "Not an electronic key!\n");
  OPENER_ASSERT(kLogicalSegmentSpecialTypeLogicalFormatElectronicKey ==
        GetPathLogicalSegmentSpecialTypeLogicalType(cip_path));
  ElectronicKeySegmentFormat result = kElectronicKeySegmentFormatReserved;
  switch(*(cip_path + 1)) {
    case ELECTRONIC_KEY_SEGMENT_KEY_FORMAT_4_MESSAGE_VALUE: result = kElectronicKeySegmentFormatKeyFormat4; break;
    default: result = kElectronicKeySegmentFormatReserved; break;
  }
  return result;
}

void GetPathLogicalSegmentElectronicKeyFormat4(const unsigned char *const cip_path, ElectronicKeyFormat4 *key) {
//  OPENER_ASSERT(kElectronicKeySegmentFormatKeyFormat4 ==
//      GetPathLogicalSegmentElectronicKeyFormat(cip_path), "Not electronic key format 4!\n");
  OPENER_ASSERT(kElectronicKeySegmentFormatKeyFormat4 ==
        GetPathLogicalSegmentElectronicKeyFormat(cip_path));

  const char *message_runner = (const char *)(cip_path + 2);
  SetElectronicKeyFormat4VendorId(GetIntFromMessage(&message_runner), key);
  SetElectronicKeyFormat4DeviceType(GetIntFromMessage(&message_runner), key);
  SetElectronicKeyFormat4ProductCode(GetIntFromMessage(&message_runner), key);
  SetElectronicKeyFormat4MajorRevisionCompatibility(GetSintFromMessage(&message_runner), key);
  SetElectronicKeyFormat4MinorRevision(GetSintFromMessage(&message_runner), key);
}

/*** Logical Segment ***/


/*** Network Segment ***/

/** @brief Return the Network Segment subtype
 *
 *  @param cip_path Pointer to the start of the EPath message
 *  @return The Network Segment subtype of the EPath
 */
NetworkSegmentSubType GetPathNetworkSegmentSubtype(const unsigned char *const cip_path) {
  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path));
  const unsigned int kSubtypeMask = 0x1F;
  const unsigned int subtype = (*cip_path) & kSubtypeMask;
  NetworkSegmentSubType result = kNetworkSegmentSubtypeReserved;
  switch(subtype) {
    case NETWORK_SEGMENT_SUBTYPE_SCHEDULE_MESSAGE_VALUE:
      result = kNetworkSegmentSubtypeScheduleSegment; break;
    case NETWORK_SEGMENT_SUBTYPE_FIXED_TAG_MESSAGE_VALUE:
      result = kNetworkSegmentSubtypeFixedTagSegment; break;
    case NETWORK_SEGMENT_SUBTYPE_PRODUCTION_INHIBIT_TIME_IN_MILLISECONDS_MESSAGE_VALUE:
      result = kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds; break;
    case NETWORK_SEGMENT_SUBTYPE_SAFETY_MESSAGE_VALUE:
      result = kNetworkSegmentSubtypeSafetySegment; break;
    case NETWORK_SEGMENT_SUBTYPE_PRODUCTION_INHIBIT_TIME_IN_MICROSECONDS_MESSAGE_VALUE:
      result = kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds; break;
    case NETWORK_SEGMENT_SUBTYPE_EXTENDED_NETWORK_MESSAGE_VALUE:
          result = kNetworkSegmentSubtypeExtendedNetworkSegment; break;
    default: result = kNetworkSegmentSubtypeReserved; break;
  }

  return result;
}

/**
 * @brief Return the Production Inhibit Time in milliseconds from an EPath
 *
 * @param cip_path Pointer to the start of the EPath message
 * @return the Production Inhibit Time in milliseconds ranging from 0 to 255
 */
CipUsint GetPathNetworkSegmentProductionInhibitTimeInMilliseconds(const unsigned char *const cip_path) {
//  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path),"Not a network segment!\n");
//  OPENER_ASSERT(kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds == GetPathNetworkSegmentSubtype(cip_path),
//                "Not a Production Inhibit Time milliseconds segment!\n");
  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path));
  OPENER_ASSERT(kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds == GetPathNetworkSegmentSubtype(cip_path));
  return *(cip_path + 1);
}

/**
 * @brief Return the Production Inhibit Time in microseconds from an EPath
 *
 * @param cip_path Pointer to the start of the EPath message
 * @return the Production Inhibit Time in microseconds ranging from 0 to 4294967295
 */
CipUdint GetPathNetworkSegmentProductionInhibitTimeInMicroseconds(const unsigned char *const cip_path) {
//  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path),"Not a network segment!\n");
//  OPENER_ASSERT(kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds == GetPathNetworkSegmentSubtype(cip_path),
//                  "Not a Production Inhibit Time microseconds segment!\n");
//  OPENER_ASSERT(2 == *(cip_path + 1), "Data Words length is incorrect! See CIP Spec Vol.1 C-1.4.3.3.2\n");

  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path));
  OPENER_ASSERT(kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds == GetPathNetworkSegmentSubtype(cip_path));
  OPENER_ASSERT(2 == *(cip_path + 1));

  const unsigned char *message_runner = cip_path + 2;
  return GetDintFromMessage(&message_runner);
}

/*** Network Segment ***/

/*** Symbolic Segment ***/

/* Currently not supported */

/*** Symbolic Segment ***/

/*** Data Segment ***/

DataSegmentSubtype GetPathDataSegmentSubtype(const unsigned char *const cip_path) {
  const unsigned int kDataSegmentSubtypeMask = 0x1F;
  const unsigned int data_subtype = (*cip_path) & kDataSegmentSubtypeMask;

  DataSegmentSubtype result = kDataSegmentSubtypeReserved;
  switch(data_subtype) {
    case DATA_SEGMENT_SUBTYPE_SIMPLE_DATA_MESSAGE_VALUE:
      result = kDataSegmentSubtypeSimpleData; break;
    case DATA_SEGMENT_SUBTYPE_ANSI_EXTENDED_SYMBOL_MESSAGE_VALUE:
      result = kDataSegmentSubtypeANSIExtendedSymbol; break;
    default: result = kDataSegmentSubtypeReserved; break;
  }
  return result;
}

/** @brief Returns the amount of 16-bit data words in the Simple Data EPath
 *
 * @param cip_path Pointer to the start of the EPath message
 * @return The amount of 16-bit words of data in the EPath
 */
CipUsint GetPathDataSegmentSimpleDataWordLength(const unsigned char *const cip_path) {
//  OPENER_ASSERT(kSegmentTypeDataSegment == GetPathSegmentType(cip_path),"Not a data segment!\n");
//  OPENER_ASSERT(kDataSegmentSubtypeSimpleData == GetPathDataSegmentSubtype(cip_path), "Not a simple data segment!\n");
  OPENER_ASSERT(kSegmentTypeDataSegment == GetPathSegmentType(cip_path));
  OPENER_ASSERT(
      kDataSegmentSubtypeSimpleData == GetPathDataSegmentSubtype(cip_path));

  const unsigned char *message_runner = cip_path + 1;
  return GetSintFromMessage(&message_runner);
}

/*** Data Segment ***/

