/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "utils/xorshiftrandom.h"

#include <time.h>

void SetXorShiftSeed(Random* const random, uint32_t seed) {
  random->current_seed_value = seed;
}

/** @brief Pseudo-random number algorithm
 * The algorithm used to create the pseudo-random numbers.
 * Works directly on the file global variable
 */
void CalculateNextSeed(Random* const random) {
  if (random->current_seed_value == 0) {
    SetXorShiftSeed(random, time(NULL));
  }

  random->current_seed_value ^= random->current_seed_value << 13;
  random->current_seed_value ^= random->current_seed_value >> 17;
  random->current_seed_value ^= random->current_seed_value << 5;
}

uint32_t NextXorShiftUint32(Random* const random) {
  CalculateNextSeed(random);
  return random->current_seed_value;
}
