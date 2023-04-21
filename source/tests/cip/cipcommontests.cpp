/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include "CppUTestExt/MockSupport.h"
#include <stdint.h>
#include <string.h>

extern "C" {

#include "opener_api.h"
#include "cipstring.h"

}

ENIPMessage message; /**< Test variable holds ENIP message*/

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
  CHECK_EQUAL( value, *(CipWord *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 2, message.current_message_position);
}

TEST(CipCommon, EncodeCipDword) {
  const CipDword value = 5357678U;
  EncodeCipDword(&value, &message);
  CHECK_EQUAL( value, *(CipDword *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 4, message.current_message_position);
}

TEST(CipCommon, EncodeCipLword) {
  const CipLword value = 8353457678U;
  EncodeCipLword(&value, &message);
  CHECK_EQUAL( value, *(CipLword *)(message.message_buffer) );
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
  CHECK_EQUAL( value, *(CipUint *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 2, message.current_message_position);
}

TEST(CipCommon, EncodeCipUdint) {
  const CipUdint value = 1653245U;
  EncodeCipUdint(&value, &message);
  CHECK_EQUAL( value, *(CipUdint *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 4, message.current_message_position);
}

TEST(CipCommon, EncodeCipUlint) {
  const CipUlint value = 5357678U;
  EncodeCipUlint(&value, &message);
  CHECK_EQUAL( value, *(CipUlint *)(message.message_buffer) );
  POINTERS_EQUAL(message.message_buffer + 8, message.current_message_position);
}

TEST (CipCommon, DecodeCipString) {
  CipMessageRouterRequest request;
  size_t number_of_strings = 4;
  size_t length_of_string[] = {8,4,0,2};
  size_t pos_in_data = 0;
  const CipOctet data[] =
    "\x08\x00\x4F\x44\x56\x41\x5F\x44\x55\x54\x04\x00\x4F\x44\x56\x41\x00\x00\x02\x00\x43\x41";                       // hex data
  request.data = data;
  request.request_data_size = sizeof(data) - 1;

  CipMessageRouterResponse response;

  CipString string = {};

  for(size_t i = 0; i < number_of_strings; i++) {
    if(0 != length_of_string[i]) {
      mock().expectOneCall("CipCalloc");
      mock().expectOneCall("CipFree");
    }
    DecodeCipString(&string, &request, &response);

    // check string + length
    CHECK_EQUAL(*(data + pos_in_data),  string.length); // first element at current pos contains the length of the following string
    MEMCMP_EQUAL(data + pos_in_data + 2, string.string, string.length ); // pos_in_data + 2 bytes for length

    pos_in_data += length_of_string[i] + 2;
  }

  ClearCipString(&string);
}

TEST (CipCommon, DecodeCipShortString) {
  CipMessageRouterRequest request;
  size_t number_of_strings = 4;
  size_t length_of_string[] = {8,4,0,2};
  size_t pos_in_data = 0;
  const CipOctet data[] =
    "\x08\x4F\x44\x56\x41\x5F\x44\x55\x54\x04\x4F\x44\x56\x41\x00\x02\x43\x41";                       // hex data
  request.data = data;
  request.request_data_size = sizeof(data) - 1;

  CipMessageRouterResponse response;

  CipShortString short_string = {};

  for(size_t i = 0; i < number_of_strings; i++) {
    if(0 != length_of_string[i]) {
      mock().expectOneCall("CipCalloc");
      mock().expectOneCall("CipFree");
    }
    DecodeCipShortString(&short_string, &request, &response);

    // check string + length
    CHECK_EQUAL(*(data + pos_in_data),  short_string.length); // first element at current pos contains the length of the following string
    MEMCMP_EQUAL(data + pos_in_data + 1,
                 short_string.string,
                 short_string.length );                                          // pos_in_data + 1 byte for length

    pos_in_data += length_of_string[i] + 1;
  }

  ClearCipShortString(&short_string);
}

