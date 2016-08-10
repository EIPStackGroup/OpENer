/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file opener_error.c
 *  @author Martin Melik Merkumians
 *  @brief This file includes the prototypes for error resolution functions like strerror or WSAGetLastError
 *
 */

 #include <windows.h>

 #include "opener_error.h"

int GetSocketErrorNumber() {
	return WSAGetLastError();
}

char *GetErrorMessage(int error_number) {
  	char *error_message = NULL;
  	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error_number, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
               &error_message, 0, NULL);
  	return error_message;
  }