/*
 * xorshiftrandom.h
 *
 *  Created on: Dec 1, 2013
 *      Author: mmm
 */

/*!
 * \file xorshiftrandom.h
 *
 * The public interface of the XOR shift pseudo-random number generator
 */
#include <stdint.h>

#ifndef XORSHIFTRANDOM_H_
#define XORSHIFTRANDOM_H_

/*!
 * Sets the initial seed for the XOR shift pseudo-random algorithm
 * \param pa_nSeed The initial seed value
 */
void setXorShiftSeed(uint32_t pa_nSeed);

/*!
 * Returns the next generated pseudo-random number
 * \return The next pseudo-random number
 */
uint32_t nextXorShiftUInt32(void);

#endif /* XORSHIFTRANDOM_H_ */
