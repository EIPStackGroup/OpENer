/*******************************************************************************
 * Copyright (c) 2020, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "cipepath.h"
#include "cipelectronickey.h"

}

TEST_GROUP (CipElectronicKey) {

};

TEST(CipElectronicKey, SetKeyFormat) {
  CipElectronicKey key;
  ElectronicKeySetKeyFormat(&key, 4);
  CHECK_EQUAL(4, key.key_format);

}

TEST(CipElectronicKey, GetKeyFormat) {
  CipElectronicKey key = {.key_format = 4, .key_data = NULL};
  CHECK_EQUAL(4, ElectronicKeyGetKeyFormat(&key) );
}

TEST(CipElectronicKey, SetKeyData) {
  char dummyFormatData[] = {0,1,2,3,4,5};
  CipElectronicKey key;
  ElectronicKeySetKeyData(&key, dummyFormatData);
  POINTERS_EQUAL(dummyFormatData, key.key_data);
}

TEST(CipElectronicKey, GetKeyData) {
  char dummyFormatData[] = {0,1,2,3,4,5};
  CipElectronicKey key = {.key_format = 0, .key_data = dummyFormatData};
  POINTERS_EQUAL(dummyFormatData, ElectronicKeyGetKeyData(&key) );
}
