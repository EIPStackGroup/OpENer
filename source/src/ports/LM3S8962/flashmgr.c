/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "hw_memmap.h"
#include "hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "lwiplib.h"

/*
 * This flash manager is rudimentary. It does wear leveling by allocating small chunks at a time.
 * When a new parameter block is to be written the old one is erased by writing zeros to it. When
 * there is no more space in the 1K chunk for a new parameter block the entire chunk is erased. 
 *
 * assumptions:
 *   -- erased FLASH is all ones
 *   -- bits in individual words can be written to zero
 *   -- a word that is part ones and part zeros can be written to all zeros
 *   -- to get any bits changed back to ones the entire chunk has to be erased
 * 
 * A parameter block starts with a one word size field. The size indicates the size of the following data.
 * If the size field is zero, the block is erased. 
 * 
 * A chunk of flash can be in one of three states:
 *   -- erased (all ones)
 *   -- data (starts with a size word that is not 0 or 0xffffffff
 *   -- zeroed (all zeros)
 * 
 */

// This flash manger only supports one parameter block at a time.
// It does not support multiple block types, or multiple instances of blocks.


#define FLASHBASE ((unsigned long *)(255*1024))			// parameter sector is the last 1024 byte chunk of the program flash
#define FLASHEND ((unsigned long *)(256*1024))
#define CHUNKSIZE 1024
static const int erasedFlashField = 0xffffffff;

// erase everything in the parameters block
static void eraseAllParameters(void)
{
	FlashErase((unsigned long)FLASHBASE);
}

// find the parameter block
// returns the address of the parameter block, or zero if not found

unsigned long* findNextEmptyParameterBlock(void)
{
	unsigned long* currentFlashAddress = FLASHBASE;
	unsigned long* const noSpaceLeft = 0;

	while (currentFlashAddress < FLASHEND)
	{
		if (0 == *currentFlashAddress)
			++currentFlashAddress;
		else if (erasedFlashField == *currentFlashAddress)
			return noSpaceLeft;
		else
			return currentFlashAddress;
	}
	return noSpaceLeft;
}

// verify that the block to be written is erased
static int parameterBlockIsEmpty(unsigned long *data, // pointer to are to be checked
		int dataSize) // sizeof(struct)
{
	int const isNotEmpty = 0;
	int const isEmpty = 1;
	while (dataSize > 0)
	{
		if (*data++ != erasedFlashField)
			return isNotEmpty;
		dataSize -= 4;
	}
	return isEmpty;
}

// write a new parameter block
void writeParameter(unsigned long *data, // pointer to struct to be saved
		int dataSize) // sizeof(struct)
{
	unsigned long *currentParameterBlock = findNextEmptyParameterBlock(); // find the current parameter block;
	unsigned long zero = 0;

	if (currentParameterBlock) // if found, zero it
	{
		int lengthToBeErased = *currentParameterBlock + 4; // calc dataSize of region to be erased

		if (currentParameterBlock + lengthToBeErased / 4 <= FLASHEND) // if properly formatted, zero the current block
		{
			while (lengthToBeErased > 0) //   for each word
			{
				FlashProgram(&zero, (unsigned long)currentParameterBlock, 4); // zero one word
				currentParameterBlock++; //     advance to next word
				lengthToBeErased -= 4; //     dec dataSize
			}
		}
		else // else flash is corrupt -- erase it all
		{
			eraseAllParameters();
			currentParameterBlock = FLASHBASE;
		}
	}

	// at this point p should point to useable erased flash

	// currentParameterBlock now points to the first unerased word

	if (currentParameterBlock + dataSize/4 > FLASHEND // if flash is full
			|| !parameterBlockIsEmpty(currentParameterBlock, dataSize)) // or not properly erased
	{
		eraseAllParameters(); // erase the entire block
		currentParameterBlock = FLASHBASE; // start from the beginning
	}

	FlashProgram((unsigned long *)&dataSize, (unsigned long)currentParameterBlock, 4); // program the length
	FlashProgram(data, (unsigned long)(currentParameterBlock+1), dataSize); // program the data
}

