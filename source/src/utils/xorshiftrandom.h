/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/**
 * @file xorshiftrandom.h
 *
 * The public interface of the XOR shift pseudo-random number generator
 */
#include <stdint.h>

#include "utils/random.h"

#ifndef SRC_UTILS_XORSHIFTRANDOM_H_
#define SRC_UTILS_XORSHIFTRANDOM_H_

/**
 * @brief Allocate and return a new XOR shift Random instance.
 *
 * The returned object is initialized for use with the XorShift implementation.
 * Caller is responsible for freeing the object using `RandomDelete`.
 *
 * @return Pointer to a newly allocated `Random` on success, or NULL on failure.
 */
Random* XorShiftRandomNew(void);

/**
 * @brief Initialize an existing `Random` object to use the XorShift
 * implementation.
 *
 * This sets the function pointers of `random` to the XorShift functions and
 * leaves the seed value unchanged.
 *
 * @param random Pointer to an already allocated `Random` structure.
 */
void XorShiftRandomInit(Random* const random);

/**
 * @brief Set the initial seed for the XorShift algorithm on the given instance.
 *
 * If seed is zero the XorShift implementation will reseed from the system
 * clock when the next value is requested.
 *
 * @param random Pointer to the `Random` instance to seed.
 * @param seed   The initial seed value.
 */
void XorShiftSetSeed(Random* const random, uint32_t seed);

/**
 * @brief Generate the next pseudo-random 16-bit value using XorShift.
 *
 * @param random Pointer to the `Random` instance to use.
 * @return Next pseudo-random 16-bit value.
 */
uint16_t XorShiftGetNextUInt16(Random* const random);

/**
 * @brief Generate the next pseudo-random 32-bit value using XorShift.
 *
 * @param random Pointer to the `Random` instance to use.
 * @return Next pseudo-random 32-bit value.
 */
uint32_t XorShiftGetNextUInt32(Random* const random);

#endif  // SRC_UTILS_XORSHIFTRANDOM_H_
