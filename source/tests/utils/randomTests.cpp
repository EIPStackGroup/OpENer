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
	pRandom = RandomNew(SetXorShiftSeed, NextXorShiftUint32);
	POINTERS_EQUAL(SetXorShiftSeed, pRandom->set_seed);
	POINTERS_EQUAL(NextXorShiftUint32, pRandom->get_next_uint32);
}
