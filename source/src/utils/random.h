/*******************************************************************************
 * Copyright (c) 2017 - 2025 Rockwell Automation, Inc., Martin Melik Merkumians
 * All rights reserved.
 *
 * Martin Melik Merkumians < Initially created >
 * Martin Melik Merkumians < Updates to use Random struct with function pointers
 * >
 *
 ******************************************************************************/

#ifndef SRC_UTILS_RANDOM_H_
#define SRC_UTILS_RANDOM_H_

#include <stdint.h>

typedef struct Random Random;

/// @brief Function pointer type for setting the seed of the random number
/// generator
typedef void (*SetSeed)(Random* const random, uint32_t seed);

/// @brief Function pointer type for getting the next uint16 random number
typedef uint16_t (*GetNextUInt16)(Random* const random);

/// @brief Function pointer type for getting the next uint32 random number
typedef uint32_t (*GetNextUInt32)(Random* const random);

/// @brief The Random struct holding the state of the random number generator
typedef struct Random {
  uint32_t current_seed_value;  ///< Holds the current seed/random value
  SetSeed set_seed;             ///< Function pointer to SetSeed function
  GetNextUInt16
    get_next_uint16;  ///< Function pointer to GetNextUInt16 function
  GetNextUInt32
    get_next_uint32;  ///< Function pointer to GetNextUInt32 function
} Random;

/// @brief Creates a new Random object
/// @param set_seed_function
/// @param get_next_uint16_function
/// @param get_next_uint32_function
/// @return Pointer to newly created Random object
Random* RandomNew(SetSeed set_seed_function,
                  GetNextUInt16 get_next_uint16_function,
                  GetNextUInt32 get_next_uint32_function);

/// @brief Initializes a Random object
/// @param random Pointer to Random object to initialize
/// @param set_seed_function Used set seed function
/// @param get_next_uint16_function used get next uint16 function
/// @param get_next_uint32_function used get next uint32 function
void RandomInit(Random* const random,
                SetSeed set_seed_function,
                GetNextUInt16 get_next_uint16_function,
                GetNextUInt32 get_next_uint32_function);

/// @brief Delete a Random object
/// @param random Pointer to pointer to Random object to delete
void RandomDelete(Random** random);

#endif  // SRC_UTILS_RANDOM_H_
