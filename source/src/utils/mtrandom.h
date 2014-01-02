/*
 * random
 *
 *  Created on: Nov 28, 2013
 *      Author: mmm
 */

#include <stdint.h>

#ifndef RANDOM_
#define RANDOM_

void mtSetupRandomParams(uint16_t pa_nBlockSize);

void mtSetSeed(uint32_t pa_nStartSeed);


#endif /* RANDOM_ */
