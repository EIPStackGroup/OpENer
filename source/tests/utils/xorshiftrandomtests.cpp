/*******************************************************************************
 * Copyright (c) 2015 - 2025, Rockwell Automation, Inc., Martin Melik Merkumians
 * All rights reserved.
 *
 * Martin Melik Merkumians < Initial implementation >
 * Martin Melik Merkumians < Updates for reentrant design >
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
  Random* random = XorShiftRandomNew();
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
