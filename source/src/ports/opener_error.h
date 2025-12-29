/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file opener_error.h
 *  @author Martin Melik Merkumians
 *  @brief This file includes the prototypes for error resolution functions like
 * strerror_r or WSAGetLastError
 *
 */

#ifndef PORTS_OPENER_ERROR_H_
#define PORTS_OPENER_ERROR_H_

#include <stddef.h>

/**
 * @brief Gets the error number or equivalent
 *
 * A delegate which implements how to get the error number from the system
 *
 * @return Error number
 */
int GetSocketErrorNumber(void);

/**
 * @brief Format error message into caller-provided buffer
 *
 * Formats a human readable error message into the caller-provided buffer.
 * This avoids heap allocation and is reentrant-safe.
 * The buffer should be at least 256 bytes for full error messages.
 *
 * @param error_number Error code to format
 * @param buffer Caller-provided buffer for error message (must not be NULL)
 * @param buffer_size Size of the provided buffer
 * @return Pointer to the buffer for convenience
 */
char* GetErrorMessage(int error_number, char* buffer, size_t buffer_size);

#endif  // PORTS_OPENER_ERROR_H_
