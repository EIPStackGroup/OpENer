/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/


#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "encap.h"

#include "ciptypes.h"
#include "enipmessage.h"

}

TEST_GROUP(EncapsulationProtocol) {

};

TEST(EncapsulationProtocol, AnswerListIdentityRequest) {

  CipOctet incoming_message[] =
    "\x63\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xd7\xdd\x00\x00" \
    "\x00\x00\x00\x00\x00\x00\x00\x00";

  CipOctet expected_outgoing_message[] =
    "\x63\x00\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\xd7\xdd\x00\x00" \
    "\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x0c\x00\x2b\x00\x01\x00" \
    "\x00\x02\xaf\x12\xc0\xa8\x38\x65\x00\x00\x00\x00\x00\x00\x00\x00" \
    "\x01\x00\x0c\x00\xe9\xfd\x02\x01\x00\x00\x15\xcd\x5b\x07\x09\x4f" \
    "\x70\x45\x4e\x65\x72\x20\x50\x43\xff";

  EncapsulationData receive_data;
  ENIPMessage outgoing_message;
  InitializeENIPMessage(&outgoing_message);

  CreateEncapsulationStructure(incoming_message,
                               sizeof(incoming_message),
                               &receive_data);

  EncapsulateListIdentityResponseMessage(&receive_data, &outgoing_message);

}
