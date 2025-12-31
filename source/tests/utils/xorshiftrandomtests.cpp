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

TEST_GROUP(XorShiftRandom){

};

/*Characterization test*/
TEST(XorShiftRandom, SeedOneCharacterization) {
  uint32_t nResult;
  Random* random = RandomNew(SetXorShiftSeed, NextXorShiftUint32);
  random->set_seed(random, 1);
  nResult = random->get_next_uint32(random);
  LONGS_EQUAL(270369, nResult);
  nResult = random->get_next_uint32(random);
  LONGS_EQUAL(67634689, nResult);
  nResult = random->get_next_uint32(random);
  LONGS_EQUAL(2647435461, nResult);
  nResult = random->get_next_uint32(random);
  LONGS_EQUAL(307599695, nResult);
  RandomDelete(&random);
}
