/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_NETWORKHANDLER_H_
#define OPENER_NETWORKHANDLER_H_

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "typedefs.h"

#define OPENER_SOCKET_WOULD_BLOCK EWOULDBLOCK

/** @brief Executes platform dependent network handler initialization code
 *
 *      @return EipStatusOk if initialization was successful, otherwise EipStatusError
 */
EipStatus NetworkHandlerInitializePlatform(void);

/** @brief Platform dependent code to close a socket
 *
 *  @param socket_handle The socket handle to be closed
 */
void CloseSocketPlatform(int socket_handle);

int SetSocketToNonBlocking(int socket_handle);

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

int SetQosOnSocket(int socket,
                   CipUsint qos_value);

#endif /* OPENER_NETWORKHANDLER_H_ */
