/*
 * randomTests.cpp
 *
 *  Created on: Dec 16, 2013
 *      Author: mmm
 */

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
	Random* pRandom;
	uint32_t nResult = 0;
	pRandom = random_new(setXorShiftSeed, nextXorShiftUInt32);
	POINTERS_EQUAL(setXorShiftSeed, pRandom->setSeed);
	POINTERS_EQUAL(nextXorShiftUInt32, pRandom->getNextUInt32);
}
