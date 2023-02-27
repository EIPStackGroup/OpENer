/*******************************************************************************
 * Copyright (c) 2015, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>

extern "C"  {
#include <xorshiftrandom.h>
}

TEST_GROUP(XorShiftRandom)
{

};

/*Characterization test*/
TEST(XorShiftRandom, SeedOneCharacterization)
{
  uint32_t nResult;
  SetXorShiftSeed(1);
  nResult = NextXorShiftUint32();
  LONGS_EQUAL(270369, nResult);
  nResult = NextXorShiftUint32();
  LONGS_EQUAL(67634689, nResult);
  nResult = NextXorShiftUint32();
  LONGS_EQUAL(2647435461, nResult);
  nResult = NextXorShiftUint32();
  LONGS_EQUAL(307599695, nResult);
}
