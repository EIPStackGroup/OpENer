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

#ifndef OPENER_XORSHIFTRANDOM_H_
#define OPENER_XORSHIFTRANDOM_H_

/**
 * @brief Sets the initial seed for the XOR shift pseudo-random algorithm
 * @param seed The initial seed value
 */
void SetXorShiftSeed(uint32_t seed);

/**
 * @brief Returns the next generated pseudo-random number
 * @return The next pseudo-random number
 */
uint32_t NextXorShiftUint32(void);

#endif /* OPENER__XORSHIFTRANDOM_H_ */
