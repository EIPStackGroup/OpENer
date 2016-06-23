/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef NETWORKHANDLER_H_
#define NETWORKHANDLER_H_

#include "typedefs.h"

typedef unsigned long socklen_t;

EipStatus NetworkHandlerInitializePlatform(void);

void CloseSocketPlatform(int socket_handle);

/** @brief This function shall return the current time in microseconds relative to epoch, and shall be implemented in a port specific networkhandler
 *
 *  @return Current time relative to epoch as MicroSeconds
 */
MicroSeconds GetMicroSeconds(void);

/** @brief This function shall return the current time in milliseconds relative to epoch, and shall be implemented in a port specific networkhandler
 *
 *  @return Current time relative to epoch as MilliSeconds
 */
MilliSeconds GetMilliSeconds(void);

#endif /*NETWORKHANDLER_H_*/
