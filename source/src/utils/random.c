/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "utils/random.h"

#include <assert.h>
#include <stdlib.h>

Random* RandomNew(SetSeed set_seed_function,
                  GetNextUInt16 get_next_uint16_function,
                  GetNextUInt32 get_next_uint32_function) {
  Random* random = malloc(sizeof(Random));
  assert(random != NULL && "Failed to allocate Random struct");
  RandomInit(random,
             set_seed_function,
             get_next_uint16_function,
             get_next_uint32_function);
  return random;
}

void RandomInit(Random* const random,
                SetSeed set_seed_function,
                GetNextUInt16 get_next_uint16_function,
                GetNextUInt32 get_next_uint32_function) {
  assert(random != NULL && "Cannot initialize NULL Random struct");
  // The next line is fine, just false positive from cpplint due to formatting
  *random = (Random){ .set_seed        = set_seed_function,
                      .get_next_uint16 = get_next_uint16_function,
                      .get_next_uint32 = get_next_uint32_function };  // NOLINT
}

void RandomDelete(Random** random) {
  free(*random);
  *random = NULL;
}
