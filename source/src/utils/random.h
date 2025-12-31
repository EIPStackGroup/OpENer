/*******************************************************************************
 * Copyright (c) 2017 - 2025 Rockwell Automation, Inc., Martin Melik Merkumians
 * All rights reserved.
 *
 * Martin Melik Merkumians < Initially created >
 * Martin Melik Merkumians < Updates to use Random struct with function pointers
 * >
 *
 ******************************************************************************/

#ifndef UTILS_RANDOM_H_
#define UTILS_RANDOM_H_

#include <stdint.h>

typedef struct Random Random;

typedef void (*SetSeed)(Random* const random, uint32_t seed);
typedef uint32_t (*GetNextUInt32)(Random* const random);

typedef struct Random {
  uint32_t current_seed_value;  ///< Holds the current seed/random value
  SetSeed set_seed;             ///< Function pointer to SetSeed function
  GetNextUInt32
    get_next_uint32;  ///< Function pointer to GetNextUInt32 function
} Random;

Random* RandomNew(SetSeed set_seed_function,
                  GetNextUInt32 get_next_uint32_function);

void RandomDelete(Random** random);

#endif /* UTILS_RANDOM_H_ */
