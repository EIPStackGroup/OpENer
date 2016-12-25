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

#include "encap.h"

MicroSeconds GetMicroSeconds(void) {
  struct timespec now;

  clock_gettime( CLOCK_MONOTONIC, &now );
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
  shutdown(socket_handle, SHUT_RDWR);
  close(socket_handle);
}

int SetSocketToNonBlocking(int socket_handle) {
  return fcntl(socket_handle, F_SETFL, fcntl(socket_handle,
                                             F_GETFL,
                                             0) | O_NONBLOCK);
}
