/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "cipconnectionmanager.h"

}

TEST_GROUP(CipConnectionManager) {

};

TEST(CipConnectionManager, GetForwardOpenFixedVarFlag) {
  ConnectionObject connection_object;
  connection_object.t_to_o_network_connection_parameter = 0x2222;
  ConnectionObjectFixedVariable fixed_variable =
    GetConnectionObjectTargetToOriginatorFixedOrVariableConnectionSize(
      &connection_object);
  CHECK_EQUAL(kConnectionObjectVariableConnectionSize, fixed_variable);
}
