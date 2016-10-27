/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <stdbool.h>

#include "cipepath.h"

#include "endianconv.h"
#include "cipelectronickey.h"

const unsigned int kPortSegmentExtendedPort = 15; /**< Reserved port segment port value, indicating the use of the extended port field */

/* Segments */
const unsigned int kSegmentTypePortSegmentMessageValue = 0x00; /**< Message value of the Port segment */
const unsigned int kSegmentTypeLogicalSegmentMessageValue = 0x20; /**< Message value of the Logical segment */
const unsigned int kSegmentTypeNetworkSegmentMessageValue = 0x40; /**< Message value of the Network segment */
const unsigned int kSegmentTypeSymbolicSegmentMessageValue = 0x60; /**< Message value of the Symbolic segment */
const unsigned int kSegmentTypeDataSegmentMessageValue = 0x80; /**< Message value of the Data segment */
const unsigned int kSegmentTypeDataTypeConstructedMessageValue = 0xA0; /**< Message value of the Data type constructed */
const unsigned int kSegmentTypeDataTypeElementaryMessageValue = 0xC0; /**< Message value of the Data type elementary */
const unsigned int kSegmentTypeSegmentTypeReservedMessageValue = 0xE0; /**< Reserved value */

const unsigned int kLogicalSegmentLogicalTypeClassIdMessageValue = 0x00; /**< Message value of the logical segment/logical type Class ID */
const unsigned int kLogicalSegmentLogicalTypeInstanceIdMessageValue = 0x04; /**< Message value of the logical segment/logical type Instance ID */
const unsigned int kLogicalSegmentLogicalTypeMemberIdMessageValue = 0x08; /**< Message value of the logical segment/logical type Member ID */
const unsigned int kLogicalSegmentLogicalTypeConnectionPointMessageValue = 0x0C; /**< Message value of the logical segment/logical type Connection Point */
const unsigned int kLogicalSegmentLogicalTypeAttributeIdMessageValue = 0x10; /**< Message value of the logical segment/logical type Attribute ID */
const unsigned int kLogicalSegmentLogicalTypeSpecialMessageValue = 0x14; /**< Message value of the logical segment/logical type Special */
const unsigned int kLogicalSegmentLogicalTypeServiceIdMessageValue = 0x18; /**< Message value of the logical segment/logical type Service ID */
const unsigned int kLogicalSegmentLogicalTypeExtendedLogicalMessageValue = 0x1C; /**< Message value of the logical segment/logical type Extended Logical */

const unsigned int kLogicalSegmentLogicalFormatEightBitMessageValue = 0x00;
const unsigned int kLogicalSegmentLogicalFormatSixteenBitMessageValue = 0x01;
const unsigned int kLogicalSegmentLogicalFormatThirtyTwoBitMessageValue = 0x02;

const unsigned int kLogicalSegmentLogicalExtendedTypeReservedMessageValue = 0x00;
const unsigned int kLogicalSegmentLogicalExtendedTypeArrayIndexMessageValue = 0x01;
const unsigned int kLogicalSegmentLogicalExtendedTypeIndirectArrayIndexMessageValue = 0x02;
const unsigned int kLogicalSegmentLogicalExtendedTypeBitIndexMessageValue = 0x03;
const unsigned int kLogicalSegmentLogicalExtendedTypeIndirectBitIndexMessageValue = 0x04;
const unsigned int kLogicalSegmentLogicalExtendedTypeStructureMemberNumberMessageValue = 0x05;
const unsigned int kLogicalSegmentLogicalExtendedTypeStructureMemberHandleMessageValue = 0x06;

const unsigned int kLogicalSegmentSpecialTypeLogicalFormatElectronicKeyMessageValue = 0x00;

const unsigned int kNetworkSegmentSubtypeScheduleSegmentMessageValue = 0x01;
const unsigned int kNetworkSegmentSubtypeFixedTagSegmentMessageValue = 0x02;
const unsigned int kNetworkSegmentSubtypeProductionInhibitTimeInMillisecondsMessageValue = 0x03;
const unsigned int kNetworkSegmentSubtypeSafetySegmentMessageValue = 0x04;
const unsigned int kNetworkSegmentSubtypeProductionInhibitTimeInMicrosecondsMessageValue = 0x10;
const unsigned int kNetworkSegmentSubtypeExtendedNetworkSegmentMessageValue = 0x1F;

typedef enum {
  kElectronicKeySegmentFormatReserved,
  kElectronicKeySegmentFormatKeyFormat4
} ElectronicKeySegmentFormat;

const unsigned int kElectronicKeySegmentFormatKeyFormat4MessageValue = 0x04;

typedef enum {
  kDataSegmentSubtypeReserved,
  kDataSegmentSubtypeSimpleData,
  kDataSegmentSubtypeANSIExtendedSymbol
} DataSegmentSubtype;

const unsigned int kDataSegmentSubtypeSimpleDataMessageValue = 0x00;
const unsigned int kDataSegmentSubtypeANSIExtendedSymbolMessageValue = 0x11;



/*** Path Segment ***/
SegmentType GetPathSegementType(const char *const cip_path) {
  const unsigned int kSegmentTypeMask = 0xE0;
  const unsigned int segment_type = cip_path & kSegmentTypeMask;
  SegmentType result = kSegmentTypeSegmentTypeReserved;
  switch (segment_type) {
    case kSegmentTypePortSegmentMessageValue:
      result = kSegmentTypePortSegment;
      break;
    case kSegmentTypeLogicalSegmentMessageValue:
      result = kSegmentTypeLogicalSegment;
      break;
    case kSegmentTypeNetworkSegmentMessageValue:
      result = kSegmentTypeNetworkSegment;
      break;
    case kSegmentTypeSymbolicSegmentMessageValue:
      result = kSegmentTypeSymbolicSegment;
      break;
    case kSegmentTypeDataSegmentMessageValue:
      result = kSegmentTypeDataSegment;
      break;
    case kSegmentTypeDataTypeConstructedMessageValue:
      result = kSegmentTypeDataTypeConstructed;
      break;
    case kSegmentTypeDataTypeElementaryMessageValue:
      result = kSegmentTypeDataTypeElementary;
      break;
    case kSegmentTypeSegmentTypeReservedMessageValue:
      result = kSegmentTypeSegmentTypeReserved;
      break;
    default:
      OPENER_ASSERT(
          "Invalid Segment type in the message! We should never come here!\n");
      break;
  }
  return result;
}

void SetPathSegementType(SegmentType segment_type, char *const cip_path) {
  switch (segment_type) {
    case kSegmentTypePortSegment:
      cip_path |= kSegmentTypePortSegmentMessageValue;
      break;
    case kSegmentTypeLogicalSegment:
      cip_path |= kSegmentTypeLogicalSegmentMessageValue;
      break;
    case kSegmentTypeNetworkSegment:
      cip_path |= kSegmentTypeNetworkSegmentMessageValue;
      break;
    case kSegmentTypeSymbolicSegment:
      cip_path |= kSegmentTypeSymbolicSegmentMessageValue;
      break;
    case kSegmentTypeDataSegment:
      cip_path |= kSegmentTypeDataSegmentMessageValue;
      break;
    case kSegmentTypeDataTypeConstructed:
      cip_path |= kSegmentTypeDataTypeConstructedMessageValue;
      break;
    case kSegmentTypeDataTypeElementary:
      cip_path |= kSegmentTypeDataTypeElementaryMessageValue;
      break;
    case kSegmentTypeSegmentTypeReserved:
      cip_path |= kSegmentTypeSegmentTypeReservedMessageValue;
      break;
    default:
      OPENER_ASSERT(
          "Invalid Segment type chosen! We should never come here!\n");
  }
}

/*** Port Segment ***/
bool GetPathPortSegementExtendedLinkAddressSizeBit(const char *const cip_path) {
  const unsigned int kExtendedLinkAddressSizeMask = 0x10;
  if (kExtendedLinkAddressSizeMask == cip_path & kExtendedLinkAddressSizeMask) {
    return true;
  }
  return false;
}

unsigned int GetPathPortSegmentPortIdentifier(const char *const cip_path) {
  const unsigned int kPortIdentifierMask = 0x0F;
  unsigned int port_identifier = cip_path & kPortIdentifierMask;
  OPENER_ASSERT(0 != port_identifier, "Use of reserved port identifier 0\n");
  return port_identifier;
}

void SetPathPortSegmentPortIdentifier(const unsigned int port_identifier,
                                      char *const cip_path) {
  OPENER_ASSERT(
      port_identifier < 16,
      "Port identifier too large for standard port identifier field\n");

}

unsigned int GetPathPortSegementLinkAddressSize(const char *const cip_path) {
  OPENER_ASSERT(
  false == GetPathPortSegementExtendedLinkAddressSizeBit(cip_path),
                "Call to non existent extended link address size\n");
  return *(cip_path + 1);
}

unsigned int GetPathPortSegmentExtendedPortNumber(const char *const cip_path) {
  OPENER_ASSERT(kPortSegmentExtendedPort == GetPathPortSegmentPortIdentifier,
                "There is no extended port available!\n");
  const unsigned int kExtendedPortSegmentPosition =
      GetPathPortSegementExtendedLinkAddressSizeBit(cip_path) == true ? 3 : 2;
  return cip_path[kExtendedPortSegmentPosition]
      + (cip_path[kExtendedPortSegmentPosition + 1] << 8);
}

void SetPathPortSegmentExtendedPortIdentifier(
    const unsigned int extended_port_identifier, char *const cip_path) {
  SetPathPortSegmentPortIdentifier(kPortSegmentExtendedPort, cip_path);
  const unsigned int kExtendedPortSegmentPosition =
      GetPathPortSegementExtendedLinkAddressSizeBit(cip_path) == true ? 3 : 2;
  cip_path[kExtendedPortSegmentPosition] = (char*) (extended_port_identifier
      & 0x00FF);
  cip_path[kExtendedPortSegmentPosition + 1] =
      (char*) ((extended_port_identifier & 0xFF00) >> 8);
}
/*** Port Segment ***/

/*** Logical Segment ***/

LogicalSegmentLogicalType GetPathLogicalSegmentLogicalType(
    const char *const cip_path) {
  const unsigned int kLogicalTypeMask = 0x1C;
  const unsigned int logical_type = (*cip_path) & kLogicalTypeMask;
  LogicalSegmentLogicalType result = kLogicalSegmentLogicalTypeExtendedLogical;
  switch (logical_type) {
    case kLogicalSegmentLogicalTypeClassIdMessageValue:
      result = kLogicalSegmentLogicalTypeClassId;
      break;
    case kLogicalSegmentLogicalTypeInstanceIdMessageValue:
      result = kLogicalSegmentLogicalTypeInstanceId;
      break;
    case kLogicalSegmentLogicalTypeMemberIdMessageValue:
      result = kLogicalSegmentLogicalTypeMemberId;
      break;
    case kLogicalSegmentLogicalTypeConnectionPointMessageValue:
      result = kLogicalSegmentLogicalTypeConnectionPoint;
      break;
    case kLogicalSegmentLogicalTypeAttributeIdMessageValue:
      result = kLogicalSegmentLogicalTypeAttributeId;
      break;
    case kLogicalSegmentLogicalTypeSpecialMessageValue:
      result = kLogicalSegmentLogicalTypeSpecial;
      break;
    case kLogicalSegmentLogicalTypeServiceIdMessageValue:
      result = kLogicalSegmentLogicalTypeServiceId;
      break;
    case kLogicalSegmentLogicalTypeExtendedLogicalMessageValue:
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
    const char *const cip_path) {
  const unsigned int kLogicalFormatMask = 0x03;
  const unsigned int logical_format = (*cip_path) & kLogicalFormatMask;
  LogicalSegmentLogicalFormat result = kLogicalSegmentLogicalFormatEightBit;
  switch (logical_format) {
    case kLogicalSegmentLogicalFormatEightBitMessageValue:
      result = kLogicalSegmentLogicalFormatEightBit;
      break;
    case kLogicalSegmentLogicalFormatSixteenBitMessageValue:
      result = kLogicalSegmentLogicalFormatSixteenBit;
      break;
    case kLogicalSegmentLogicalFormatThirtyTwoBitMessageValue:
      result = kLogicalSegmentLogicalFormatThirtyTwoBit;
      break;
    default:
      OPENER_ASSERT(
          "Logical segment/logical type: Invalid logical type detected!\n");
      break;
  }
  return result;
}

LogicalSegmentExtendedLogicalType GetPathLogicalSegmentExtendedLogicalType(const char *const cip_path) {
  OPENER_ASSERT(kLogicalSegmentLogicalTypeExtendedLogicalMessageValue == GetPathLogicalSegmentLogicalType(cip_path),
                "Trying to extract non-existent extended logical type");
  const char extended_logical_type = *(cip_path + 1);
  LogicalSegmentExtendedLogicalType result = kLogicalSegmentExtendedLogicalTypeReserved;
  switch(extended_logical_type) {
    case kLogicalSegmentLogicalExtendedTypeArrayIndexMessageValue: result = kLogicalSegmentExtendedLogicalTypeArrayIndex; break;
    case kLogicalSegmentLogicalExtendedTypeIndirectArrayIndexMessageValue: result = kLogicalSegmentExtendedLogicalTypeIndirectArrayIndex; break;
    case kLogicalSegmentLogicalExtendedTypeBitIndexMessageValue: result = kLogicalSegmentExtendedLogicalTypeBitIndex; break;
    case kLogicalSegmentLogicalExtendedTypeIndirectBitIndexMessageValue: result = kLogicalSegmentExtendedLogicalTypeIndirectBitIndex; break;
    case kLogicalSegmentLogicalExtendedTypeStructureMemberNumberMessageValue: result = kLogicalSegmentExtendedLogicalTypeStructureMemberNumber; break;
    case kLogicalSegmentLogicalExtendedTypeStructureMemberHandleMessageValue: result = kLogicalSegmentExtendedLogicalTypeStructureMemberHandle; break;
    default: result = kLogicalSegmentExtendedLogicalTypeReserved;
  }
  return result;
}

LogicalSegmentSpecialTypeLogicalFormat GetPathLogicalSegmentSpecialTypeLogicalType(const char *const cip_path) {
  OPENER_ASSERT(kSegmentTypeLogicalSegment == GetPathSegementType(cip_path), "Not a logical segment!\n");
  const unsigned int kLogicalFormatMask = 0x03;
  const unsigned int logical_format = (*cip_path) & kLogicalFormatMask;

  LogicalSegmentSpecialTypeLogicalFormat result = kLogicalSegmentSpecialTypeLogicalFormatReserved;
  switch(logical_format) {
    case kLogicalSegmentSpecialTypeLogicalFormatElectronicKeyMessageValue:
      result = kLogicalSegmentSpecialTypeLogicalFormatElectronicKey; break;
    default: result = kLogicalSegmentSpecialTypeLogicalFormatReserved; break;
  }

  return result;
}

ElectronicKeySegmentFormat *GetPathLogicalSegmentElectronicKeyFormat(const char *const cip_path) {
  OPENER_ASSERT(kLogicalSegmentSpecialTypeLogicalFormatElectronicKey ==
      GetPathLogicalSegmentSpecialTypeLogicalType(cip_path), "Not an electronic key!\n");
  ElectronicKeySegmentFormat result = kElectronicKeySegmentFormatReserved;
  switch(*(cip_path + 1)) {
    case kElectronicKeySegmentFormatKeyFormat4MessageValue: result = kElectronicKeySegmentFormatKeyFormat4; break;
    default: result = kElectronicKeySegmentFormatReserved; break;
  }
  return result;
}

ElectronicKeyFormat4 *GetPathLogicalSegmentElectronicKeyFormat4(const char *const cip_path) {
  OPENER_ASSERT(kElectronicKeySegmentFormatKeyFormat4 ==
      GetPathLogicalSegmentElectronicKeyFormat(cip_path), "Not electronic key format 4!\n");

  const char *message_runner = (const char *)cip_path;
  ElectronicKeyFormat4 *result = calloc(sizeof(ElectronicKeySegmentFormat));
  SetElectronicKeyFormat4VendorId(GetIntFromMessage(&message_runner), result);
  SetElectronicKeyFormat4DeviceType(GetIntFromMessage(&message_runner), result);
  SetElectronicKeyFormat4ProductCode(GetIntFromMessage(&message_runner), result);
  SetElectronicKeyFormat4MajorRevisionCompatibility(GetSintFromMessage(&message_runner), result);
  SetElectronicKeyFormat4MinorRevision(GetSIntFromMessage(&message_runner), result);

  return result;
}

/*** Logical Segment ***/


/*** Network Segment ***/

/** @brief Return the Network Segment subtype
 *
 *  @param cip_path Pointer to the start of the EPath message
 *  @return The Network Segment subtype of the EPath
 */
NetworkSegmentSubType GetPathNetworkSegmentSubtype(const char *const cip_path) {
  const unsigned int kSubtypeMask = 0x1F;
  const unsigned int subtype = (*cip_path) & kSubtypeMask;
  NetworkSegmentSubType result = kNetworkSegmentSubtypeReserved;
  switch(subtype) {
    case kNetworkSegmentSubtypeScheduleSegmentMessageValue:
      result = kNetworkSegmentSubtypeScheduleSegment; break;
    case kNetworkSegmentSubtypeFixedTagSegmentMessageValue:
      result = kNetworkSegmentSubtypeFixedTagSegment; break;
    case kNetworkSegmentSubtypeProductionInhibitTimeInMillisecondsMessageValue:
      result = kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds; break;
    case kNetworkSegmentSubtypeSafetySegmentMessageValue:
      result = kNetworkSegmentSubtypeSafetySegment; break;
    case kNetworkSegmentSubtypeProductionInhibitTimeInMicrosecondsMessageValue:
      result = kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds; break;
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
CipUsint GetPathNetworkSegmentProductionInhibitTimeInMilliseconds(const char *const cip_path) {
  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegementType(cip_path),"Not a network segment!\n");
  OPENER_ASSERT(kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds == GetPathNetworkSegmentSubtype(cip_path),
                "Not a Production Inhibit Time milliseconds segment!\n");
  return *(cip_path + 1);
}

/**
 * @brief Return the Production Inhibit Time in microseconds from an EPath
 *
 * @param cip_path Pointer to the start of the EPath message
 * @return the Production Inhibit Time in microseconds ranging from 0 to 4294967295
 */
CipUdint GetPathNetworkSegmentProductionInhibitTimeInMicroseconds(const char *const cip_path) {
  OPENER_ASSERT(kSegmentTypeNetworkSegment == GetPathSegementType(cip_path),"Not a network segment!\n");
  OPENER_ASSERT(kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds == GetPathNetworkSegmentSubtype(cip_path),
                  "Not a Production Inhibit Time microseconds segment!\n");
  OPENER_ASSERT(2 == *(cip_path + 1), "Data Words length is incorrect! See CIP Spec Vol.1 C-1.4.3.3.2\n");
  const char *message_runner = cip_path;
  return GetDintFromMessage(&message_runner);
}

/*** Network Segment ***/

/*** Symbolic Segment ***/

/* Currently not supported */

/*** Symbolic Segment ***/

/*** Data Segment ***/

DataSegmentSubtype GetPathDataSegmentSubtype(const char *const cip_path) {
  const unsigned int kDataSegmentSubtypeMask = 0x1F;
  const unsigned int data_subtype = (*cip_path) & kDataSegmentSubtypeMask;

  DataSegmentSubtype result = kDataSegmentSubtypeReserved;
  switch(data_subtype) {
    case kDataSegmentSubtypeSimpleDataMessageValue:
      result = kDataSegmentSubtypeSimpleData; break;
    case kDataSegmentSubtypeANSIExtendedSymbolMessageValue:
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
CipUsint GetPathDataSegmentSimpleDataWordLength(const char *const cip_path) {
  OPENER_ASSERT(kSegmentTypeDataSegment == GetPathSegementType(cip_path),"Not a data segment!\n");
  OPENER_ASSERT(kDataSegmentSubtypeSimpleData == GetPathDataSegmentSubtype(cip_path), "Not a simple data segment!\n");

  const char *message_runner = cip_path;
  return GetSintFromMessage(&message_runner);
}

/*** Data Segment ***/

