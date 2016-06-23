/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_NETWORKHANDLER_H_
#define OPENER_NETWORKHANDLER_H_

#include <string.h>
#include <sys/socket.h>

#include "typedefs.h"

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

#endif /* OPENER_NETWORKHANDLER_H_ */
