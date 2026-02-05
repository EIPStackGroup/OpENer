/*******************************************************************************
 * Copyright (c) 2017 - 2025, Rockwell Automation, Inc., Martin Melik Merkumians
 * All rights reserved.
 *
 * Martin Melik Merkumians < Initial implementation >
 * Martin Melik Merkumians < Updates for reentrant design >
 ******************************************************************************/

#include "utils/xorshiftrandom.h"

#include <time.h>

Random* XorShiftRandomNew(void) {
  return RandomNew(
    XorShiftSetSeed, XorShiftGetNextUInt16, XorShiftGetNextUInt32);
}

void XorShiftRandomInit(Random* const random) {
  RandomInit(
    random, XorShiftSetSeed, XorShiftGetNextUInt16, XorShiftGetNextUInt32);
}

void XorShiftSetSeed(Random* const random, uint32_t seed) {
  random->current_seed_value = seed;
}

/** @brief Pseudo-random number algorithm
 * The algorithm used to create the pseudo-random numbers.
 * Works directly on the file global variable
 */
void XorShiftCalculateNextSeed(Random* const random) {
  if (random->current_seed_value == 0) {
    XorShiftSetSeed(random, (uint32_t)time(NULL));
  }

  random->current_seed_value ^= random->current_seed_value << 13;
  random->current_seed_value ^= random->current_seed_value >> 17;
  random->current_seed_value ^= random->current_seed_value << 5;
}

uint16_t XorShiftGetNextUInt16(Random* const random) {
  XorShiftCalculateNextSeed(random);
  // Return the higher 16 bits, as they have better randomness properties
  return (uint16_t)(random->current_seed_value >> 16);
}

uint32_t XorShiftGetNextUInt32(Random* const random) {
  XorShiftCalculateNextSeed(random);
  return random->current_seed_value;
}
