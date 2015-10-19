/*
 * xorshiftrandomtests.c
 *
 *  Created on: Dec 1, 2013
 *      Author: mmm
 */

#include <CppUTest/TestHarness.h>
#include <stdint.h>

extern "C"  {
#include <xorshiftrandom.h>
}

TEST_GROUP(XorShiftRandom)
{

};

/*This test should always return 0 as the next random number (see XorShift algorithm*/
TEST(XorShiftRandom, SeedZeroInitResult)
{
	uint32_t nResult;
	nResult = 1;
	SetXorShiftSeed(0);
	nResult = NextXorShiftUint32();
	LONGS_EQUAL(0, nResult);
}

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
