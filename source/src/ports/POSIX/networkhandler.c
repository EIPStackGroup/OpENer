/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>

#include "networkhandler.h"

#include "opener_error.h"
#include "trace.h"
#include "encap.h"
#include "opener_user_conf.h"

MicroSeconds GetMicroSeconds(void) {
  struct timespec now = { .tv_nsec = 0, .tv_sec = 0 };

  int error = clock_gettime( CLOCK_MONOTONIC, &now );
  OPENER_ASSERT(-1 != error);
  MicroSeconds micro_seconds =  (MicroSeconds)now.tv_nsec / 1000ULL +
                               now.tv_sec * 1000000ULL;
  return micro_seconds;
}

MilliSeconds GetMilliSeconds(void) {
  return (MilliSeconds) (GetMicroSeconds() / 1000ULL);
}

EipStatus NetworkHandlerInitializePlatform(void) {
  /* Add platform dependent code here if necessary */
  return kEipStatusOk;
}

void CloseSocketPlatform(int socket_handle) {
  if(0 != shutdown(socket_handle, SHUT_RDWR) ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("Could not close socket %d - Error Code: %d - %s\n",
                     socket_handle,
                     error_code,
                     error_message);
    FreeErrorMessage(error_message);
  }
  close(socket_handle);
}

int SetSocketToNonBlocking(int socket_handle) {
  return fcntl(socket_handle, F_SETFL, fcntl(socket_handle,
                                             F_GETFL,
                                             0) | O_NONBLOCK);
}

//TODO: Return setsocket return value
int SetQosOnSocket(int socket,
                   CipUsint qos_value) {

  int set_tos = qos_value;
  return setsockopt(socket, IPPROTO_IP, IP_TOS, &set_tos, sizeof(set_tos) );
}
