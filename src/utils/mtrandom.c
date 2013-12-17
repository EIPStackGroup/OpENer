/*
 * random.c
 *
 *  Created on: Nov 28, 2013
 *      Author: mmm
 */

#include "mtrandom.h"
#include <stddef.h>
#include <typedefs.h>

typedef struct {
	uint16_t nBlockSize;
	uint32_t* panSeedBlocks;
} MTRandomParams;

static MTRandomParams fg_sRandomParams = {0, 0};

void mtSetupRandomParams(uint16_t pa_nBlockSize)  {
	fg_sRandomParams.nBlockSize = pa_nBlockSize;
	fg_sRandomParams.panSeedBlocks = malloc(sizeof(uint32_t) * fg_sRandomParams.nBlockSize);
}

int mtIsInitialized(void)
{
	if( NULL != fg_sRandomParams.panSeedBlocks)
		return true;
	else return false;
}

void mtSetSeed(uint32_t pa_nStartSeed)  {

}
