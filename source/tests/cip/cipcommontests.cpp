/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "cipcommon.h"

}

TEST_GROUP(CipCommon) {

};

TEST(CipCommon, GetSizeOfAttributeCipBool) {
  CipAttributeStruct attribute;
  attribute.type = kCipBool;
  CHECK_EQUAL(sizeof(CipBool), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipSint) {
  CipAttributeStruct attribute;
  attribute.type = kCipSint;
  CHECK_EQUAL(sizeof(CipSint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipInt) {
  CipAttributeStruct attribute;
  attribute.type = kCipInt;
  CHECK_EQUAL(sizeof(CipInt), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipDint) {
  CipAttributeStruct attribute;
  attribute.type = kCipDint;
  CHECK_EQUAL(sizeof(CipDint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipUsint) {
  CipAttributeStruct attribute;
  attribute.type = kCipUsint;
  CHECK_EQUAL(sizeof(CipUsint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipUint) {
  CipAttributeStruct attribute;
  attribute.type = kCipUint;
  CHECK_EQUAL(sizeof(CipUint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipUdint) {
  CipAttributeStruct attribute;
  attribute.type = kCipUdint;
  CHECK_EQUAL(sizeof(CipUdint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipReal) {
  CipAttributeStruct attribute;
  attribute.type = kCipReal;
  CHECK_EQUAL(sizeof(CipReal), GetSizeOfAttribute(&attribute) );
}

#ifdef OPENER_SUPPORT_64BIT_DATATYPES
TEST(CipCommon, GetSizeOfAttributeCipLreal) {
  CipAttributeStruct attribute;
  attribute.type = kCipLreal;
  CHECK_EQUAL(sizeof(CipLreal), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipUlint) {
  CipAttributeStruct attribute;
  attribute.type = kCipUlint;
  CHECK_EQUAL(sizeof(CipUlint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipLint) {
  CipAttributeStruct attribute;
  attribute.type = kCipLint;
  CHECK_EQUAL(sizeof(CipLint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipLword) {
  CipAttributeStruct attribute;
  attribute.type = kCipLword;
  CHECK_EQUAL(sizeof(CipLword), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipLtime) {
  CipAttributeStruct attribute;
  attribute.type = kCipLtime;
  CHECK_EQUAL(sizeof(CipLint), GetSizeOfAttribute(&attribute) );
}
#endif /* OPENER_SUPPORT_64BIT_DATATYPES */

TEST(CipCommon, GetSizeOfAttributeCipStime) {
  CipAttributeStruct attribute;
  attribute.type = kCipStime;
  CHECK_EQUAL(sizeof(CipDint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipData) {
  CipAttributeStruct attribute;
  attribute.type = kCipDate;
  CHECK_EQUAL(sizeof(CipUint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipTimeOfDay) {
  CipAttributeStruct attribute;
  attribute.type = kCipTimeOfDay;
  CHECK_EQUAL(sizeof(CipUdint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipDateAndTime) {
  CipAttributeStruct attribute;
  attribute.type = kCipDateAndTime;
  CHECK_EQUAL(sizeof(CipUdint) + sizeof(CipUint),
              GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipString) {
  CipAttributeStruct attribute;
  char demo_string[] = "Hello World!";
  CipString test_string =
  { .length = sizeof(demo_string), .string = (EipByte *)demo_string };
  attribute.type = kCipString;
  attribute.data = (void *)&test_string;

  CHECK_EQUAL(
    sizeof(test_string.length) + test_string.length * sizeof(CipOctet),
    GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipByte) {
  CipAttributeStruct attribute;
  attribute.type = kCipByte;
  CHECK_EQUAL(sizeof(CipByte), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipWord) {
  CipAttributeStruct attribute;
  attribute.type = kCipWord;
  CHECK_EQUAL(sizeof(CipWord), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipDword) {
  CipAttributeStruct attribute;
  attribute.type = kCipDword;
  CHECK_EQUAL(sizeof(CipDword), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipString2) {
  CipAttributeStruct attribute;
  char demo_string[] = "H e l l o   W o r l d !"; /* \0 termination symbol is seen as second byte for ! */
  CipString test_string =
  { .length = sizeof(demo_string) / 2, .string = (EipByte *)demo_string };
  attribute.type = kCipString;
  attribute.data = (void *)&test_string;

  CHECK_EQUAL(
    sizeof(test_string.length) + test_string.length * sizeof(CipOctet),
    GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipFtime) {
  CipAttributeStruct attribute;
  attribute.type = kCipFtime;
  CHECK_EQUAL(sizeof(CipDint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipItime) {
  CipAttributeStruct attribute;
  attribute.type = kCipItime;
  CHECK_EQUAL(sizeof(CipInt), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipStringN) {
  CipAttributeStruct attribute;
  char demo_string[] = "Hello World!";
  CipStringN test_string =
  { .size = 1, .length = sizeof(demo_string),
    .string = (EipByte *) demo_string };
  attribute.type = kCipStringN;
  attribute.data = (void *) &test_string;

  CHECK_EQUAL(
    sizeof(test_string.size) + sizeof(test_string.length) + test_string.size * test_string.length *
    sizeof(CipOctet),
    GetSizeOfAttribute(
      &attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipTime) {
  CipAttributeStruct attribute;
  attribute.type = kCipTime;
  CHECK_EQUAL(sizeof(CipDint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipEpath) {
  /* TODO: Fix me */
  CipAttributeStruct attribute;
  attribute.type = kCipItime;
  CHECK_EQUAL(sizeof(CipInt), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipEngUnit) {
  CipAttributeStruct attribute;
  attribute.type = kCipEngUnit;
  CHECK_EQUAL(sizeof(CipUint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipUsintUsint) {
  CipAttributeStruct attribute;
  attribute.type = kCipUsintUsint;
  CHECK_EQUAL(2 * sizeof(CipUsint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipUdintUdintUdintUdintUdintString) {
  CipTcpIpNetworkInterfaceConfiguration config;
  char domain_name[] = "www.github.com/EIPStackGroup/OpENer";
  config.domain_name.length = sizeof(domain_name);
  config.domain_name.string = (EipByte *)domain_name;
  CipAttributeStruct attribute;
  attribute.type = kCipUdintUdintUdintUdintUdintString;
  attribute.data = (void *)&config;
  CHECK_EQUAL(
    5 * sizeof(CipUdint) + sizeof(CipUint) + sizeof(domain_name) *
    sizeof(EipByte),
    GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCip6Usint) {
  CipAttributeStruct attribute;
  attribute.type = kCip6Usint;
  CHECK_EQUAL(6 * sizeof(CipUsint), GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeCipMemberList) {
  CipAttributeStruct attribute;
  attribute.type = kCipMemberList;
  CHECK_EQUAL(0, GetSizeOfAttribute(&attribute) );
  /* Currently not implemented */
}

TEST(CipCommon, GetSizeOfAttributeCipByteArray) {
  CipByte data_array[] = {1,2,3,4,5,6,7,8,9};
  CipByteArray array;
  array.data = (EipByte *)&data_array;
  array.length = sizeof(data_array);
  CipAttributeStruct attribute;
  attribute.type = kCipByteArray;
  attribute.data = (void *)&array;
  CHECK_EQUAL(sizeof(CipUint) + array.length * sizeof(CipByte),
              GetSizeOfAttribute(&attribute) );
}

TEST(CipCommon, GetSizeOfAttributeInternalUint6) {
  CipAttributeStruct attribute;
  attribute.type = kInternalUint6;
  CHECK_EQUAL(6 * sizeof(CipUint), GetSizeOfAttribute(&attribute) );
}
