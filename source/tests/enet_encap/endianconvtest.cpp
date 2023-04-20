/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include <sys/socket.h>
#include <arpa/inet.h>

#include "endianconv.h"

#include "ciptypes.h"

}

TEST_GROUP (EndianConversion) {

};

TEST(EndianConversion, GetSintFromMessage) {
  const CipOctet test_message[] = {8};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipSint returned_value = GetSintFromMessage(message);

  LONGS_EQUAL(8, returned_value);
  POINTERS_EQUAL(test_message + 1, *message);
}

TEST(EndianConversion, GetUsintFromMessage) {
  const CipOctet test_message[] = {8};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipUsint returned_value = GetUsintFromMessage(message);

  LONGS_EQUAL(8, returned_value);
  POINTERS_EQUAL(test_message + 1, *message);
}

TEST(EndianConversion, GetByteFromMessage) {
  const CipOctet test_message[] = {8};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipByte returned_value = GetByteFromMessage(message);

  LONGS_EQUAL(8, returned_value);
  POINTERS_EQUAL(test_message + 1, *message);
}

TEST(EndianConversion, GetIntFromMessage) {
  const CipOctet test_message[] = {8, 60};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipInt returned_value = GetIntFromMessage(message);

  LONGS_EQUAL(15368, returned_value);
  POINTERS_EQUAL(test_message + 2, *message);
}

TEST(EndianConversion, GetUintFromMessage) {
  const CipOctet test_message[] = {8, 60};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipUint returned_value = GetUintFromMessage(message);

  LONGS_EQUAL(15368, returned_value);
  POINTERS_EQUAL(test_message + 2, *message);
}

TEST(EndianConversion, GetWordFromMessage) {
  const CipOctet test_message[] = {8, 60};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipWord returned_value = GetWordFromMessage(message);

  LONGS_EQUAL(15368, returned_value);
  POINTERS_EQUAL(test_message + 2, *message);
}

TEST(EndianConversion, GetDintFromMessage) {
  const CipOctet test_message[] = {28, 53, 41, 37};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipDint returned_value = GetDintFromMessage(message);

  LONGS_EQUAL(623457564, returned_value);
  POINTERS_EQUAL(test_message + 4, *message);
}

TEST(EndianConversion, GetUdintFromMessage) {
  const CipOctet test_message[] = {28, 53, 41, 37};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipUdint returned_value = GetUdintFromMessage(message);

  LONGS_EQUAL(623457564, returned_value);
  POINTERS_EQUAL(test_message + 4, *message);
}

TEST(EndianConversion, GetDwordFromMessage) {
  const CipOctet test_message[] = {28, 53, 41, 37};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  CipDword returned_value = GetDwordFromMessage(message);

  LONGS_EQUAL(623457564, returned_value);
  POINTERS_EQUAL(test_message + 4, *message);
}

TEST(EndianConversion, GetLintFromMessage) {
  const CipOctet test_message[] = {81, 126, 166, 15, 70, 97, 208, 236};
  const CipOctet *message_pointer = test_message;
  const CipOctet **const message = &message_pointer;
  EipUint64 returned_value = GetLintFromMessage(message);

  LONGS_EQUAL(5872313548673241324, returned_value);
  POINTERS_EQUAL(test_message + 8, *message);
}

TEST(EndianConversion, AddIntToMessage) {
  CipUint value_to_add_to_message = 0x5499;
  ENIPMessage message;
  InitializeENIPMessage(&message);

  AddIntToMessage(value_to_add_to_message, &message);

  BYTES_EQUAL(0x99, message.message_buffer[0]);
  BYTES_EQUAL(0x54, message.message_buffer[1]);

  POINTERS_EQUAL(message.message_buffer + 2, message.current_message_position);
}

TEST(EndianConversion, AddDintToMessage) {
  CipUdint value_to_add_to_message = 0x25E0C459;
  ENIPMessage message;
  InitializeENIPMessage(&message);

  AddDintToMessage(value_to_add_to_message, &message);

  BYTES_EQUAL(0x59, message.message_buffer[0]);
  BYTES_EQUAL(0xC4, message.message_buffer[1]);
  BYTES_EQUAL(0xE0, message.message_buffer[2]);
  BYTES_EQUAL(0x25, message.message_buffer[3]);

  POINTERS_EQUAL(message.message_buffer + 4, message.current_message_position);
}

TEST(EndianConversion, AddLintToMessage) {
  CipLint value_to_add_to_message = 0x2D2AEF0B84095230;
  ENIPMessage message;
  InitializeENIPMessage(&message);

  AddLintToMessage(value_to_add_to_message, &message);

  /* Expected message from highest to lowest byte [30][52][09][84][0B][EF][2A][2D] */
  BYTES_EQUAL(0x30, message.message_buffer[0]);
  BYTES_EQUAL(0x52, message.message_buffer[1]);
  BYTES_EQUAL(0x09, message.message_buffer[2]);
  BYTES_EQUAL(0x84, message.message_buffer[3]);
  BYTES_EQUAL(0x0B, message.message_buffer[4]);
  BYTES_EQUAL(0xEF, message.message_buffer[5]);
  BYTES_EQUAL(0x2A, message.message_buffer[6]);
  BYTES_EQUAL(0x2D, message.message_buffer[7]);

  POINTERS_EQUAL(message.message_buffer + 8, message.current_message_position);
}

TEST(EndianConversion, EncapsulateIpAddress) {
  ENIPMessage message;
  InitializeENIPMessage(&message);

  DetermineEndianess();

  EncapsulateIpAddress(0xAF12, 0x25E0C459, &message);

  BYTES_EQUAL(AF_INET >> 8, message.message_buffer[0]);
  BYTES_EQUAL(AF_INET, message.message_buffer[1]);
  BYTES_EQUAL(0x12, message.message_buffer[2]);
  BYTES_EQUAL(0xAF, message.message_buffer[3]);
  BYTES_EQUAL(0x59, message.message_buffer[4]);
  BYTES_EQUAL(0xC4, message.message_buffer[5]);
  BYTES_EQUAL(0xE0, message.message_buffer[6]);
  BYTES_EQUAL(0x25, message.message_buffer[7]);

  POINTERS_EQUAL(message.message_buffer + 8, message.current_message_position);
}

TEST(EndianConversion, MoveMessageNOctets) {
  ENIPMessage message;
  InitializeENIPMessage(&message);

  MoveMessageNOctets(4, &message);

  POINTERS_EQUAL(message.message_buffer + 4, message.current_message_position);
}

TEST(EndianConversion, FillNextNMEssageOctetsWith) {
  ENIPMessage message;
  InitializeENIPMessage(&message);
  memset(message.message_buffer, 15, 8);

  FillNextNMessageOctetsWith(0, 8, &message);
  BYTES_EQUAL(0, message.message_buffer[0]);
  BYTES_EQUAL(0, message.message_buffer[1]);
  BYTES_EQUAL(0, message.message_buffer[2]);
  BYTES_EQUAL(0, message.message_buffer[3]);
  BYTES_EQUAL(0, message.message_buffer[4]);
  BYTES_EQUAL(0, message.message_buffer[5]);
  BYTES_EQUAL(0, message.message_buffer[6]);
  BYTES_EQUAL(0, message.message_buffer[7]);
  POINTERS_EQUAL(message.message_buffer, message.current_message_position);

}
