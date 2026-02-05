/*******************************************************************************
 * Copyright (c) 2015, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>

extern "C" {
#include "utils/random.h"
#include "utils/xorshiftrandom.h"
}

TEST_GROUP(RandomClass){

};

TEST(RandomClass, CreateXOrShiftObject) {
  Random* random = NULL;
  random =
    RandomNew(XorShiftSetSeed, XorShiftGetNextUInt16, XorShiftGetNextUInt32);
  POINTERS_EQUAL(XorShiftSetSeed, random->set_seed);
  POINTERS_EQUAL(XorShiftGetNextUInt16, random->get_next_uint16);
  POINTERS_EQUAL(XorShiftGetNextUInt32, random->get_next_uint32);
  RandomDelete(&random);
}
