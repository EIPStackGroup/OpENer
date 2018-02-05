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

TEST_GROUP(EndianConversion) {

};

TEST(EndianConversion, GetSintFromMessage) {
  const EipUint8 test_message[] = { 8 };
  const EipUint8 *message_pointer = test_message;
  const EipUint8 **const message = &message_pointer;
  EipUint16 returned_value = GetSintFromMessage(message);

  LONGS_EQUAL(8, returned_value);
  POINTERS_EQUAL(test_message + 1, *message);
}

TEST(EndianConversion, GetIntFromMessage) {
  const EipUint8 test_message[] = { 8, 60 };
  const EipUint8 *message_pointer = test_message;
  const EipUint8 **const message = &message_pointer;
  EipUint16 returned_value = GetIntFromMessage(message);

  LONGS_EQUAL(15368, returned_value);
  POINTERS_EQUAL(test_message + 2, *message);
}

TEST(EndianConversion, GetDintFromMessage) {
  const EipUint8 test_message[] = { 28, 53, 41, 37 };
  const EipUint8 *message_pointer = test_message;
  const EipUint8 **const message = &message_pointer;
  EipUint32 returned_value = GetDintFromMessage(message);

  LONGS_EQUAL(623457564, returned_value);
  POINTERS_EQUAL(test_message + 4, *message);
}

TEST(EndianConversion, GetLintFromMessage) {
  const EipUint8 test_message[] = { 81, 126, 166, 15, 70, 97, 208, 236 };
  const EipUint8 *message_pointer = test_message;
  const EipUint8 **const message = &message_pointer;
  EipUint64 returned_value = GetLintFromMessage(message);

  LONGS_EQUAL(5872313548673241324, returned_value);
  POINTERS_EQUAL(test_message + 8, *message);
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
  const CipOctet message[] = "This is a test message";
  const CipOctet *message_runner = message;

  MoveMessageNOctets(4, &message_runner);

  POINTERS_EQUAL(message + 4, message_runner);
}

TEST(EndianConversion, FillNextNMEssageOctetsWith) {
  CipOctet message[8];
  CipOctet *message_runner = message;
  memset(message, 15, 8);

  FillNextNMessageOctetsWith(0, 8, &message_runner);
  BYTES_EQUAL(0, message_runner[0]);
  BYTES_EQUAL(0, message_runner[1]);
  BYTES_EQUAL(0, message_runner[2]);
  BYTES_EQUAL(0, message_runner[3]);
  BYTES_EQUAL(0, message_runner[4]);
  BYTES_EQUAL(0, message_runner[5]);
  BYTES_EQUAL(0, message_runner[6]);
  BYTES_EQUAL(0, message_runner[7]);

}
