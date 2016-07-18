/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file opener_error.h
 *  @author Martin Melik Merkumians
 *  @brief This file includes the prototypes for error resolution functions like strerror_r or WSAGetLastError
 *
 */

/**
 * @brief Gets the error number or equivalent
 *
 * A delegate which implements how to get the error number from the system
 *
 * @return Error number
 */
int GetSocketErrorNumber();


/**
 * @brief Returns a human readable message for the given error number
 *
 * Returns a human readable error message to be used in logs and traces.
 * The error message shall not be a shared memory, like the classic strerror function, as such functions are non-reentrant
 * The user of this function is responsible to free the space in which the error message is returned
 *
 * @return A human readable error message for the given error number
 */
char *GetErrorMessage(int error_number);
