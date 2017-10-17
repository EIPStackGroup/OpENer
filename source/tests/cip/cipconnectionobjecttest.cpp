/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "cipconnectionobject.h"

}

TEST_GROUP(CipConnectionObject) {

};

TEST(CipConnectionObject, StateNonExistent) {
	CipConnectionObject connection_object = { 0 };
	connection_object.state = 0;
	ConnectionObjectState state = GetConnectionObjectState(&connection_object);
	CHECK_EQUAL(kConnectionObjectStateNonExistent, state);
}

TEST(CipConnectionObject, StateConfiguring) {
	CipConnectionObject connection_object = { 0 };
	connection_object.state = 1;
	ConnectionObjectState state = GetConnectionObjectState(&connection_object);
	CHECK_EQUAL(kConnectionObjectStateConfiguring, state);
}
TEST(CipConnectionObject, StateWaitingForConnectionID) {
	CipConnectionObject connection_object = { 0 };
	connection_object.state = 2;
	ConnectionObjectState state = GetConnectionObjectState(&connection_object);
	CHECK_EQUAL(kConnectionObjectStateWaitingForConnectionID, state);
}
TEST(CipConnectionObject, StateEstablished) {
	CipConnectionObject connection_object = { 0 };
	connection_object.state = 3;
	ConnectionObjectState state = GetConnectionObjectState(&connection_object);
	CHECK_EQUAL(kConnectionObjectStateEstablished, state);
}
TEST(CipConnectionObject, StateTimedOut) {
	CipConnectionObject connection_object = { 0 };
	connection_object.state = 4;
	ConnectionObjectState state = GetConnectionObjectState(&connection_object);
	CHECK_EQUAL(kConnectionObjectStateTimedOut, state);
}
TEST(CipConnectionObject, StateDeferredDelete) {
	CipConnectionObject connection_object = { 0 };
	connection_object.state = 5;
	ConnectionObjectState state = GetConnectionObjectState(&connection_object);
	CHECK_EQUAL(kConnectionObjectStateDeferredDelete, state);
}
TEST(CipConnectionObject, StateClosing) {
	CipConnectionObject connection_object = { 0 };
	connection_object.state = 6;
	ConnectionObjectState state = GetConnectionObjectState(&connection_object);
	CHECK_EQUAL(kConnectionObjectStateClosing, state);
}

