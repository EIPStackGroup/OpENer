/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "random.h"

#include <stdlib.h>

Random *RandomNew(SetSeed set_seed,
                  GetNextUInt32 get_next_uint32) {
  Random *random = malloc( sizeof(Random) );
  random->set_seed = set_seed;
  random->get_next_uint32 = get_next_uint32;
  return random;
}

void RandomDelete(Random **random) {
  free(*random);
  *random = NULL;
}
