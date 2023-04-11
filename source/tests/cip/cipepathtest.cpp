/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "cipepath.h"

}

TEST_GROUP (CipEpath) {

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
/* Set Segment Type tests*/
TEST(CipEpath, SetSegmentTypePortSegment) {
  unsigned char message[] = {0xFF};
  SetPathSegmentType(kSegmentTypePortSegment, message);
  CHECK_EQUAL(0x00, message[0]);
}

TEST(CipEpath, SetSegmentTypeLogicalSegment) {
  unsigned char message[] = {0xFF};
  SetPathSegmentType(kSegmentTypeLogicalSegment, message);
  CHECK_EQUAL(0x20, message[0]);
}

TEST(CipEpath, SetSegmentTypeNetworkSegment) {
  unsigned char message[] = {0xFF};
  SetPathSegmentType(kSegmentTypeNetworkSegment, message);
  CHECK_EQUAL(0x40, message[0]);
}

TEST(CipEpath, SetSegmentTypeSymbolicSegment) {
  unsigned char message[] = {0xFF};
  SetPathSegmentType(kSegmentTypeSymbolicSegment, message);
  CHECK_EQUAL(0x60, message[0]);
}

TEST(CipEpath, SetSegmentTypeDataSegment) {
  unsigned char message[] = {0xFF};
  SetPathSegmentType(kSegmentTypeDataSegment, message);
  CHECK_EQUAL(0x80, message[0]);
}

TEST(CipEpath, SetSegmentTypeDataTypeConstructed) {
  unsigned char message[] = {0xFF};
  SetPathSegmentType(kSegmentTypeDataTypeConstructed, message);
  CHECK_EQUAL(0xA0, message[0]);
}

TEST(CipEpath, SetSegmentTypeDataTypeElementary) {
  unsigned char message[] = {0xFF};
  SetPathSegmentType(kSegmentTypeDataTypeElementary, message);
  CHECK_EQUAL(0xC0, message[0]);
}

TEST(CipEpath, SetSegmentTypeReserved) {
  unsigned char message[] = {0xFF};
  SetPathSegmentType(kSegmentTypeReserved, message);
  CHECK_EQUAL(0xE0, message[0]);
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
  CHECK_EQUAL( 15, (unsigned int)(message[0]) );
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
  SetPathPortSegmentExtendedPortIdentifier( (unsigned int)25634, message );
  MEMCMP_EQUAL(expected_message, message, 3);
}

TEST(CipEpath, SetPortSegmentExtendedPortWithExtendedAddress) {
  unsigned char message[] = {0x10, 0x00, 0x00, 0x00};
  const char expected_message[] = {0x1F, 0x00, 0x22, 0x64};
  SetPathPortSegmentExtendedPortIdentifier( (unsigned int)25634, message );
  MEMCMP_EQUAL(expected_message, message, 4);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeClassId) {
  const unsigned char message[] = {0x20};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeClassId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeInstanceId) {
  const unsigned char message[] = {0x24};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeInstanceId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeMemberId) {
  const unsigned char message[] = {0x28};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeMemberId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeConnectionPoint) {
  const unsigned char message[] = {0x2C};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeConnectionPoint, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeAttributeId) {
  const unsigned char message[] = {0x30};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeAttributeId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeSpecial) {
  const unsigned char message[] = {0x34};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeSpecial, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeServiceId) {
  const unsigned char message[] = {0x38};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeServiceId, type);
}

TEST(CipEpath, GetLogicalSegmentLogicalTypeExtendedLogical) {
  const unsigned char message[] = {0x3C};
  const LogicalSegmentLogicalType type = GetPathLogicalSegmentLogicalType(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalTypeExtendedLogical, type);
}

TEST(CipEpath, SetLogicalSegmentLogicalTypeClassId) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeClassId, message);
  CHECK_EQUAL(SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_TYPE_CLASS_ID,
              message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalTypeInstanceId) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeInstanceId,
                                   message);
  CHECK_EQUAL(SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_TYPE_INSTANCE_ID,
              message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalTypeMemberId) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeMemberId, message);
  CHECK_EQUAL(SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_TYPE_MEMBER_ID,
              message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalTypeConnectionPoint) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeConnectionPoint,
                                   message);
  CHECK_EQUAL(
    SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_TYPE_CONNECTION_POINT,
    message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalTypeAttributeId) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeAttributeId,
                                   message);
  CHECK_EQUAL(SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID,
              message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalTypeSpecial) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeSpecial, message);
  CHECK_EQUAL(SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_TYPE_SPECIAL,
              message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalTypeServiceId) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeServiceId,
                                   message);
  CHECK_EQUAL(SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_TYPE_SERVICE_ID,
              message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalTypeExtendedLogicalPath) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalType(kLogicalSegmentLogicalTypeExtendedLogical,
                                   message);
  CHECK_EQUAL(
    SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_TYPE_EXTENDED_LOGICAL,
    message[0]);
}

TEST(CipEpath, GetLogicalSegmentLogicalFormatEightBits) {
  const unsigned char message[] = {0x20};
  const LogicalSegmentLogicalFormat format = GetPathLogicalSegmentLogicalFormat(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalFormatEightBit, format);
}

TEST(CipEpath, GetLogicalSegmentLogicalFormatSixteenBits) {
  const unsigned char message[] = {0x21};
  const LogicalSegmentLogicalFormat format = GetPathLogicalSegmentLogicalFormat(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalFormatSixteenBit, format);
}

TEST(CipEpath, GetLogicalSegmentLogicalFormatThirtyTwoBits) {
  const unsigned char message[] = {0x22};
  const LogicalSegmentLogicalFormat format = GetPathLogicalSegmentLogicalFormat(
    message);
  CHECK_EQUAL(kLogicalSegmentLogicalFormatThirtyTwoBit, format);
}

TEST(CipEpath, SetLogicalSegmentLogicalFormatEightBits) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalFormat(kLogicalSegmentLogicalFormatEightBit,
                                     message);
  CHECK_EQUAL(SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_FORMAT_EIGHT_BIT,
              message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalFormatSixteenBits) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalFormat(kLogicalSegmentLogicalFormatSixteenBit,
                                     message);
  CHECK_EQUAL(SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT,
              message[0]);
}

TEST(CipEpath, SetLogicalSegmentLogicalFormatThirtyTwoBits) {
  CipOctet message[] = {SEGMENT_TYPE_LOGICAL_SEGMENT};
  SetPathLogicalSegmentLogicalFormat(kLogicalSegmentLogicalFormatThirtyTwoBit,
                                     message);
  CHECK_EQUAL(
    SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_FORMAT_THIRTY_TWO_BIT,
    message[0]);
}

TEST(CipEpath, GetLogicalValue8Bit) {
  CipOctet *message = (CipOctet *)calloc ( 6, sizeof(CipOctet) );
  CipOctet *original_address = message;
  message[0] = SEGMENT_TYPE_LOGICAL_SEGMENT | LOGICAL_SEGMENT_FORMAT_EIGHT_BIT;
  message[1] = 20;
  message[2] = 21;
  message[3] = 22;
  message[4] = 23;
  message[5] = 24;
  CipDword result = CipEpathGetLogicalValue( (const CipOctet **)&message );
  CHECK_EQUAL(20, result);
  free(original_address);
}

TEST(CipEpath, GetLogicalValue16Bit) {
  CipOctet *message = (CipOctet *)calloc ( 6, sizeof(CipOctet) );
  CipOctet *original_address = message;
  message[0] = SEGMENT_TYPE_LOGICAL_SEGMENT |
               LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT;
  message[1] = 0;
  message[2] = 21;
  message[3] = 22;
  message[4] = 23;
  message[5] = 24;
  CipDword result = CipEpathGetLogicalValue( (const CipOctet **)&message );
  CHECK_EQUAL(5653, result);
  free(original_address);
}

TEST(CipEpath, GetLogicalValue32Bit) {
  CipOctet *message = (CipOctet *)calloc ( 6, sizeof(CipOctet) );
  CipOctet *original_address = message;
  message[0] = SEGMENT_TYPE_LOGICAL_SEGMENT |
               LOGICAL_SEGMENT_FORMAT_THIRTY_TWO_BIT;
  message[1] = 0;
  message[2] = 21;
  message[3] = 22;
  message[4] = 23;
  message[5] = 24;
  CipDword result = CipEpathGetLogicalValue( (const CipOctet **)&message );
  CHECK_EQUAL(404166165, result);
  free(original_address);
}

TEST(CipEpath, SetLogicalValue8Bit) {
  const CipUsint value = 54;
  CipMessageRouterResponse response = {0};
  response.message.current_message_position = response.message.message_buffer;
  CipEpathSetLogicalValue(value, kLogicalSegmentLogicalFormatEightBit,
                          &response);
  CHECK_EQUAL(value, *response.message.message_buffer);
}

TEST(CipEpath, SetLogicalValue16Bit) {
  const CipUsint value = 54;
  CipMessageRouterResponse response = {0};
  response.message.current_message_position = response.message.message_buffer;
  CipEpathSetLogicalValue(value,
                          kLogicalSegmentLogicalFormatSixteenBit,
                          &response);
  CHECK_EQUAL( value, *(response.message.message_buffer + 1) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 2) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 3) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 4) );
}

TEST(CipEpath, SetLogicalValue32Bit) {
  const CipUsint value = 54;
  CipMessageRouterResponse response = {0};
  response.message.current_message_position = response.message.message_buffer;
  CipEpathSetLogicalValue(value,
                          kLogicalSegmentLogicalFormatThirtyTwoBit,
                          &response);
  CHECK_EQUAL( value, *(response.message.message_buffer + 1) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 2) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 3) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 4) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 5) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 6) );
  CHECK_EQUAL( 0, *(response.message.message_buffer + 7) );
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeReserved) {
  const unsigned char message[] = {0x3C, 0x00};
  const LogicalSegmentExtendedLogicalType extended_type =
    GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeReserved, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeArrayIndex) {
  const unsigned char message[] = {0x3C, 0x01};
  const LogicalSegmentExtendedLogicalType extended_type =
    GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeArrayIndex, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeIndirectArrayIndex) {
  const unsigned char message[] = {0x3C, 0x02};
  const LogicalSegmentExtendedLogicalType extended_type =
    GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeIndirectArrayIndex,
              extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeBitIndex) {
  const unsigned char message[] = {0x3C, 0x03};
  const LogicalSegmentExtendedLogicalType extended_type =
    GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeBitIndex, extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeIndirectBitIndex) {
  const unsigned char message[] = {0x3C, 0x04};
  const LogicalSegmentExtendedLogicalType extended_type =
    GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeIndirectBitIndex,
              extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeStructureMemberNumber) {
  const unsigned char message[] = {0x3C, 0x05};
  const LogicalSegmentExtendedLogicalType extended_type =
    GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeStructureMemberNumber,
              extended_type);
}

TEST(CipEpath, GetLogicalSegmentExtendedLogicalTypeStructureMemberHandle) {
  const unsigned char message[] = {0x3C, 0x06};
  const LogicalSegmentExtendedLogicalType extended_type =
    GetPathLogicalSegmentExtendedLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentExtendedLogicalTypeStructureMemberHandle,
              extended_type);
}

TEST(CipEpath, GetLogicalSegmentSpecialTypeLogicalTypeReserved) {
  const unsigned char message[] = {0x35};
  LogicalSegmentSpecialTypeLogicalFormat special_type =
    GetPathLogicalSegmentSpecialTypeLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentSpecialTypeLogicalFormatReserved, special_type);
}

TEST(CipEpath, GetLogicalSegmentSpecialTypeLogicalTypeElectronicKey) {
  const unsigned char message[] = {0x34};
  LogicalSegmentSpecialTypeLogicalFormat special_type =
    GetPathLogicalSegmentSpecialTypeLogicalType(message);
  CHECK_EQUAL(kLogicalSegmentSpecialTypeLogicalFormatElectronicKey,
              special_type);
}

TEST(CipEpath, GetPathLogicalSegmentElectronicKeyFormatReserved) {
  const unsigned char message[] = {0x34, 0x00};
  ElectronicKeySegmentFormat key_format =
    GetPathLogicalSegmentElectronicKeyFormat(message);
  CHECK_EQUAL(kElectronicKeySegmentFormatReserved, key_format);
}

TEST(CipEpath, GetElectronicKeyFormat4FromMessage) {
  const unsigned char message[] = {0x34, 0x04};
  ElectronicKeySegmentFormat key_format =
    GetPathLogicalSegmentElectronicKeyFormat(message);
  CHECK_EQUAL(kElectronicKeySegmentFormatKeyFormat4, key_format);
}

TEST(CipEpath, GetLogicalSegmentElectronicKeyFormat4) {
  /* Size of an electronic key is 1 + 1 + 8 (Segment, Key format, Key) */
  const CipOctet message[] =
  { 0x34, 0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x04, 0x05};
  const CipOctet *message_pp = (const CipOctet *)message;
  const CipOctet *message_buffer = message;
  ElectronicKeyFormat4 *electronic_key = ElectronicKeyFormat4New();
  GetElectronicKeyFormat4FromMessage(&message_pp, electronic_key);

  MEMCMP_EQUAL(message_buffer + 2, electronic_key, 8);

  ElectronicKeyFormat4Delete(&electronic_key);
}

TEST(CipEpath, GetPathNetworkSegmentSubtypeReserved) {
  const unsigned char message[] = {0x40};
  NetworkSegmentSubtype sub_type = GetPathNetworkSegmentSubtype(message);
  CHECK_EQUAL(kNetworkSegmentSubtypeReserved, sub_type);
}

TEST(CipEpath, GetPathNetworkSegmentSubtypeScheduled) {
  const unsigned char message[] = {0x41};
  NetworkSegmentSubtype sub_type = GetPathNetworkSegmentSubtype(message);
  CHECK_EQUAL(kNetworkSegmentSubtypeScheduleSegment, sub_type);
}

TEST(CipEpath, GetPathNetworkSegmentSubtypeFixedTag) {
  const unsigned char message[] = {0x42};
  NetworkSegmentSubtype sub_type = GetPathNetworkSegmentSubtype(message);
  CHECK_EQUAL(kNetworkSegmentSubtypeFixedTagSegment, sub_type);
}

TEST(CipEpath,
     GetPathNetworkSegmentSubtypeProductionInhibitTimerInMilliseconds) {
  const unsigned char message[] = {0x43};
  NetworkSegmentSubtype sub_type = GetPathNetworkSegmentSubtype(message);
  CHECK_EQUAL(kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds,
              sub_type);
}

TEST(CipEpath, GetPathNetworkSegmentSubtypeSafety) {
  const unsigned char message[] = {0x44};
  NetworkSegmentSubtype sub_type = GetPathNetworkSegmentSubtype(message);
  CHECK_EQUAL(kNetworkSegmentSubtypeSafetySegment, sub_type);
}

TEST(CipEpath,
     GetPathNetworkSegmentSubtypeProductionInhibitTimerInMicroseconds) {
  const unsigned char message[] = {0x50};
  NetworkSegmentSubtype sub_type = GetPathNetworkSegmentSubtype(message);
  CHECK_EQUAL(kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds,
              sub_type);
}

TEST(CipEpath, GetPathNetworkSegmentSubtypeExtendedNetwork) {
  const unsigned char message[] = {0x5F};
  NetworkSegmentSubtype sub_type = GetPathNetworkSegmentSubtype(message);
  CHECK_EQUAL(kNetworkSegmentSubtypeExtendedNetworkSegment, sub_type);
}

TEST(CipEpath, GetPathNetworkSegmentProductionInhibitTimeInMilliseconds) {
  const unsigned char message[] = {0x43, 0xFF};
  CipUsint time_im_milliseconds =
    GetPathNetworkSegmentProductionInhibitTimeInMilliseconds(message);
  CHECK_EQUAL(255, time_im_milliseconds);
}

TEST(CipEpath, GetPathNetworkSegmentProductionInhibitTimeInMicroseconds) {
  const unsigned char message[] = {0x50, 0x02, 0x00, 0x00, 0x00, 0xFF};
  CipUdint time_in_microseconds =
    GetPathNetworkSegmentProductionInhibitTimeInMicroseconds(message);
  CHECK_EQUAL(4278190080, time_in_microseconds);
}

TEST(CipEpath, GetPathDataSegmentSubtypeReserverd) {
  const unsigned char message[] = {0x81};
  DataSegmentSubtype type = GetPathDataSegmentSubtype(message);
  CHECK_EQUAL(kDataSegmentSubtypeReserved, type);
}

TEST(CipEpath, GetPathDataSegmentSubtypeSimpleData) {
  const unsigned char message[] = {0x80};
  DataSegmentSubtype type = GetPathDataSegmentSubtype(message);
  CHECK_EQUAL(kDataSegmentSubtypeSimpleData, type);
}

TEST(CipEpath, GetPathDataSegmentSubtypeANSIExtendedSymbol) {
  const unsigned char message[] = {0x91};
  DataSegmentSubtype type = GetPathDataSegmentSubtype(message);
  CHECK_EQUAL(kDataSegmentSubtypeANSIExtendedSymbol, type);
}

TEST(CipEpath, GetPathDataSegmentSimpleDataWordLength) {
  const unsigned char message[] = {0x80, 0x25};
  CipUsint length = GetPathDataSegmentSimpleDataWordLength(message);
  CHECK_EQUAL(37, length);
}

TEST(CipEpath, GetPathSymbolicSegmentFormatASCII) {
  const unsigned char message[] = {0x61};
  SymbolicSegmentFormat format = GetPathSymbolicSegmentFormat(message);
  CHECK_EQUAL(kSymbolicSegmentFormatASCII, format);
}

TEST(CipEpath, GetPathSymbolicSegmentFormatExtendedFormat) {
  const unsigned char message[] = {0x60};
  SymbolicSegmentFormat format = GetPathSymbolicSegmentFormat(message);
  CHECK_EQUAL(kSymbolicSegmentFormatExtendedString, format);
}

TEST(CipEpath, GetPathSymbolicSegmentASCIIFormatLength) {
  const unsigned char message[] = {0x61};
  SymbolicSegmentFormat format = GetPathSymbolicSegmentFormat(message);
  CHECK_EQUAL(kSymbolicSegmentFormatASCII, format);
}

TEST(CipEpath, GetPathSymbolicSegmentNumericTypeReserved) {
  const unsigned char message[] = {0x60, 0xC0};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentNumericType(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatReserved, format);
}

TEST(CipEpath, GetPathSymbolicSegmentNumericTypeUSINT) {
  const unsigned char message[] = {0x60, 0xC6};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentNumericType(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatNumericSymbolUSINT, format);
}

TEST(CipEpath, GetPathSymbolicSegmentNumericTypeUINT) {
  const unsigned char message[] = {0x60, 0xC7};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentNumericType(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatNumericSymbolUINT, format);
}

TEST(CipEpath, GetPathSymbolicSegmentNumericTypeUDINT) {
  const unsigned char message[] = {0x60, 0xC8};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentNumericType(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatNumericSymbolUDINT, format);
}

TEST(CipEpath, GetPathSymbolicSegmentExtendedFormatReserverd) {
  const unsigned char message[] = {0x60, 0x00};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentExtendedFormat(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatReserved, format);
}

TEST(CipEpath, GetPathSymbolicSegmentExtendedFormatDoubleChars) {
  const unsigned char message[] = {0x60, 0x20};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentExtendedFormat(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatDoubleByteChars, format);
}

TEST(CipEpath, GetPathSymbolicSegmentExtendedFormatTripleChars) {
  const unsigned char message[] = {0x60, 0x40};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentExtendedFormat(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatTripleByteChars, format);
}

TEST(CipEpath, GetPathSymbolicSegmentExtendedFormatUSINT) {
  const unsigned char message[] = {0x60, 0xC6};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentExtendedFormat(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatNumericSymbolUSINT, format);
}

TEST(CipEpath, GetPathSymbolicSegmentExtendedFormatUINT) {
  const unsigned char message[] = {0x60, 0xC7};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentExtendedFormat(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatNumericSymbolUINT, format);
}

TEST(CipEpath, GetPathSymbolicSegmentExtendedFormatUDINT) {
  const unsigned char message[] = {0x60, 0xC8};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentExtendedFormat(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatNumericSymbolUDINT, format);
}

TEST(CipEpath, GetPathSymbolicSegmentExtendedFormatNumericReserved) {
  const unsigned char message[] = {0x60, 0xC9};
  SymbolicSegmentExtendedFormat format = GetPathSymbolicSegmentExtendedFormat(
    message);
  CHECK_EQUAL(kSymbolicSegmentExtendedFormatReserved, format);
}
