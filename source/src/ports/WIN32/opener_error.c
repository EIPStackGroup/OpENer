/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file WIN32/opener_error.c
 *  @author Martin Melik Merkumians
 *  @brief This file includes the prototypes for error resolution functions like strerror or WSAGetLastError
 *
 */

#define WIN32_LEAN_AND_MEAN
 #include <windows.h>

 #include "opener_error.h"

int GetSocketErrorNumber(void) {
  return WSAGetLastError();
}

char *GetErrorMessage(int error_number) {
  char *error_message = NULL;
  FormatMessageA(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    error_number,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPSTR)&error_message,
    0,
    NULL);
  return error_message;
}

void FreeErrorMessage(char *error_message) {
  LocalFree(error_message);
}
