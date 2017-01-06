/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
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

void SocketTimerSetSocket(SocketTimer *const socket_timer, const int socket);

void SocketTimerSetLastUpdate(SocketTimer *const socket_timer,
                              const MilliSeconds actual_time);

MilliSeconds GetSocketTimerLastUpdate(SocketTimer *const socket_timer);

void DeleteSocketTimer(SocketTimer *const socket_timer);

void SocketTimerArrayInitialize(SocketTimer *const array_of_socket_timers,
                                const size_t array_length);

SocketTimer *GetSocketTimer(SocketTimer *const array_of_socket_timers,
                            const size_t array_length,
                            const int socket);

SocketTimer *GetEmptySocketTimer(SocketTimer *const array_of_socket_timers,
                                 const size_t array_length);

#endif /* SRC_PORTS_SOCKET_TIMER_H_ */
