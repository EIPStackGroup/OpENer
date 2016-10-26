/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <stdbool.h>

#include "cipepath.h"

const unsigned int kPathSegmentExtendedPort = 15;

/*** Path Segment ***/
SegmentType GetPathSegementType(const char *const cip_path) {
  const unsigned int kSegmentTypeMask = 0xE0;
  const unsigned int segment_type = cip_path & kSegmentTypeMask;
  switch(segment_type) {
    case 0x0: return kSegmentTypePortSegment;
    case 0x20: return kSegmentTypeLogicalSegment;
    case 0x40: return kSegmentTypeNetworkSegment;
    case 0x60: return kSegmentTypeSymbolicSegment;
    case 0x80: return kSegmentTypeDataSegment;
    case 0xA0: return kSegmentTypeDataTypeConstructed;
    case 0xC0: return kSegmentTypeDataTypeElementary;
    case 0xE0: return kSegmentTypeSegmentTypeReserved;
  }
  OPENER_ASSERT("Invalid Segment type in the message! We should never come here!\n");
  return kSegmentTypeSegmentTypeReserved;
}

void SetPathSegementType(SegmentType segment_type, char *const cip_path) {
  switch (segment_type) {
    case kSegmentTypePortSegment: cip_path |= 0x00; break;
    case kSegmentTypeLogicalSegment: cip_path |= 0x20; break;
    case kSegmentTypeNetworkSegment: cip_path |= 0x40; break;
    case kSegmentTypeSymbolicSegment: cip_path |= 0x60; break;
    case kSegmentTypeDataSegment: cip_path |= 0x80; break;
    case kSegmentTypeDataTypeConstructed: cip_path |= 0xA0; break;
    case kSegmentTypeDataTypeElementary: cip_path |= 0xC0; break;
    case kSegmentTypeSegmentTypeReserved: cip_path |= 0xE0; break;
    default: OPENER_ASSERT("Invalid Segment type chosen! We should never come here!\n");
  }
}

/*** Port Segment ***/
bool GetPathPortSegementExtendedLinkAddressSizeBit(const char *const cip_path) {
  const unsigned int kExtendedLinkAddressSizeMask = 0x10;
  if(kExtendedLinkAddressSizeMask == cip_path & kExtendedLinkAddressSizeMask) {
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

void SetPathPortSegmentPortIdentifier(const unsigned int port_identifier, char *const cip_path) {
  OPENER_ASSERT(port_identifier < 16, "Port identifier too large for standard port identifier field\n");

}

unsigned int GetPathPortSegementLinkAddressSize(const char *const cip_path) {
  OPENER_ASSERT(false == GetPathPortSegementExtendedLinkAddressSizeBit(cip_path), "Call to non existent extended link address size\n");
  return *(cip_path + 1);
}

unsigned int GetPathPortSegmentExtendedPortNumber(const char *const cip_path) {
  OPENER_ASSERT(kPathSegmentExtendedPort == GetPathPortSegmentPortIdentifier, "There is no extended port available!\n");
  const unsigned int kExtendedPortSegmentPosition = GetPathPortSegementExtendedLinkAddressSizeBit(cip_path) == true ? 3 : 2;
  return cip_path[kExtendedPortSegmentPosition] + (cip_path[kExtendedPortSegmentPosition + 1] << 8);
}

void SetPathPortSegmentExtendedPortIdentifier(const unsigned int extended_port_identifier, char *const cip_path) {
  SetPathPortSegmentPortIdentifier(kPathSegmentExtendedPort, cip_path);
  const unsigned int kExtendedPortSegmentPosition = GetPathPortSegementExtendedLinkAddressSizeBit(cip_path) == true ? 3 : 2;
  cip_path[kExtendedPortSegmentPosition] = (char*)(extended_port_identifier & 0x00FF);
  cip_path[kExtendedPortSegmentPosition + 1] = (char*)((extended_port_identifier & 0xFF00) >> 8);
}
/*** Port Segment ***/

/*** Logical Segment ***/

LogicalSegmentLogicalType GetPathLogicalSegmentLogicalType(const char *const cip_path) {
  const unsigned int kLogicalTypeMask = 0x1C;
  const unsigned int logical_type = cip_path & kLogicalTypeMask;
  LogicalSegmentLogicalType result = kLogicalSegmentLogicalTypeExtendedLogical;
  switch(logical_type) {
    case 0x00: result = kLogicalSegmentLogicalTypeClassId; break;
    case 0x04: result = kLogicalSegmentLogicalTypeInstanceId; break;
    case 0x08: result = kLogicalSegmentLogicalTypeMemberId; break;
    case 0x0C: result = kLogicalSegmentLogicalTypeConnectionPoint; break;
    case 0x10: result = kLogicalSegmentLogicalTypeAttributeId; break;
    case 0x14: result = kLogicalSegmentLogicalTypeSpecial; break;
    case 0x18: result = kLogicalSegmentLogicalTypeServiceId; break;
    case 0x1C: result = kLogicalSegmentLogicalTypeExtendedLogical; break;
    default: OPENER_ASSERT("It is not possible to reach this point"); break;
  }
  return result;
}

/*** Logical Segment ***/
