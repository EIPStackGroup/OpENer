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

  ENIPMessage outgoing_message;
  InitializeENIPMessage(&outgoing_message);

  EncapsulationData receive_data;
  CreateEncapsulationStructure(incoming_message,
                               sizeof(incoming_message),
                               &receive_data);

  EncapsulateListIdentityResponseMessage(&receive_data, &outgoing_message);

}

TEST(EncapsulationProtocol, AnswerListServicesRequest) {
  CipOctet incoming_message[] =
    "\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe0\xdd\x00\x00" \
    "\x00\x00\x00\x00\x00\x00\x00\x00";

  CipOctet expected_outgoing_message[] =
    "\x04\x00\x1a\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe0\xdd\x00\x00" \
    "\x00\x00\x00\x00\x00\x00\x00\x00";

  ENIPMessage outgoing_message;
  InitializeENIPMessage(&outgoing_message);

  EncapsulationData recieved_data = {0};
  CreateEncapsulationStructure(incoming_message,
                               sizeof(incoming_message),
                               &recieved_data);

  HandleReceivedListServicesCommand(&recieved_data, &outgoing_message);

}

TEST(EncapsulationProtocol, AnswerListInterfacesRequest) {
  CipOctet incoming_message[] = "";

  CipOctet expected_outgoing_message[] = "";

  ENIPMessage outgoing_message;
  InitializeENIPMessage(&outgoing_message);

  EncapsulationData received_data = {0};
  CreateEncapsulationStructure(incoming_message,
                               sizeof(incoming_message),
                               &received_data);

  HandleReceivedListInterfacesCommand(&received_data, &outgoing_message);
}

TEST(EncapsulationProtocol, AnswerRegisterSessionRequestWrongProtocolVersion) {

  CipOctet incoming_message[] =
    "\x65\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x67\x88\x00\x00" \
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

  CipOctet expected_outgoing_message[] = "";

  ENIPMessage outgoing_message;
  InitializeENIPMessage(&outgoing_message);

  EncapsulationData received_data = {0};
  CreateEncapsulationStructure(incoming_message,
                               sizeof(incoming_message),
                               &received_data);

  HandleReceivedRegisterSessionCommand(0, &received_data, &outgoing_message);

}

TEST(EncapsulationProtocol, SendRRData) {
  CipOctet incoming_message[] =
    "\x6f\x00\x0c\x00\x01\x00\x00\x00\x00\x00\x00\x00\xf0\xdd\x00\x00" \
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00" \
    "\x01\x00\x00\x00";

  CipOctet expected_outgoing_message[] = "";

  ENIPMessage outgoing_message = {0};
  InitializeENIPMessage(&outgoing_message);

  EncapsulationData received_data = {0};
  CreateEncapsulationStructure(incoming_message,
                               sizeof(incoming_message),
                               &received_data);

  struct sockaddr_in fake_originator = {0};
  struct sockaddr *fake_originator_pointer =
    (struct sockaddr *)&fake_originator;

  HandleReceivedSendRequestResponseDataCommand(&received_data,
                                               fake_originator_pointer,
                                               &outgoing_message);
}
