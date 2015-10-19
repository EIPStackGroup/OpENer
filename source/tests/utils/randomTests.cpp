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
	pRandom = random_new(SetXorShiftSeed, NextXorShiftUint32);
	POINTERS_EQUAL(SetXorShiftSeed, pRandom->setSeed);
	POINTERS_EQUAL(NextXorShiftUint32, pRandom->getNextUInt32);
}
