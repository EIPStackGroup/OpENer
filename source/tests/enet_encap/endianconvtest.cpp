/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>

extern "C" {

#include <sys/socket.h>
#include <arpa/inet.h>

#include "endianconv.h"

#include "ciptypes.h"
}

TEST_GROUP(EndianConversion) {

};

TEST(EndianConversion, GetIntFromMessage) {
  CipOctet test_message[] = { 8, 60 };
  CipOctet *message = test_message;
  EipUint16 returned_value = GetIntFromMessage(&message);

  LONGS_EQUAL(15368, returned_value);
  POINTERS_EQUAL(test_message + 2, message);
}

TEST(EndianConversion, GetDintFromMessage) {
  CipOctet test_message[] = { 28, 53, 41, 37 };
  CipOctet *message = test_message;
  EipUint32 returned_value = GetDintFromMessage(&message);

  LONGS_EQUAL(623457564, returned_value);
  POINTERS_EQUAL(test_message + 4, message);
}

TEST(EndianConversion, GetLintFromMessage) {
  CipOctet test_message[] = { 81, 126, 166, 15, 70, 97, 208, 236 };
  CipOctet *message = test_message;
  EipUint64 returned_value = GetLintFromMessage(&message);

  LONGS_EQUAL(5872313548673241324, returned_value);
  POINTERS_EQUAL(test_message + 8, message);
}

TEST(EndianConversion, AddIntToMessage) {
  CipUint value_to_add_to_message = 0x5499;
  CipOctet message[2];
  CipOctet *message_pointer = message;

  AddIntToMessage(value_to_add_to_message, &message_pointer);

  BYTES_EQUAL(0x99, message[0]);
  BYTES_EQUAL(0x54, message[1]);

  POINTERS_EQUAL(message + 2, message_pointer)
}

TEST(EndianConversion, AddDintToMessage) {
  CipUdint value_to_add_to_message = 0x25E0C459;
  CipOctet message[4];
  CipOctet *message_pointer = message;

  AddDintToMessage(value_to_add_to_message, &message_pointer);

  BYTES_EQUAL(0x59, message[0]);
  BYTES_EQUAL(0xC4, message[1]);
  BYTES_EQUAL(0xE0, message[2]);
  BYTES_EQUAL(0x25, message[3]);

  POINTERS_EQUAL(message + 4, message_pointer)
}

TEST(EndianConversion, AddLintToMessage) {
  CipLint value_to_add_to_message = 0x2D2AEF0B84095230;
  CipOctet message[8];
  CipOctet *message_pointer = message;

  AddLintToMessage(value_to_add_to_message, &message_pointer);

  /* Expected message from highest to lowest byte [30][52][09][84][0B][EF][2A][2D] */
  BYTES_EQUAL(0x2D, message[0]);
  BYTES_EQUAL(0x2A, message[1]);
  BYTES_EQUAL(0xEF, message[2]);
  BYTES_EQUAL(0x0B, message[3]);
  BYTES_EQUAL(0x84, message[4]);
  BYTES_EQUAL(0x09, message[5]);
  BYTES_EQUAL(0x52, message[6]);
  BYTES_EQUAL(0x30, message[7]);

  POINTERS_EQUAL(message + 8, message_pointer)
}

TEST(EndianConversion, EncapsulateIpAddress) {
  CipOctet ip_message[8];
  CipOctet *ip_message_ponter = ip_message;

  DetermineEndianess();

  EncapsulateIpAddress(0xAF12, 0x25E0C459, &ip_message_ponter);

  BYTES_EQUAL(AF_INET >> 8, ip_message[0]);
  BYTES_EQUAL(AF_INET, ip_message[1]);
  BYTES_EQUAL(0x12, ip_message[2]);
  BYTES_EQUAL(0xAF, ip_message[3]);
  BYTES_EQUAL(0x59, ip_message[4]);
  BYTES_EQUAL(0xC4, ip_message[5]);
  BYTES_EQUAL(0xE0, ip_message[6]);
  BYTES_EQUAL(0x25, ip_message[7]);
}

TEST(EndianConversion, MoveMessageNOctets) {
  CipOctet message[8];
  CipOctet *message_runner = message;

  MoveMessageNOctetets(&message_runner, 4);

  POINTERS_EQUAL(message + 4, message_runner);
}
