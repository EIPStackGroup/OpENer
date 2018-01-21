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
#define SEGMENT_TYPE_PORT_SEGMENT 0x00 /**< Message value of the Port segment */
#define SEGMENT_TYPE_LOGICAL_SEGMENT 0x20 /**< Message value of the Logical segment */
#define SEGMENT_TYPE_NETWORK_SEGMENT 0x40 /**< Message value of the Network segment */
#define SEGMENT_TYPE_SYMBOLIC_SEGMENT 0x60 /**< Message value of the Symbolic segment */
#define SEGMENT_TYPE_DATA_SEGMENT 0x80 /**< Message value of the Data segment */
#define SEGMENT_TYPE_DATA_TYPE_CONSTRUCTED 0xA0 /**< Message value of the Data type constructed */
#define SEGMENT_TYPE_DATA_TYPE_ELEMENTARTY 0xC0 /**< Message value of the Data type elementary */
#define SEGMENT_TYPE_SEGMENT_RESERVED 0xE0 /**< Reserved value */

#define LOGICAL_SEGMENT_TYPE_CLASS_ID 0x00 /**< Message value of the logical segment/logical type Class ID */
#define LOGICAL_SEGMENT_TYPE_INSTANCE_ID 0x04 /**< Message value of the logical segment/logical type Instance ID */
#define LOGICAL_SEGMENT_TYPE_MEMBER_ID 0x08 /**< Message value of the logical segment/logical type Member ID */
#define LOGICAL_SEGMENT_TYPE_CONNECTION_POINT 0x0C /**< Message value of the logical segment/logical type Connection Point */
#define LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID 0x10 /**< Message value of the logical segment/logical type Attribute ID */
#define LOGICAL_SEGMENT_TYPE_SPECIAL 0x14 /**< Message value of the logical segment/logical type Special */
#define LOGICAL_SEGMENT_TYPE_SERVICE_ID 0x18 /**< Message value of the logical segment/logical type Service ID */
#define LOGICAL_SEGMENT_TYPE_EXTENDED_LOGICAL 0x1C /**< Message value of the logical segment/logical type Extended Logical */

#define LOGICAL_SEGMENT_FORMAT_EIGHT_BIT 0x00
#define LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT 0x01
#define LOGICAL_SEGMENT_FORMAT_THIRTY_TWO_BIT 0x02

#define LOGICAL_SEGMENT_EXTENDED_TYPE_RESERVED 0x00
#define LOGICAL_SEGMENT_EXTENDED_TYPE_ARRAY_INDEX 0x01
#define LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_ARRAY_INDEX 0x02
#define LOGICAL_SEGMENT_EXTENDED_TYPE_BIT_INDEX 0x03
#define LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_BIT_INDEX 0x04
#define LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_NUMBER 0x05
#define LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_HANDLE 0x06

#define LOGICAL_SEGMENT_SPECIAL_TYPE_FORMAT_ELECTRONIC_KEY 0x00
#define ELECTRONIC_KEY_SEGMENT_KEY_FORMAT_4 0x04

#define NETWORK_SEGMENT_SCHEDULE 0x01
#define NETWORK_SEGMENT_FIXED_TAG 0x02
#define NETWORK_SEGMENT_PRODUCTION_INHIBIT_TIME_IN_MILLISECONDS 0x03
#define NETWORK_SEGMENT_SAFETY 0x04
#define NETWORK_SEGMENT_PRODUCTION_INHIBIT_TIME_IN_MICROSECONDS 0x10
#define NETWORK_SEGMENT_EXTENDED_NETWORK 0x1F

#define SYMBOLIC_SEGMENT_FORMAT_EXTENDED_STRING 0x00

#define SYMBOLIC_SEGMENT_EXTENDED_FORMAT_DOUBLE_CHAR 0x20
#define SYMBOLIC_SEGMENT_EXTENDED_FORMAT_TRIPLE_CHAR 0x40
#define SYMBOLIC_SEGMENT_EXTENDED_FORMAT_NUMERIC 0xC0

#define SYMBOLIC_SEGMENT_EXTENDED_FORMAT_NUMERIC_USINT_TYPE 0x06
#define SYMBOLIC_SEGMENT_EXTENDED_FORMAT_NUMERIC_UINT_TYPE 0x07
#define SYMBOLIC_SEGMENT_EXTENDED_FORMAT_NUMERIC_UDINT_TYPE 0x08

#define DATA_SEGMENT_SUBTYPE_SIMPLE_DATA 0x00
#define DATA_SEGMENT_SUBTYPE_ANSI_EXTENDED_SYMBOL 0x11



/*** Path Segment ***/
SegmentType GetPathSegmentType(const CipOctet *const cip_path) {
  const unsigned int kSegmentTypeMask = 0xE0;
  const unsigned int segment_type = *cip_path & kSegmentTypeMask;
  SegmentType result = kSegmentTypeReserved;
  switch (segment_type) {
    case SEGMENT_TYPE_PORT_SEGMENT:
      result = kSegmentTypePortSegment;
      break;
    case SEGMENT_TYPE_LOGICAL_SEGMENT:
      result = kSegmentTypeLogicalSegment;
      break;
    case SEGMENT_TYPE_NETWORK_SEGMENT:
      result = kSegmentTypeNetworkSegment;
      break;
    case SEGMENT_TYPE_SYMBOLIC_SEGMENT:
      result = kSegmentTypeSymbolicSegment;
      break;
    case SEGMENT_TYPE_DATA_SEGMENT:
      result = kSegmentTypeDataSegment;
      break;
    case SEGMENT_TYPE_DATA_TYPE_CONSTRUCTED:
      result = kSegmentTypeDataTypeConstructed;
      break;
    case SEGMENT_TYPE_DATA_TYPE_ELEMENTARTY:
      result = kSegmentTypeDataTypeElementary;
      break;
    case SEGMENT_TYPE_SEGMENT_RESERVED:
      result = kSegmentTypeReserved;
      break;
    default:
      OPENER_ASSERT(
        "Invalid Segment type in the message! We should never come here!\n");
      break;
  }
  return result;
}

void SetPathSegmentType(SegmentType segment_type,
                        unsigned char *const cip_path) {
  switch (segment_type) {
    case kSegmentTypePortSegment:
      *cip_path |= SEGMENT_TYPE_PORT_SEGMENT;
      break;
    case kSegmentTypeLogicalSegment:
      *cip_path |= SEGMENT_TYPE_LOGICAL_SEGMENT;
      break;
    case kSegmentTypeNetworkSegment:
      *cip_path |= SEGMENT_TYPE_NETWORK_SEGMENT;
      break;
    case kSegmentTypeSymbolicSegment:
      *cip_path |= SEGMENT_TYPE_SYMBOLIC_SEGMENT;
      break;
    case kSegmentTypeDataSegment:
      *cip_path |= SEGMENT_TYPE_DATA_SEGMENT;
      break;
    case kSegmentTypeDataTypeConstructed:
      *cip_path |= SEGMENT_TYPE_DATA_TYPE_CONSTRUCTED;
      break;
    case kSegmentTypeDataTypeElementary:
      *cip_path |= SEGMENT_TYPE_DATA_TYPE_ELEMENTARTY;
      break;
    case kSegmentTypeReserved:
      *cip_path |= SEGMENT_TYPE_SEGMENT_RESERVED;
      break;
    default:
      OPENER_ASSERT(
        "Invalid Segment type chosen! We should never come here!\n");
  }
}

/*** Port Segment ***/
bool GetPathPortSegmentExtendedLinkAddressSizeBit(
  const unsigned char *const cip_path) {
  const unsigned int kExtendedLinkAddressSizeMask = 0x10;
  if ( kExtendedLinkAddressSizeMask ==
       (*cip_path & kExtendedLinkAddressSizeMask) ) {
    return true;
  }
  return false;
}

unsigned int GetPathPortSegmentPortIdentifier(
  const unsigned char *const cip_path) {
  const unsigned int kPortIdentifierMask = 0x0F;
  unsigned int port_identifier = *cip_path & kPortIdentifierMask;
//  OPENER_ASSERT(0 != port_identifier, "Use of reserved port identifier 0\n");
  OPENER_ASSERT( kSegmentTypePortSegment == GetPathSegmentType(cip_path) );
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

unsigned int GetPathPortSegmentLinkAddressSize(
  const unsigned char *const cip_path) {
//  OPENER_ASSERT(false == GetPathPortSegmentExtendedLinkAddressSizeBit(cip_path),
//                "Call to non existent extended link address size\n");
  OPENER_ASSERT( true ==
                 GetPathPortSegmentExtendedLinkAddressSizeBit(cip_path) );
  return *(cip_path + 1);
}

unsigned int GetPathPortSegmentExtendedPortNumber(
  const unsigned char *const cip_path) {
//  OPENER_ASSERT(kPortSegmentExtendedPort == GetPathPortSegmentPortIdentifier(cip_path),
//                "There is no extended port available!\n");
  OPENER_ASSERT( kPortSegmentExtendedPort ==
                 GetPathPortSegmentPortIdentifier(cip_path) );
  const unsigned int kExtendedPortSegmentPosition =
    GetPathPortSegmentExtendedLinkAddressSizeBit(cip_path) == true ? 2 : 1;
  return cip_path[kExtendedPortSegmentPosition]
         + (cip_path[kExtendedPortSegmentPosition + 1] << 8);
}

void SetPathPortSegmentExtendedPortIdentifier(
  const unsigned int extended_port_identifier,
  CipOctet *const cip_path) {
  SetPathPortSegmentPortIdentifier(kPortSegmentExtendedPort, cip_path);
  const unsigned int kExtendedPortSegmentPosition =
    GetPathPortSegmentExtendedLinkAddressSizeBit(cip_path) == true ? 2 : 1;
  cip_path[kExtendedPortSegmentPosition] = (char) (extended_port_identifier
                                                   & 0x00FF);
  cip_path[kExtendedPortSegmentPosition + 1] =
    (char) ( (extended_port_identifier & 0xFF00) >> 8 );
}
/*** Port Segment ***/

/*** Logical Segment ***/

LogicalSegmentLogicalType GetPathLogicalSegmentLogicalType(
  const unsigned char *const cip_path) {
  OPENER_ASSERT( kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path) );
  const unsigned int kLogicalTypeMask = 0x1C;
  const unsigned int logical_type = (*cip_path) & kLogicalTypeMask;
  LogicalSegmentLogicalType result = kLogicalSegmentLogicalTypeExtendedLogical;
  switch (logical_type) {
    case LOGICAL_SEGMENT_TYPE_CLASS_ID:
      result = kLogicalSegmentLogicalTypeClassId;
      break;
    case LOGICAL_SEGMENT_TYPE_INSTANCE_ID:
      result = kLogicalSegmentLogicalTypeInstanceId;
      break;
    case LOGICAL_SEGMENT_TYPE_MEMBER_ID:
      result = kLogicalSegmentLogicalTypeMemberId;
      break;
    case LOGICAL_SEGMENT_TYPE_CONNECTION_POINT:
      result = kLogicalSegmentLogicalTypeConnectionPoint;
      break;
    case LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID:
      result = kLogicalSegmentLogicalTypeAttributeId;
      break;
    case LOGICAL_SEGMENT_TYPE_SPECIAL:
      result = kLogicalSegmentLogicalTypeSpecial;
      break;
    case LOGICAL_SEGMENT_TYPE_SERVICE_ID:
      result = kLogicalSegmentLogicalTypeServiceId;
      break;
    case LOGICAL_SEGMENT_TYPE_EXTENDED_LOGICAL:
      result = kLogicalSegmentLogicalTypeExtendedLogical;
      break;
    default:
      OPENER_ASSERT(
        "Logical segment/logical type: It is not possible to reach this point!\n");
      break;
  }
  return result;
}

void SetPathLogicalSegmentLogicalType(LogicalSegmentLogicalType logical_type,
                                      CipOctet *const cip_path) {
  OPENER_ASSERT( kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path) );
  switch (logical_type) {
    case kLogicalSegmentLogicalTypeClassId:
      (*cip_path) |= LOGICAL_SEGMENT_TYPE_CLASS_ID;
      break;
    case kLogicalSegmentLogicalTypeInstanceId:
      (*cip_path) |= LOGICAL_SEGMENT_TYPE_INSTANCE_ID;
      break;
    case kLogicalSegmentLogicalTypeMemberId:
      (*cip_path) |= LOGICAL_SEGMENT_TYPE_MEMBER_ID;
      break;
    case kLogicalSegmentLogicalTypeConnectionPoint:
      (*cip_path) |= LOGICAL_SEGMENT_TYPE_CONNECTION_POINT;
      break;
    case kLogicalSegmentLogicalTypeAttributeId:
      (*cip_path) |= LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID;
      break;
    case kLogicalSegmentLogicalTypeSpecial:
      (*cip_path) |= LOGICAL_SEGMENT_TYPE_SPECIAL;
      break;
    case kLogicalSegmentLogicalTypeServiceId:
      (*cip_path) |= LOGICAL_SEGMENT_TYPE_SERVICE_ID;
      break;
    case kLogicalSegmentLogicalTypeExtendedLogical:
      (*cip_path) |= LOGICAL_SEGMENT_TYPE_EXTENDED_LOGICAL;
      break;
    default:
      OPENER_ASSERT(
        "Logical segment/logical type: It is not possible to reach this point!\n");
      break;
  }
}

LogicalSegmentLogicalFormat GetPathLogicalSegmentLogicalFormat(
  const unsigned char *const cip_path) {
  OPENER_ASSERT( kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path) );
  const unsigned int kLogicalFormatMask = 0x03;
  const unsigned int logical_format = (*cip_path) & kLogicalFormatMask;
  LogicalSegmentLogicalFormat result = kLogicalSegmentLogicalFormatEightBit;
  switch (logical_format) {
    case LOGICAL_SEGMENT_FORMAT_EIGHT_BIT:
      result = kLogicalSegmentLogicalFormatEightBit;
      break;
    case LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT:
      result = kLogicalSegmentLogicalFormatSixteenBit;
      break;
    case LOGICAL_SEGMENT_FORMAT_THIRTY_TWO_BIT:
      result = kLogicalSegmentLogicalFormatThirtyTwoBit;
      break;
    default:
      OPENER_ASSERT(
        "Logical segment/logical type: Invalid logical type detected!\n");
      break;
  }
  return result;
}

void SetPathLogicalSegmentLogicalFormat(LogicalSegmentLogicalFormat format,
                                        CipOctet *const cip_path) {
  OPENER_ASSERT( kSegmentTypeLogicalSegment ==
                 GetPathSegmentType( (const CipOctet *)cip_path ) );
  switch (format) {
    case kLogicalSegmentLogicalFormatEightBit:
      (*cip_path) |= LOGICAL_SEGMENT_FORMAT_EIGHT_BIT;
      break;
    case kLogicalSegmentLogicalFormatSixteenBit:
      (*cip_path) |= LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT;
      break;
    case kLogicalSegmentLogicalFormatThirtyTwoBit:
      (*cip_path) |= LOGICAL_SEGMENT_FORMAT_THIRTY_TWO_BIT;
      break;
    default:
      OPENER_ASSERT(
        "Logical segment/logical type: Invalid logical type detected!\n");
      break;
  }
}

const CipDword CipEpathGetLogicalValue(const EipUint8 **message) {
  LogicalSegmentLogicalFormat logical_format =
    GetPathLogicalSegmentLogicalFormat(*message);
  CipDword data = 0;
  MoveMessageNOctets(1, message); /* Move to logical value */
  switch (logical_format) {
    case kLogicalSegmentLogicalFormatEightBit:
      data = GetSintFromMessage(message);
      break;
    case kLogicalSegmentLogicalFormatSixteenBit:
      MoveMessageNOctets(1, message); /* Pad byte needs to be skipped */
      data = GetIntFromMessage(message);
      break;
    case kLogicalSegmentLogicalFormatThirtyTwoBit:
      MoveMessageNOctets(1, message); /* Pad byte needs to be skipped */
      data = GetDintFromMessage(message);
      break;
    default:
      OPENER_ASSERT(false); //shall not happen!
  }
  return data;
}

size_t CipEpathSetLogicalValue(const CipDword logical_value,
                               const LogicalSegmentLogicalFormat logical_format,
                               CipOctet **message) {
  switch(logical_value) {
    case kLogicalSegmentLogicalFormatEightBit: AddSintToMessage(logical_value,
                                                                message);
      return 1; break;
    case kLogicalSegmentLogicalFormatSixteenBit: MoveMessageNOctets(1,
                                                                    (const
                                                                     CipOctet **)message);
      AddIntToMessage(logical_value, message);
      return 3; break;
    case kLogicalSegmentLogicalFormatThirtyTwoBit: MoveMessageNOctets(1,
                                                                      (const
                                                                       CipOctet
                                                                       **)message);
      AddDintToMessage(logical_value, message);
      return 5; break;
  }
  OPENER_ASSERT(false); /* This should never happen! */
  return 0;
}

LogicalSegmentExtendedLogicalType GetPathLogicalSegmentExtendedLogicalType(
  const unsigned char *const cip_path) {
//  OPENER_ASSERT(LOGICAL_SEGMENT_TYPE_EXTENDED_kLogicalSegmentLogicalTypeExtendedLogicalMessageValue == GetPathLogicalSegmentLogicalType(cip_path),
//                "Trying to extract non-existent extended logical type");
  OPENER_ASSERT( kLogicalSegmentLogicalTypeExtendedLogical == GetPathLogicalSegmentLogicalType(
                   cip_path) );
  const unsigned int extended_logical_type = *(cip_path + 1);
  LogicalSegmentExtendedLogicalType result =
    kLogicalSegmentExtendedLogicalTypeReserved;
  switch(extended_logical_type) {
    case LOGICAL_SEGMENT_EXTENDED_TYPE_ARRAY_INDEX: result =
      kLogicalSegmentExtendedLogicalTypeArrayIndex; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_ARRAY_INDEX: result =
      kLogicalSegmentExtendedLogicalTypeIndirectArrayIndex; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_BIT_INDEX: result =
      kLogicalSegmentExtendedLogicalTypeBitIndex; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_INDIRECT_BIT_INDEX: result =
      kLogicalSegmentExtendedLogicalTypeIndirectBitIndex; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_NUMBER: result =
      kLogicalSegmentExtendedLogicalTypeStructureMemberNumber; break;
    case LOGICAL_SEGMENT_EXTENDED_TYPE_STRUCTURE_MEMBER_HANDLE: result =
      kLogicalSegmentExtendedLogicalTypeStructureMemberHandle; break;
    default: result = kLogicalSegmentExtendedLogicalTypeReserved;
  }
  return result;
}

LogicalSegmentSpecialTypeLogicalFormat
GetPathLogicalSegmentSpecialTypeLogicalType(const unsigned char *const cip_path)
{
//  OPENER_ASSERT(kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path), "Not a logical segment!\n");
  OPENER_ASSERT( kSegmentTypeLogicalSegment == GetPathSegmentType(cip_path) );
  OPENER_ASSERT( kLogicalSegmentLogicalTypeSpecial == GetPathLogicalSegmentLogicalType(
                   cip_path) );
  const unsigned int kLogicalFormatMask = 0x03;
  const unsigned int logical_format = (*cip_path) & kLogicalFormatMask;

  LogicalSegmentSpecialTypeLogicalFormat result =
    kLogicalSegmentSpecialTypeLogicalFormatReserved;
  switch(logical_format) {
    case LOGICAL_SEGMENT_SPECIAL_TYPE_FORMAT_ELECTRONIC_KEY:
      result = kLogicalSegmentSpecialTypeLogicalFormatElectronicKey; break;
    default: result = kLogicalSegmentSpecialTypeLogicalFormatReserved; break;
  }

  return result;
}

ElectronicKeySegmentFormat GetPathLogicalSegmentElectronicKeyFormat(
  const unsigned char *const cip_path) {
//  OPENER_ASSERT(kLogicalSegmentSpecialTypeLogicalFormatElectronicKey ==
//      GetPathLogicalSegmentSpecialTypeLogicalType(cip_path), "Not an electronic key!\n");
  OPENER_ASSERT( kLogicalSegmentSpecialTypeLogicalFormatElectronicKey ==
                 GetPathLogicalSegmentSpecialTypeLogicalType(cip_path) );
  ElectronicKeySegmentFormat result = kElectronicKeySegmentFormatReserved;
  switch( *(cip_path + 1) ) {
    case ELECTRONIC_KEY_SEGMENT_KEY_FORMAT_4: result =
      kElectronicKeySegmentFormatKeyFormat4; break;
    default: result = kElectronicKeySegmentFormatReserved; break;
  }
  return result;
}

void GetElectronicKeyFormat4FromMessage(
  const CipOctet **const message,
  ElectronicKeyFormat4 *key) {
  OPENER_ASSERT( kElectronicKeySegmentFormatKeyFormat4 ==
                 GetPathLogicalSegmentElectronicKeyFormat(*message) );

  MoveMessageNOctets(2, message);
  ElectronicKeyFormat4SetVendorId(key, GetIntFromMessage(message) );
  ElectronicKeyFormat4SetDeviceType(key, GetIntFromMessage(message) );
  ElectronicKeyFormat4SetProductCode(key, GetIntFromMessage(message) );
  ElectronicKeyFormat4SetMajorRevisionCompatibility(key,
                                                    GetSintFromMessage(message) );
  ElectronicKeyFormat4SetMinorRevision(key, GetSintFromMessage(message) );
}

/*** Logical Segment ***/


/*** Network Segment ***/

/** @brief Return the Network Segment subtype
 *
 *  @param cip_path Pointer to the start of the EPath message
 *  @return The Network Segment subtype of the EPath
 */
NetworkSegmentSubtype GetPathNetworkSegmentSubtype(
  const unsigned char *const cip_path) {
  OPENER_ASSERT( kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path) );
  const unsigned int kSubtypeMask = 0x1F;
  const unsigned int subtype = (*cip_path) & kSubtypeMask;
  NetworkSegmentSubtype result = kNetworkSegmentSubtypeReserved;
  switch(subtype) {
    case NETWORK_SEGMENT_SCHEDULE:
      result = kNetworkSegmentSubtypeScheduleSegment; break;
    case NETWORK_SEGMENT_FIXED_TAG:
      result = kNetworkSegmentSubtypeFixedTagSegment; break;
    case NETWORK_SEGMENT_PRODUCTION_INHIBIT_TIME_IN_MILLISECONDS:
      result = kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds; break;
    case NETWORK_SEGMENT_SAFETY:
      result = kNetworkSegmentSubtypeSafetySegment; break;
    case NETWORK_SEGMENT_PRODUCTION_INHIBIT_TIME_IN_MICROSECONDS:
      result = kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds; break;
    case NETWORK_SEGMENT_EXTENDED_NETWORK:
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
CipUsint GetPathNetworkSegmentProductionInhibitTimeInMilliseconds(
  const unsigned char *const cip_path) {
//  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path),"Not a network segment!\n");
//  OPENER_ASSERT(kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds == GetPathNetworkSegmentSubtype(cip_path),
//                "Not a Production Inhibit Time milliseconds segment!\n");
  OPENER_ASSERT( kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path) );
  OPENER_ASSERT( kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds == GetPathNetworkSegmentSubtype(
                   cip_path) );
  return *(cip_path + 1);
}

/**
 * @brief Return the Production Inhibit Time in microseconds from an EPath
 *
 * @param cip_path Pointer to the start of the EPath message
 * @return the Production Inhibit Time in microseconds ranging from 0 to 4294967295
 */
CipUdint GetPathNetworkSegmentProductionInhibitTimeInMicroseconds(
  const unsigned char *const cip_path) {
//  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path),"Not a network segment!\n");
//  OPENER_ASSERT(kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds == GetPathNetworkSegmentSubtype(cip_path),
//                  "Not a Production Inhibit Time microseconds segment!\n");
//  OPENER_ASSERT(2 == *(cip_path + 1), "Data Words length is incorrect! See CIP Spec Vol.1 C-1.4.3.3.2\n");

  OPENER_ASSERT( kSegmentTypeNetworkSegment == GetPathSegmentType(cip_path) );
  OPENER_ASSERT( kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds == GetPathNetworkSegmentSubtype(
                   cip_path) );
  OPENER_ASSERT( 2 == *(cip_path + 1) );

  const unsigned char *message_runner = cip_path + 2;
  return GetDintFromMessage(&message_runner);
}

/*** Network Segment ***/

/*** Symbolic Segment ***/

SymbolicSegmentFormat GetPathSymbolicSegmentFormat(
  const unsigned char *const cip_path) {
  const unsigned int kSymbolicSegmentFormatMask = 0x1F;
  if( SYMBOLIC_SEGMENT_FORMAT_EXTENDED_STRING ==
      (*cip_path & kSymbolicSegmentFormatMask) ) {
    return kSymbolicSegmentFormatExtendedString;
  }
  return kSymbolicSegmentFormatASCII;
}

unsigned int GetPathSymbolicSegmentASCIIFormatLength(
  const unsigned char *const cip_path) {
  const unsigned int kSymbolicSegmentASCIIFormatLength = 0x1F;
  const unsigned int length = *cip_path & kSymbolicSegmentASCIIFormatLength;
  OPENER_ASSERT(0 != length);
  return length;
}

SymbolicSegmentExtendedFormat GetPathSymbolicSegmentNumericType(
  const unsigned char *const cip_path) {
  const unsigned int kSymbolicSegmentExtendedFormatNumericTypeMask = 0x1F;
  const unsigned int numeric_subtype = *(cip_path + 1) &
                                       kSymbolicSegmentExtendedFormatNumericTypeMask;
  SymbolicSegmentExtendedFormat result = kSymbolicSegmentExtendedFormatReserved;
  switch(numeric_subtype) {
    case SYMBOLIC_SEGMENT_EXTENDED_FORMAT_NUMERIC_USINT_TYPE: result =
      kSymbolicSegmentExtendedFormatNumericSymbolUSINT; break;
    case SYMBOLIC_SEGMENT_EXTENDED_FORMAT_NUMERIC_UINT_TYPE: result =
      kSymbolicSegmentExtendedFormatNumericSymbolUINT; break;
    case SYMBOLIC_SEGMENT_EXTENDED_FORMAT_NUMERIC_UDINT_TYPE: result =
      kSymbolicSegmentExtendedFormatNumericSymbolUDINT; break;
    default: result = kSymbolicSegmentExtendedFormatReserved; break;
  }
  return result;
}

SymbolicSegmentExtendedFormat GetPathSymbolicSegmentExtendedFormat(
  const unsigned char *const cip_path) {
  OPENER_ASSERT( kSegmentTypeSymbolicSegment == GetPathSegmentType(cip_path) );
  OPENER_ASSERT( kSymbolicSegmentFormatExtendedString == GetPathSymbolicSegmentFormat(
                   cip_path) );
  const unsigned int kSymbolicSegmentExtendedFormatMask = 0xE0;
  const unsigned int extended_type = *(cip_path + 1) &
                                     kSymbolicSegmentExtendedFormatMask;
  SymbolicSegmentExtendedFormat result = kSymbolicSegmentExtendedFormatReserved;
  switch(extended_type) {
    case SYMBOLIC_SEGMENT_EXTENDED_FORMAT_DOUBLE_CHAR: result =
      kSymbolicSegmentExtendedFormatDoubleByteChars; break;
    case SYMBOLIC_SEGMENT_EXTENDED_FORMAT_TRIPLE_CHAR: result =
      kSymbolicSegmentExtendedFormatTripleByteChars; break;
    case SYMBOLIC_SEGMENT_EXTENDED_FORMAT_NUMERIC: result =
      GetPathSymbolicSegmentNumericType(cip_path); break;
    default: result = kSymbolicSegmentExtendedFormatReserved; break;
  }
  return result;
}

/*** Symbolic Segment ***/

/*** Data Segment ***/

DataSegmentSubtype GetPathDataSegmentSubtype(const unsigned char *const cip_path)
{
  const unsigned int kDataSegmentSubtypeMask = 0x1F;
  const unsigned int data_subtype = (*cip_path) & kDataSegmentSubtypeMask;

  DataSegmentSubtype result = kDataSegmentSubtypeReserved;
  switch(data_subtype) {
    case DATA_SEGMENT_SUBTYPE_SIMPLE_DATA:
      result = kDataSegmentSubtypeSimpleData; break;
    case DATA_SEGMENT_SUBTYPE_ANSI_EXTENDED_SYMBOL:
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
CipUsint GetPathDataSegmentSimpleDataWordLength(
  const unsigned char *const cip_path) {
//  OPENER_ASSERT(kSegmentTypeDataSegment == GetPathSegmentType(cip_path),"Not a data segment!\n");
//  OPENER_ASSERT(kDataSegmentSubtypeSimpleData == GetPathDataSegmentSubtype(cip_path), "Not a simple data segment!\n");
  OPENER_ASSERT( kSegmentTypeDataSegment == GetPathSegmentType(cip_path) );
  OPENER_ASSERT( kDataSegmentSubtypeSimpleData ==
                 GetPathDataSegmentSubtype(cip_path) );

  const unsigned char *message_runner = cip_path + 1;
  return GetSintFromMessage(&message_runner);
}

/*** End Data Segment ***/

/* Special purpose functions */

LogicalSegmentLogicalFormat CipEpathGetNeededLogicalFormatForValue(
  CipDword value) {
  LogicalSegmentLogicalFormat logical_format =
    kLogicalSegmentLogicalFormatEightBit;
  if(0xFF < value) {
    logical_format = kLogicalSegmentLogicalFormatSixteenBit;
  }
  if(0xFFFF < value) {
    logical_format = kLogicalSegmentLogicalFormatThirtyTwoBit;
  }
  return logical_format;
}

size_t CipEpathEncodeConnectionEpath(
  const CipConnectionPathEpath *const connection_epath,
  CipOctet **encoded_path) {

  size_t encoded_path_length = 0;
  {
    SetPathSegmentType(kSegmentTypeLogicalSegment, *encoded_path);
    SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeClassId,
                                     *encoded_path);
    LogicalSegmentLogicalFormat logical_value =
      CipEpathGetNeededLogicalFormatForValue(connection_epath->class_id);
    SetPathLogicalSegmentLogicalFormat(logical_value, *encoded_path);
    encoded_path_length += 1;
    MoveMessageNOctets(1, (const CipOctet **)encoded_path);
    encoded_path_length += CipEpathSetLogicalValue(connection_epath->class_id,
                                                   logical_value,
                                                   encoded_path);
  }

  {
    SetPathSegmentType(kSegmentTypeLogicalSegment, *encoded_path);
    SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeClassId,
                                     *encoded_path);
    LogicalSegmentLogicalFormat logical_value =
      CipEpathGetNeededLogicalFormatForValue(connection_epath->instance_id);
    SetPathLogicalSegmentLogicalFormat(logical_value, *encoded_path);
    encoded_path_length += 1;
    MoveMessageNOctets(1, (const CipOctet **)encoded_path);
    encoded_path_length += CipEpathSetLogicalValue(
      connection_epath->instance_id,
      logical_value,
      encoded_path);
  }

  if(0 != connection_epath->attribute_id_or_connection_point) {
    SetPathSegmentType(kSegmentTypeLogicalSegment, *encoded_path);
    SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeClassId,
                                     *encoded_path);
    LogicalSegmentLogicalFormat logical_value =
      CipEpathGetNeededLogicalFormatForValue(
        connection_epath->attribute_id_or_connection_point);
    SetPathLogicalSegmentLogicalFormat(logical_value, *encoded_path);
    encoded_path_length += 1;
    MoveMessageNOctets(1, (const CipOctet **)encoded_path);
    encoded_path_length += CipEpathSetLogicalValue(
      connection_epath->attribute_id_or_connection_point,
      logical_value,
      encoded_path);
  }
  return encoded_path_length += 1;
}

bool CipEpathEqual(const CipOctet *const path1,
                   const CipUint path1_length,
                   const CipOctet *const path2,
                   const CipUint path2_length) {
  if(path1_length != path2_length) {
    return false;
  }
  for(size_t i = 0; i < path1_length; ++i) {
    if(path1[i] != path2[i]) {
      return false;
    }
  }
  return true;
}

