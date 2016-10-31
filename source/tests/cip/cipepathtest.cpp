/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "cipepath.h"

}

TEST_GROUP(CipEpath) {

};

/** Segment type tests **/
TEST(CipEpath, GetSegmentTypePortSegment) {
  const unsigned char message[] = {0x00};
  SegmentType segment_type = GetPathSegmentType(message);
  CHECK_EQUAL(kSegmentTypePortSegment, segment_type);
}

TEST(CipEpath, GetSegmentTypeLogicalSegment) {
  const unsigned char message[] = {0x20};
  SegmentType segment_type = GetPathSegmentType(message);
  CHECK_EQUAL(kSegmentTypeLogicalSegment, segment_type);
}

TEST(CipEpath, GetSegmentTypeNetworkSegment) {
  const unsigned char message[] = {0x40};
  SegmentType segment_type = GetPathSegmentType(message);
  CHECK_EQUAL(kSegmentTypeNetworkSegment, segment_type);
}

TEST(CipEpath, GetSegmentTypeSymbolicSegment) {
  const unsigned char message[] = {0x60};
  SegmentType segment_type = GetPathSegmentType(message);
  CHECK_EQUAL(kSegmentTypeSymbolicSegment, segment_type);
}

TEST(CipEpath, GetSegmentTypeDataSegment) {
  const unsigned char message[] = {0x80};
  SegmentType segment_type = GetPathSegmentType(message);
  CHECK_EQUAL(kSegmentTypeDataSegment, segment_type);
}

TEST(CipEpath, GetSegmentTypeDataTypeConstructed) {
  const unsigned char message[] = {0xA0};
  SegmentType segment_type = GetPathSegmentType(message);
  CHECK_EQUAL(kSegmentTypeDataTypeConstructed, segment_type);
}

TEST(CipEpath, GetSegmentTypeDataTypeElementary) {
  const unsigned char message[] = {0xC0};
  SegmentType segment_type = GetPathSegmentType(message);
  CHECK_EQUAL(kSegmentTypeDataTypeElementary, segment_type);
}

TEST(CipEpath, GetSegmentTypeReserved) {
  const unsigned char message[] = {0xE0};
  SegmentType segment_type = GetPathSegmentType(message);
  CHECK_EQUAL(kSegmentTypeReserved, segment_type);
}

/** Port segment tests **/
TEST(CipEpath, GetPortSegmentExtendedAddressSizeTrue) {
  const unsigned char message[] = {0x10};
  bool extended_address = GetPathPortSegmentExtendedLinkAddressSizeBit(message);
  CHECK_EQUAL(true, extended_address);
}

TEST(CipEpath, GetPortSegmentExtendedAddressSizeFalse) {
  const unsigned char message[] = {0x00};
  bool extended_address = GetPathPortSegmentExtendedLinkAddressSizeBit(message);
  CHECK_EQUAL(false, extended_address);
}

TEST(CipEpath, GetPortSegmentPortIdentifier) {
  const unsigned char message[] = {0x0F};
  unsigned int port = GetPathPortSegmentPortIdentifier(message);
  unsigned int mes = (unsigned int)(message[0]);
  CHECK_EQUAL(15, mes);
}

TEST(CipEpath, SetPortSegmentPortIdentifier) {
  unsigned char message[] = {0x00};
  SetPathPortSegmentPortIdentifier(15, message);
  CHECK_EQUAL(15, (unsigned int)(message[0]));
}

TEST(CipEpath, GetPortSegmentLinkAddressSize) {
  const unsigned char message[] = {0x10,0x04};
  unsigned int size = GetPathPortSegmentLinkAddressSize(message);
  CHECK_EQUAL(4, size);
}

TEST(CipEpath, GetPortSegmentExtendedPortNumberNoExtendedAddress) {
  const unsigned char message[] = {0x0F, 0x22, 0x64};
  unsigned int extended_port = GetPathPortSegmentExtendedPortNumber(message);
  CHECK_EQUAL(25634, extended_port);
}

TEST(CipEpath, GetPortSegmentExtendedPortNumberWithExtendedAddress) {
  const unsigned char message[] = {0x1F, 0x00, 0x22, 0x64};
  unsigned int extended_port = GetPathPortSegmentExtendedPortNumber(message);
  CHECK_EQUAL(25634, extended_port);
}

TEST(CipEpath, SetPortSegmentExtendedPortNoExtendedAddress) {
  unsigned char message[] = {0x00, 0x00, 0x00};
  const char expected_message[] = {0x0F, 0x22, 0x64};
  SetPathPortSegmentExtendedPortIdentifier((unsigned int)25634, message);
  MEMCMP_EQUAL(expected_message, message, 3);
}

TEST(CipEpath, SetPortSegmentExtendedPortWithExtendedAddress) {
  unsigned char message[] = {0x10, 0x00, 0x00, 0x00};
  const char expected_message[] = {0x1F, 0x00, 0x22, 0x64};
  SetPathPortSegmentExtendedPortIdentifier((unsigned int)25634, message);
  MEMCMP_EQUAL(expected_message, message, 4);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeClassId) {
  const unsigned char message[] = {0x20};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeClassId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeInstanceId) {
  const unsigned char message[] = {0x24};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeInstanceId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeMemberId) {
  const unsigned char message[] = {0x28};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeMemberId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeConnectionPoint) {
  const unsigned char message[] = {0x2C};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeConnectionPoint, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeAttributeId) {
  const unsigned char message[] = {0x30};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeAttributeId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeSpecial) {
  const unsigned char message[] = {0x34};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeSpecial, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeServiceId) {
  const unsigned char message[] = {0x38};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeServiceId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeExtendedLogical) {
  const unsigned char message[] = {0x3C};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeExtendedLogical, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalFormatEightBits) {
  const unsigned char message[] = {0x20};
  const LogicalSegmentLogicalFormat format = GetPathLogicalSegmentLogicalFormat(message);
  CHECK_EQUAL(kLogicalSegmentLogicalFormatEightBit, format);
}

TEST(CipEpath, GetLogicalSegmentLogicalFormatSixteenBits) {
  const unsigned char message[] = {0x21};
  const LogicalSegmentLogicalFormat format = GetPathLogicalSegmentLogicalFormat(message);
  CHECK_EQUAL(kLogicalSegmentLogicalFormatSixteenBit, format);
}

TEST(CipEpath, GetLogicalSegmentLogicalFormatThirtyTwoBits) {
  const unsigned char message[] = {0x22};
  const LogicalSegmentLogicalFormat format = GetPathLogicalSegmentLogicalFormat(message);
  CHECK_EQUAL(kLogicalSegmentLogicalFormatThirtyTwoBit, format);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeReserved) {
  const unsigned char message[] = {0x3C, 0x00};
  const LogicalSegmentExtendedLogicalType extended_type = GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeReserved, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeArrayIndex) {
  const unsigned char message[] = {0x3C, 0x01};
  const LogicalSegmentExtendedLogicalType extended_type = GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeArrayIndex, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeIndirectArrayIndex) {
  const unsigned char message[] = {0x3C, 0x02};
  const LogicalSegmentExtendedLogicalType extended_type = GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeIndirectArrayIndex, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeBitIndex) {
  const unsigned char message[] = {0x3C, 0x03};
  const LogicalSegmentExtendedLogicalType extended_type = GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeBitIndex, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeIndirectBitIndex) {
  const unsigned char message[] = {0x3C, 0x04};
  const LogicalSegmentExtendedLogicalType extended_type = GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeIndirectBitIndex, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeStructureMemberNumber) {
  const unsigned char message[] = {0x3C, 0x05};
  const LogicalSegmentExtendedLogicalType extended_type = GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeStructureMemberNumber, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeStructureMemberHandle) {
  const unsigned char message[] = {0x3C, 0x06};
  const LogicalSegmentExtendedLogicalType extended_type = GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeStructureMemberHandle, extended_type);
}

TEST(CipEpath, GetLogicalSegmentSpecialTypeLogicalTypeReserved) {
  const unsigned char message[] = {0x35};
  LogicalSegmentSpecialTypeLogicalFormat special_type = GetPathLogicalSegmentSpecialTypeLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentSpecialTypeLogicalFormatReserved, special_type);
}

TEST(CipEpath, GetLogicalSegmentSpecialTypeLogicalTypeElectronicKey) {
  const unsigned char message[] = {0x34};
  LogicalSegmentSpecialTypeLogicalFormat special_type = GetPathLogicalSegmentSpecialTypeLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentSpecialTypeLogicalFormatElectronicKey, special_type);
}

TEST(CipEpath, GetPathLogicalSegmentElectronicKeyFormatReserved) {
  const unsigned char message[] = {0x34, 0x00};
  ElectronicKeySegmentFormat key_format = GetPathLogicalSegmentElectronicKeyFormat(message);
  CHECK_EQUAL(kElectronicKeySegmentFormatReserved, key_format);
}

TEST(CipEpath, GetPathLogicalSegmentElectronicKeyFormat4) {
  const unsigned char message[] = {0x34, 0x04};
  ElectronicKeySegmentFormat key_format = GetPathLogicalSegmentElectronicKeyFormat(message);
  CHECK_EQUAL(kElectronicKeySegmentFormatKeyFormat4, key_format);
}

TEST(CipEpath, GetLogicalSegmentElectronicKeyFormat4) {
  const unsigned char message[] = {0x34,0x04};
  ElectronicKeyFormat4 *electronic_key = GetPathLogicalSegmentElectronicKeyFormat4(message);

  //TODO: check on how to free in CppuTest
  //free(electronic_key);
}
