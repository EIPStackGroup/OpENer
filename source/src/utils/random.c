/*
 * random.cpp
 *
 *  Created on: Dec 16, 2013
 *      Author: mmm
 */

#include "random.h"

Random* random_new(setSeedfn pa_fnpSetSeed, getNextUInt32fn pa_fnGetNextUInt32)
{
	Random* out = malloc(sizeof(Random));
	*out = (Random){.setSeed = pa_fnpSetSeed, .getNextUInt32 = pa_fnGetNextUInt32};
	return out;
}
