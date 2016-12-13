/*******************************************************************************
 * Copyright (c) 2015, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/


#include <CppUTest/TestHarness.h>
#include <stdint.h>

extern "C"  {
#include <random.h>
#include <xorshiftrandom.h>
}

TEST_GROUP(RandomClass)
{

};

TEST(RandomClass, CreateXOrShiftObject)
{
  Random *pRandom = NULL;
  uint32_t nResult = 0;
  pRandom = RandomNew(SetXorShiftSeed, NextXorShiftUint32);
  POINTERS_EQUAL(SetXorShiftSeed, pRandom->set_seed);
  POINTERS_EQUAL(NextXorShiftUint32, pRandom->get_next_uint32);
  RandomDelete(&pRandom);
}
