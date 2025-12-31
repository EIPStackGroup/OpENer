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
  random         = RandomNew(SetXorShiftSeed, NextXorShiftUint32);
  POINTERS_EQUAL(SetXorShiftSeed, random->set_seed);
  POINTERS_EQUAL(NextXorShiftUint32, random->get_next_uint32);
  RandomDelete(&random);
}
