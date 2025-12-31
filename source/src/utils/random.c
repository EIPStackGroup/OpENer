/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "utils/random.h"

#include <assert.h>
#include <stdlib.h>

Random* RandomNew(SetSeed set_seed_function,
                  GetNextUInt32 get_next_uint32_function) {
  Random* random = malloc(sizeof(Random));
  assert(random != NULL && "Failed to allocate Random struct");
  // The next line is fine, just false positive from cpplint due to formatting
  *random = (Random){ .set_seed        = set_seed_function,
                      .get_next_uint32 = get_next_uint32_function };  // NOLINT
  return random;
}

void RandomDelete(Random** random) {
  free(*random);
  *random = NULL;
}
