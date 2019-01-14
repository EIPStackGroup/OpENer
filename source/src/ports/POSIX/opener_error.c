/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file POSIX/opener_error.c
 *  @author Martin Melik Merkumians
 *  @brief This file includes the prototypes for error resolution functions like strerror or WSAGetLastError
 *
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "opener_error.h"

const int kErrorMessageBufferSize = 255;

int GetSocketErrorNumber() {
  return errno;
}

char *GetErrorMessage(int error_number) {
  char *error_message = malloc(kErrorMessageBufferSize);
  strerror_r(error_number, error_message, kErrorMessageBufferSize);
  return error_message;
}

void FreeErrorMessage(char *error_message) {
  free(error_message);
}

