/*
 * random.h
 *
 *  Created on: Dec 1, 2013
 *      Author: mmm
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#include <stdint.h>

typedef void (*setSeedfn)(uint32_t pa_nSeed);
typedef uint32_t (*getNextUInt32fn)(void);

typedef struct
{
	uint32_t nXorShiftSeed; /**< Holds the current seed/random value */
	setSeedfn setSeed; /**< Function pointer to setSeed function */
	getNextUInt32fn getNextUInt32; /**< Function pointer to getNextUInt32 function */
} Random;

Random* random_new(setSeedfn, getNextUInt32fn);

#endif /* RANDOM_H_ */
