/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_PORTS_SOCKET_TIMER_H_
#define SRC_PORTS_SOCKET_TIMER_H_

#include "typedefs.h"

/** @brief Data structure to store last usage times for sockets
 *
 */
typedef struct socket_timer {
  int socket;       /**< key */
  MilliSeconds last_update;       /**< time stop of last update */
} SocketTimer;


/** @brief
 * Sets socket of a Socket Timer
 *
 * @param socket_timer Socket Timer to be set
 * @param socket Socket handle
 */
void SocketTimerSetSocket(SocketTimer *const socket_timer,
                          const int socket);

/** @brief
 * Sets time stamp entry of the Socket Timer
 *
 * @param socket_timer Socket Timer to be set
 * @param actual_time Time stamp
 */
void SocketTimerSetLastUpdate(SocketTimer *const socket_timer,
                              const MilliSeconds actual_time);

/** @brief
 * Gets time stamp of the last update
 *
 * @param socket_timer Socket Timer to be set
 * @return Last update field value
 */
MilliSeconds SocketTimerGetLastUpdate(SocketTimer *const socket_timer);

/** @brief
 * Clears a Socket Timer entry
 *
 * @param socket_timer Socket Timer to be cleared
 */
void SocketTimerClear(SocketTimer *const socket_timer);

/** @brief
 * Initializes an array of Socket Timer entries
 *
 * @param array_of_socket_timers The array of Socket Timer entries to be initialized
 * @param array_length the length of the array
 */
void SocketTimerArrayInitialize(SocketTimer *const array_of_socket_timers,
                                const size_t array_length);

/** @brief
 * Get the Socket Timer entry with the spezified socket value
 *
 * @param array_of_socket_timers The Socket Timer array
 * @param array_length The Socket Timer array length
 * @param socket The socket value to be searched for
 *
 * @return The Socket Timer of found, otherwise NULL
 */
SocketTimer *SocketTimerArrayGetSocketTimer(
  SocketTimer *const array_of_socket_timers,
  const size_t array_length,
  const int socket);

/** @brief
 * Get an empty Socket Timer entry
 *
 * @param array_of_socket_timers The Socket Timer array
 * @param array_length The Socket Timer array length
 *
 * @return An empty Socket Timer entry, or NULL if non is available
 */
SocketTimer *SocketTimerArrayGetEmptySocketTimer(
  SocketTimer *const array_of_socket_timers,
  const size_t array_length);

#endif /* SRC_PORTS_SOCKET_TIMER_H_ */
