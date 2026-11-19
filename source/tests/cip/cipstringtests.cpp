/*******************************************************************************
 * Copyright (c) 2020, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************I*************************/

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "cip/cipstring.h"
#include "src/opener_api.h"
}

TEST_GROUP(CipString){void setup(){mock().disable();
}
}
;

TEST(CipString, CipStringNClearNullPointer) {
  CipStringN* null_ptr = NULL;
  ClearCipStringN(null_ptr);
};

TEST(CipString, CipStringNFreeNullPointer) {
  CipStringN* null_ptr = NULL;
  FreeCipStringN(null_ptr);
};

TEST(CipString, ClearCipStringNWithContent) {
  CipStringN* string;
  string = reinterpret_cast<CipStringN*>(CipCalloc(sizeof(CipStringN), 1));
  string->size             = 3;
  string->length           = 10;
  string->string           = reinterpret_cast<EipByte*>(CipCalloc(10, 3));
  CipStringN* returned_ptr = ClearCipStringN(string);
  POINTERS_EQUAL(string, returned_ptr);
  CHECK_EQUAL(0, string->size);
  CHECK_EQUAL(0, string->length);
  POINTERS_EQUAL(NULL, string->string);
  FreeCipStringN(string);
};

TEST(CipString, FreeCipStringNWithContent) {
  CipStringN* string;
  string = reinterpret_cast<CipStringN*>(CipCalloc(sizeof(CipStringN), 1));
  string->size   = 3;
  string->length = 10;
  string->string = reinterpret_cast<EipByte*>(CipCalloc(10, 3));
  FreeCipStringN(string);
};

TEST(CipString, CreateStringNFromData) {
  const CipOctet data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  CipStringN* string;
  string = reinterpret_cast<CipStringN*>(CipCalloc(1, sizeof(CipStringN)));
  SetCipStringNByData(string, 4, 3, data);
  CHECK_EQUAL(3, string->size);
  CHECK_EQUAL(4, string->length);
  MEMCMP_EQUAL(data, string->string, sizeof(data));
  FreeCipStringN(string);
}

TEST(CipString, CreateStringNFromCString) {
  const char data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0};
  CipStringN* string;
  string = reinterpret_cast<CipStringN*>(CipCalloc(1, sizeof(CipStringN)));
  SetCipStringNByCstr(string, data, 3);
  CHECK_EQUAL(3, string->size);
  CHECK_EQUAL(4, string->length);
  MEMCMP_EQUAL(data, string->string, strlen(data));
  FreeCipStringN(string);
}

/*** CipString2 ***/
TEST(CipString, CipString2ClearNullPointer) {
  CipString2* null_ptr = NULL;
  ClearCipString2(null_ptr);
};

TEST(CipString, CipString2FreeNullPointer) {
  CipString2* null_ptr = NULL;
  FreeCipString2(null_ptr);
};

TEST(CipString, ClearCipString2WithContent) {
  CipString2* string;
  string = reinterpret_cast<CipString2*>(CipCalloc(sizeof(CipString2), 1));
  string->length           = 10;
  string->string           = reinterpret_cast<CipWord*>(CipCalloc(10, 2));
  CipString2* returned_ptr = ClearCipString2(string);
  POINTERS_EQUAL(string, returned_ptr);
  CHECK_EQUAL(0, string->length);
  POINTERS_EQUAL(NULL, string->string);
  FreeCipString2(string);
};

TEST(CipString, FreeCipString2WithContent) {
  CipString2* string;
  string = reinterpret_cast<CipString2*>(CipCalloc(sizeof(CipString2), 1));
  string->length = 10;
  string->string = reinterpret_cast<CipWord*>(CipCalloc(10, 2));
  FreeCipString2(string);
};

TEST(CipString, CreateString2FromData) {
  const CipOctet data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  CipString2* string;
  string = reinterpret_cast<CipString2*>(CipCalloc(1, sizeof(CipString2)));
  SetCipString2ByData(string, 6, data);
  CHECK_EQUAL(6, string->length);
  MEMCMP_EQUAL(data, string->string, sizeof(data));
  FreeCipString2(string);
}

TEST(CipString, CreateString2FromCString) {
  const char data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0};
  CipString2* string;
  string = reinterpret_cast<CipString2*>(CipCalloc(1, sizeof(CipString2)));
  SetCipString2ByCstr(string, data);
  CHECK_EQUAL(6, string->length);
  MEMCMP_EQUAL(data, string->string, strlen(data));
  FreeCipString2(string);
}

/*** CipString ***/
TEST(CipString, CipStringClearNullPointer) {
  CipString* null_ptr = NULL;
  ClearCipString(null_ptr);
};

TEST(CipString, CipStringFreeNullPointer) {
  CipString* null_ptr = NULL;
  FreeCipString(null_ptr);
};

TEST(CipString, ClearCipStringWithContent) {
  CipString* string;
  string = reinterpret_cast<CipString*>(CipCalloc(sizeof(CipString), 1));
  string->length          = 10;
  string->string          = reinterpret_cast<CipByte*>(CipCalloc(10, 1));
  CipString* returned_ptr = ClearCipString(string);
  POINTERS_EQUAL(string, returned_ptr);
  CHECK_EQUAL(0, string->length);
  POINTERS_EQUAL(NULL, string->string);
  FreeCipString(string);
};

TEST(CipString, FreeCipStringWithContent) {
  CipString* string;
  string = reinterpret_cast<CipString*>(CipCalloc(sizeof(CipString), 1));
  string->length = 10;
  string->string = reinterpret_cast<CipByte*>(CipCalloc(10, 1));
  FreeCipString(string);
};

TEST(CipString, CreateStringFromData) {
  const CipOctet data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  CipString* string;
  string = reinterpret_cast<CipString*>(CipCalloc(1, sizeof(CipString)));
  SetCipStringByData(string, sizeof(data), data);
  CHECK_EQUAL(12, string->length);
  MEMCMP_EQUAL(data, string->string, sizeof(data));
  FreeCipString(string);
}

TEST(CipString, CreateStringFromCString) {
  const char data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0};
  CipString* string;
  string = reinterpret_cast<CipString*>(CipCalloc(1, sizeof(CipString)));
  SetCipStringByCstr(string, data);
  CHECK_EQUAL(12, string->length);
  MEMCMP_EQUAL(data, string->string, strlen(data));
  FreeCipString(string);
}

/*** CipShortString ***/
TEST(CipString, CipShortStringClearNullPointer) {
  CipShortString* null_ptr = NULL;
  ClearCipShortString(null_ptr);
};

TEST(CipString, CipShortStringFreeNullPointer) {
  CipShortString* null_ptr = NULL;
  FreeCipShortString(null_ptr);
};

TEST(CipString, ClearCipShortStringWithContent) {
  CipShortString* string;
  string =
      reinterpret_cast<CipShortString*>(CipCalloc(sizeof(CipShortString), 1));
  string->length               = 10;
  string->string               = reinterpret_cast<CipByte*>(CipCalloc(10, 1));
  CipShortString* returned_ptr = ClearCipShortString(string);
  POINTERS_EQUAL(string, returned_ptr);
  CHECK_EQUAL(0, string->length);
  POINTERS_EQUAL(NULL, string->string);
  FreeCipShortString(string);
};

TEST(CipString, FreeCipShortStringWithContent) {
  CipShortString* string;
  string =
      reinterpret_cast<CipShortString*>(CipCalloc(sizeof(CipShortString), 1));
  string->length = 10;
  string->string = reinterpret_cast<CipByte*>(CipCalloc(10, 1));
  FreeCipShortString(string);
};

TEST(CipString, CreateShortStringFromData) {
  const CipOctet data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  CipShortString* string;
  string =
      reinterpret_cast<CipShortString*>(CipCalloc(1, sizeof(CipShortString)));
  SetCipShortStringByData(string, sizeof(data), data);
  CHECK_EQUAL(12, string->length);
  MEMCMP_EQUAL(data, string->string, sizeof(data));
  FreeCipShortString(string);
}

TEST(CipString, CreateShortStringFromCString) {
  const char data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0};
  CipShortString* string;
  string =
      reinterpret_cast<CipShortString*>(CipCalloc(1, sizeof(CipShortString)));
  SetCipShortStringByCstr(string, data);
  CHECK_EQUAL(12, string->length);
  MEMCMP_EQUAL(data, string->string, strlen(data));
  FreeCipShortString(string);
}
