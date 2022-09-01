/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <time.h>
#include "xorshiftrandom.h"

static uint32_t xor_shift_seed; /** < File-global variable holding the current seed*/

void SetXorShiftSeed(uint32_t seed) {
  xor_shift_seed = seed;
}

/** @brief Pseudo-random number algorithm
 * The algorithm used to create the pseudo-random numbers.
 * Works directly on the file global variable
 */
void CalculateNextSeed(void) {
  if (xor_shift_seed == 0)
    SetXorShiftSeed(time(NULL));

  xor_shift_seed ^= xor_shift_seed << 13;
  xor_shift_seed ^= xor_shift_seed >> 17;
  xor_shift_seed ^= xor_shift_seed << 5;
}

uint32_t NextXorShiftUint32(void) {
  CalculateNextSeed();
  return xor_shift_seed;
}
