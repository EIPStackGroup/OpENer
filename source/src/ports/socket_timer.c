/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "socket_timer.h"

#include "trace.h"

void SocketTimerSetSocket(SocketTimer *const socket_timer,
                          const int socket) {
  socket_timer->socket = socket;
  OPENER_TRACE_INFO("Adds socket %d to socket timers\n", socket);
}

void SocketTimerSetLastUpdate(SocketTimer *const socket_timer,
                              const MilliSeconds actual_time) {
  if (NULL != socket_timer) {
    socket_timer->last_update = actual_time;
    OPENER_TRACE_INFO("Sets time stamp for socket %d\n", socket_timer->socket);
  }
}

MilliSeconds SocketTimerGetLastUpdate(SocketTimer *const socket_timer) {
  return socket_timer->last_update;
}

void SocketTimerClear(SocketTimer *const socket_timer) {
  socket_timer->socket = kEipInvalidSocket;
  socket_timer->last_update = 0;
}

void SocketTimerArrayInitialize(SocketTimer *const array_of_socket_timers,
                                const size_t array_length) {
  for (size_t i = 0; i < array_length; ++i) {
    SocketTimerClear(&array_of_socket_timers[i]);
  }
}

SocketTimer *SocketTimerArrayGetSocketTimer(
  SocketTimer *const array_of_socket_timers,
  const size_t array_length,
  const int socket) {
  for (size_t i = 0; i < array_length; ++i) {
    if (socket == array_of_socket_timers[i].socket) {
      return &array_of_socket_timers[i];
    }
  }
  return NULL;
}

SocketTimer *SocketTimerArrayGetEmptySocketTimer(
  SocketTimer *const array_of_socket_timers,
  const size_t array_length) {
  return SocketTimerArrayGetSocketTimer(array_of_socket_timers, array_length,
                                        kEipInvalidSocket);
}
