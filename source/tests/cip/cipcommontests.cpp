/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "opener_api.h"
#include "cipstring.h"

}

ENIPMessage message; /**< Test variable holds ENIP message*/
CipMessageRouterRequest request;
CipMessageRouterResponse response;

TEST_GROUP(CipCommon) {

  void setup() {
    InitializeENIPMessage(&message);
  }

};

TEST(CipCommon, EncodeCipBool) {
  const CipBool value = false;
  EncodeCipBool(&value, &message);
  CHECK_EQUAL(0, *(CipBool *)message.message_buffer);
  POINTERS_EQUAL(message.message_buffer + 1, message.current_message_position);
}

TEST(CipCommon, EncodeCipByte) {
  const CipByte value = 173U;
  EncodeCipBool(&value, &message);
  CHECK_EQUAL(value, *(CipByte *)message.message_buffer);
  POINTERS_EQUAL(message.message_buffer + 1, message.current_message_position);
}

TEST(CipCommon, EncodeCipWord) {
  const CipWord value = 53678U;
  EncodeCipWord(&value, &message);
  CHECK_EQUAL(value, *(CipWord *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 2, message.current_message_position);
}

TEST(CipCommon, EncodeCipDword) {
  const CipDword value = 5357678U;
  EncodeCipDword(&value, &message);
  CHECK_EQUAL(value, *(CipDword *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 4, message.current_message_position);
}

TEST(CipCommon, EncodeCipLword) {
  const CipLword value = 8353457678U;
  EncodeCipLword(&value, &message);
  CHECK_EQUAL(value, *(CipLword *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 8, message.current_message_position);
}


TEST(CipCommon, EncodeCipUsint) {
  const CipUsint value = 212U;
  EncodeCipBool(&value, &message);
  CHECK_EQUAL(value, *(CipUsint *)message.message_buffer);
  POINTERS_EQUAL(message.message_buffer + 1, message.current_message_position);
}

TEST(CipCommon, EncodeCipUint) {
  const CipUint value = 42568U;
  EncodeCipUint(&value, &message);
  CHECK_EQUAL(value, *(CipUint *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 2, message.current_message_position);
}

TEST(CipCommon, EncodeCipUdint) {
  const CipUdint value = 1653245U;
  EncodeCipUdint(&value, &message);
  CHECK_EQUAL(value, *(CipUdint *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 4, message.current_message_position);
}

TEST(CipCommon, EncodeCipUlint) {
  const CipUlint value = 5357678U;
  EncodeCipUlint(&value, &message);
  CHECK_EQUAL(value, *(CipUlint *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 8, message.current_message_position);
}


/*******************************************************************************
 * CIP string decoding tests.
 ******************************************************************************/

CipString *dst_string;


TEST_GROUP(StringDecode)
{
  void setup() {
    /* Initialize the destination string with junk to be overwritten. */
    dst_string = (CipString *)CipCalloc( 1, sizeof(CipString) );
    SetCipStringByCstr(dst_string, "spam");
  }

  void teardown() {
    FreeCipString(dst_string);
  }
};


/* Validate decoding a zero-length string. */
TEST(StringDecode, ZeroLength)
{
  /* Configure the serialized source message. */
  CipOctet data[] = {0, 0};
  request.request_data_size = sizeof(data);
  request.data = data;

  size_t bytes = DecodeCipString(dst_string, &request, &response);

  CHECK_EQUAL(sizeof(data), bytes);
  CHECK_EQUAL(0, dst_string->length);
  POINTERS_EQUAL(NULL, dst_string->string);
  POINTERS_EQUAL(&data[sizeof(data)], request.data);
  CHECK_EQUAL(kCipErrorSuccess, response.general_status);
}


/* Validate decoding a maximum-length string. */
TEST(StringDecode, MaxLength)
{
  /* Allocate and initialize a maximum length serialized string. */
  size_t data_size = sizeof(CipUint) /* 16-bit length */
                     + UINT16_MAX    /* String content */
                     + 1;            /* Pad byte for even size. */
  CipOctet *data = (CipOctet *)malloc(data_size);

  /* Set length to maximum. */
  data[0] = 0xff;
  data[1] = 0xff;

  /* Fill the string content. */
  memset(&data[2], 'x', UINT16_MAX);

  /* Configure the serialized source message. */
  request.request_data_size = data_size;
  request.data = data;

  size_t bytes = DecodeCipString(dst_string, &request, &response);

  CHECK_EQUAL(data_size, bytes);
  CHECK_EQUAL(UINT16_MAX, dst_string->length);
  MEMCMP_EQUAL(&data[2], dst_string->string, UINT16_MAX);
  POINTERS_EQUAL(&data[data_size], request.data);
  CHECK_EQUAL(kCipErrorSuccess, response.general_status);

  free(data);
}


/* Validate overwriting an empty string. */
TEST(StringDecode, OverwriteEmpty)
{
  ClearCipString(dst_string);

  /* Configure the serialized source message. */
  CipOctet data[] = {
    6, 0,                        /* 16-bit length */
    'f', 'o', 'o', 'b', 'a', 'r' /* String */
  };
  request.request_data_size = sizeof(data);
  request.data = data;

  size_t string_len = sizeof(data)
                      - sizeof(CipUint); /* Less length field. */

  size_t bytes = DecodeCipString(dst_string, &request, &response);

  CHECK_EQUAL(sizeof(data), bytes);
  CHECK_EQUAL(string_len, dst_string->length);
  MEMCMP_EQUAL(&data[sizeof(CipUint)], dst_string->string, string_len);
  POINTERS_EQUAL(&data[sizeof(data)], request.data);
  CHECK_EQUAL(kCipErrorSuccess, response.general_status);
}


/* Validate decoding a string with an odd number of octets. */
TEST(StringDecode, OddLength)
{
  /* Configure the serialized source message. */
  CipOctet data[] = {
    3, 0,          /* 16-bit length */
    'o', 'd', 'd', /* String */
    0              /* Pad byte for even size. */
  };
  request.request_data_size = sizeof(data);
  request.data = data;

  size_t string_len = sizeof(data)
                      - sizeof(CipUint) /* Less length field. */
                      - 1;              /* Less pad. */

  size_t bytes = DecodeCipString(dst_string, &request, &response);

  CHECK_EQUAL(sizeof(data), bytes);
  CHECK_EQUAL(string_len, dst_string->length);
  MEMCMP_EQUAL(&data[sizeof(CipUint)], dst_string->string, string_len);
  POINTERS_EQUAL(&data[sizeof(data)], request.data);
  CHECK_EQUAL(kCipErrorSuccess, response.general_status);
}


/* Validate decoding a string with an even number of octets. */
TEST(StringDecode, EvenLength)
{
  /* Configure the serialized source message. */
  CipOctet data[] = {
    4, 0,              /* 16-bit length */
    'e', 'v', 'e', 'n' /* String */
  };
  request.request_data_size = sizeof(data);
  request.data = data;

  size_t string_len = sizeof(data)
                      - sizeof(CipUint); /* Less length field. */

  size_t bytes = DecodeCipString(dst_string, &request, &response);

  CHECK_EQUAL(sizeof(data), bytes);
  CHECK_EQUAL(string_len, dst_string->length);
  MEMCMP_EQUAL(&data[sizeof(CipUint)], dst_string->string, string_len);
  POINTERS_EQUAL(&data[sizeof(data)], request.data);
  CHECK_EQUAL(kCipErrorSuccess, response.general_status);
}


/*******************************************************************************
 * CIP short string decoding tests.
 ******************************************************************************/

CipShortString *short_string;


TEST_GROUP(ShortStringDecode)
{
  void setup() {
    /* Initialize the destination string with junk to be overwritten. */
    short_string = (CipShortString *)CipCalloc( 1, sizeof(CipShortString) );
    SetCipShortStringByCstr(short_string, "spam");
  }

  void teardown() {
    FreeCipShortString(short_string);
  }
};


/* Validate decoding a zero-length string. */
TEST(ShortStringDecode, ZeroLength)
{
  /* Configure the serialized source message. */
  CipOctet data[] = {0};
  request.request_data_size = sizeof(data);
  request.data = data;

  int bytes = DecodeCipShortString(short_string, &request, &response);

  CHECK_EQUAL(sizeof(data), bytes);
  CHECK_EQUAL(0, short_string->length);
  POINTERS_EQUAL(NULL, short_string->string);
  POINTERS_EQUAL(&data[sizeof(data)], request.data);
  CHECK_EQUAL(kCipErrorSuccess, response.general_status);
}


/* Validate decoding a maximum-length string. */
TEST(ShortStringDecode, MaxLength)
{
  /* Allocate and initialize a maximum length serialized string. */
  size_t data_size = sizeof(CipUsint) /* 8-bit length */
                     + UINT8_MAX;     /* String content */
  CipOctet *data = (CipOctet *)malloc(data_size);
  data[0] = UINT8_MAX; /* Set length to maximum. */
  memset(&data[1], 'x', UINT8_MAX); /* Fill the string content. */

  /* Configure the serialized source message. */
  request.request_data_size = data_size;
  request.data = data;

  int bytes = DecodeCipShortString(short_string, &request, &response);

  CHECK_EQUAL(data_size, bytes);
  CHECK_EQUAL(UINT8_MAX, short_string->length);
  MEMCMP_EQUAL(&data[1], short_string->string, UINT8_MAX);
  POINTERS_EQUAL(&data[data_size], request.data);
  CHECK_EQUAL(kCipErrorSuccess, response.general_status);

  free(data);
}


/* Validate overwriting an empty string. */
TEST(ShortStringDecode, OverwriteEmpty)
{
  ClearCipShortString(short_string);

  /* Configure the serialized source message. */
  CipOctet data[] = {
    6,                           /* 8-bit length */
    'f', 'o', 'o', 'b', 'a', 'r' /* String */
  };
  request.request_data_size = sizeof(data);
  request.data = data;

  size_t string_len = sizeof(data)
                      - sizeof(CipUsint); /* Less length field. */

  int bytes = DecodeCipShortString(short_string, &request, &response);

  CHECK_EQUAL(sizeof(data), bytes);
  CHECK_EQUAL(string_len, short_string->length);
  MEMCMP_EQUAL(&data[sizeof(CipUsint)], short_string->string, string_len);
  POINTERS_EQUAL(&data[sizeof(data)], request.data);
  CHECK_EQUAL(kCipErrorSuccess, response.general_status);
}
