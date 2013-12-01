/*
 * xorshiftrandom.c
 *
 *  Created on: Nov 28, 2013
 *      Author: mmm
 */

#include "xorshiftrandom.h"

static uint32_t nXorShiftSeed; /*! < File-global variable holding the current seed*/

void setXorShiftSeed(uint32_t pa_nSeed)
{
	nXorShiftSeed = pa_nSeed;
}

/*!
 * The algorithm used to create the pseudo-random numbers.
 * Works directly on the file global variable
 */
void calculateNextSeed(void)
{
	nXorShiftSeed ^= nXorShiftSeed << 13;
	nXorShiftSeed ^= nXorShiftSeed >> 17;
	nXorShiftSeed ^= nXorShiftSeed << 5;
}

uint32_t nextXorShiftUInt32(void)
{
	calculateNextSeed();
	return nXorShiftSeed;
}
