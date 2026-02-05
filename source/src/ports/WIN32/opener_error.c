/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file WIN32/opener_error.c
 *  @author Martin Melik Merkumians
 *  @brief This file includes the prototypes for error resolution functions like
 * strerror or WSAGetLastError
 *
 */

#include "ports/opener_error.h"

#include <windows.h>

int GetSocketErrorNumber(void) {
  return WSAGetLastError();
}

/** @brief Format error message into a caller-provided buffer
 * This avoids heap allocation and uses Windows FormatMessage API
 * @param error_number Error code to format
 * @param buffer Caller-provided buffer for error message
 * @param buffer_size Size of the provided buffer
 * @return pointer to the buffer for convenience
 */
char* GetErrorMessage(int error_number, char* buffer, size_t buffer_size) {
  if (buffer == NULL || buffer_size == 0) {
    return "";
  }

  /* Use Windows API to format the error message directly into caller buffer */
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
                 error_number,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPSTR)buffer,
                 (DWORD)buffer_size,
                 NULL);
  return buffer;
}
