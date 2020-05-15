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
