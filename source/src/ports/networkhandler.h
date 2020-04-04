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
void ShutdownSocketPlatform(socket_platform_t socket_handle);

/** @brief Platform dependent code to close a socket
 *
 *  @param socket_handle The socket handle to be closed
 */
void CloseSocketPlatform(socket_platform_t socket_handle);

/** @brief Tries to set socket to non blocking behavior
 *
 * @param socket_handle The socket handle to be set
 *
 * @return platform specific result code
 */
int SetSocketToNonBlocking(socket_platform_t socket_handle);

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
int SetQosOnSocket(const socket_platform_t socket,
                   CipUsint qos_value);


/** @brief Platform-dependent implementation of the sendto() function.
 *
 * This is a wrapper for the sendto() function to handle different data
 * types between Windows and POSIX for buf_len, dest_len,
 * and return value. Each implementation should check and cast the
 * length parameters as necessary. The return value is handled differently,
 * serving only to indicate success or failure; the number of bytes sent
 * is relayed via an output parameter instead of the return value.
 *
 * @param socket Same as sendto() parameter.
 * @param buf Same as sendto() parameter.
 * @param buf_len Same as sendto() parameter.
 * @param flags Same as sendto() parameter.
 * @param dest_addr Same as sendto() parameter.
 * @param dest_len Same as sendto() parameter.
 * @param bytes_sent Location to store the number of bytes transmitted
 *                   if the underlying sendto() call was successful.
 *
 * @return kEipStatusOk or kEipStatusError.
 */
EipStatus SendToPlatform(
   const socket_platform_t socket,
   const void *const buf,
   const size_t buf_len,
   const int flags,
   const struct sockaddr *const dest_addr,
   const socklen_t dest_len,
   size_t *const bytes_sent
   );


/** @brief Platform-dependent implementation of the send() function.
 *
 * This is a wrapper for the send() function to handle different data
 * types between Windows and POSIX for buf_len and return value.
 * Each implementation should check and cast the length parameter as
 * necessary. The return value is handled differently,
 * serving only to indicate success or failure; the number of bytes sent
 * is relayed via an output parameter instead of the return value.
 *
 * @param socket Same as send() parameter.
 * @param buf Same as send() parameter.
 * @param buf_len Same as send() parameter.
 * @param flags Same as send() parameter.
 * @param bytes_sent Location to store the number of bytes transmitted
 *                   if the underlying send() call was successful.
 *
 * @return kEipStatusOk or kEipStatusError.
 */
EipStatus SendPlatform(
   const socket_platform_t socket,
   const void *const buf,
   const size_t buf_len,
   const int flags,
   size_t *const bytes_sent
   );


/** @brief Platform-dependent inplementation of the recv() function.
 *
 * This is a wrapper for recv() to handle different data
 * types between Windows and POSIX for buf_len and return value.
 * Each implementation should check and cast the length parameter as
 * necessary. The return value is handled differently,
 * serving only to indicate success or failure; the number of bytes received
 * is relayed via an output parameter instead of the return value.
 *
 * @param socket Same as recv() parameter.
 * @param buf Same as recv() parameter.
 * @param buf_len Same as recv() parameter.
 * @param flags Same as recv() parameter.
 * @param bytes_sent Location to store the number of bytes received
 *                   if the underlying send() call was successful.
 *
 * @return kEipStatusOk or kEipStatusError.
 */
EipStatus RecvPlatform(
   const socket_platform_t socket,
   void *const buf,
   const size_t buf_len,
   const int flags,
   size_t *const bytes_received
);


#endif /* OPENER_NETWORKHANDLER_H_ */
