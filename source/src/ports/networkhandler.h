/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_NETWORKHANDLER_H_
#define OPENER_NETWORKHANDLER_H_

#include "typedefs.h"

#define OPENER_SOCKET_WOULD_BLOCK EWOULDBLOCK

/** @brief Executes platform dependent network handler initialization code
 *
 *  @return EipStatusOk if initialization was successful, otherwise EipStatusError
 */
EipStatus NetworkHandlerInitializePlatform(void);

/** @brief Platform dependent code to shutdown a stream socket
 *
 *  @param socket_handle The socket to shut down
 */
void ShutdownSocketPlatform(int socket_handle);

/** @brief Platform dependent code to close a socket
 *
 *  @param socket_handle The socket handle to be closed
 */
void CloseSocketPlatform(int socket_handle);

/** @brief Tries to set socket to non blocking behavior
 *
 * @param socket_handle The socket handle to be set
 *
 * @return platform specific result code
 */
int SetSocketToNonBlocking(int socket_handle);

/** @brief Returns current time in microseconds from monotonic time base, please note
 *  that this does not represent a real absolute time, but measured from an arbitrary starting point
 *
 * This function returns the current time relative to an arbitrary starting point from a monotonic time source.
 * As monotonic clocks and clock functions in general are platform dependent, this has to be implemented for each platform
 * (see ports subfolders)
 *
 *  @return Current time relative to monotonic clock starting point as MicroSeconds
 */
MicroSeconds GetMicroSeconds(void);

/** @brief Returns current time in milliseconds from monotonic time base, please note
 *  that this does not represent a real absolute time, but measured from an arbitrary starting point
 *
 * This function returns the current time relative to an arbitrary starting point from a monotonic time source.
 * As monotonic clocks and clock functions in general are platform dependent, this has to be implemented for each platform
 * (see ports subfolders)
 *
 *  @return Current time relative to monotonic clock starting point as MilliSeconds
 */
MilliSeconds GetMilliSeconds(void);

/** @brief Sets QoS on socket
 *
 * A wrapper function - needs a platform dependent implementation to set QoS on a socket
 *
 * @param socket The socket which QoS shall be set
 * @param qos_value The desired QoS value, as specified in ENIP Vol.2 QoS Object
 *
 * @return platform dependent result code
 *
 */
int SetQosOnSocket(const int socket,
                   CipUsint qos_value);

#endif /* OPENER_NETWORKHANDLER_H_ */
