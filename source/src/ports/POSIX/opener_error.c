/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file POSIX/opener_error.c
 *  @author Martin Melik Merkumians
 *  @brief This file includes the prototypes for error resolution functions like
 * strerror or WSAGetLastError
 *
 */

// Force the use of the XSI compliant strerror_r() function.
#undef _GNU_SOURCE

#include "ports/opener_error.h"

#include <errno.h>
#include <string.h>

int GetSocketErrorNumber(void) {
  return errno;
}

/* Format error message into a caller-provided buffer
 * This avoids heap allocation and buffer overflow risks
 * @param error_number Error code to format
 * @param buffer Caller-provided buffer for error message
 * @param buffer_size Size of the provided buffer
 * @return pointer to the buffer for convenience
 */
char* GetErrorMessage(int error_number, char* buffer, size_t buffer_size) {
  if (buffer != NULL && buffer_size != 0) {
    strerror_r(error_number, buffer, buffer_size);
  }
  return buffer;
}
