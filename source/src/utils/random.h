/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_RANDOM_H_
#define OPENER_RANDOM_H_

#include <stdint.h>

typedef void (*SetSeed)(uint32_t seed);
typedef uint32_t (*GetNextUInt32)(void);

typedef struct {
  uint32_t current_seed_value; /**< Holds the current seed/random value */
  SetSeed set_seed; /**< Function pointer to SetSeed function */
  GetNextUInt32 get_next_uint32; /**< Function pointer to GetNextUInt32 function */
} Random;

Random *RandomNew(SetSeed,
                  GetNextUInt32);

void RandomDelete(Random **random);

#endif /* OPENER_RANDOM_H_ */
