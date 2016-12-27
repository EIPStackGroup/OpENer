/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>

#include "networkhandler.h"

#include "generic_networkhandler.h"
#include "encap.h"

MicroSeconds getMicroSeconds() {
  LARGE_INTEGER performance_counter;
  LARGE_INTEGER performance_frequency;

  QueryPerformanceCounter(&performance_counter);
  QueryPerformanceFrequency(&performance_frequency);

  return (MicroSeconds) (performance_counter.QuadPart * 1000000LL
                         / performance_frequency.QuadPart);
}

MilliSeconds GetMilliSeconds(void) {
  return (MilliSeconds) (getMicroSeconds() / 1000ULL);
}

EipStatus NetworkHandlerInitializePlatform(void) {
  /* Add platform dependent code here if necessary */
  WORD wVersionRequested;
  WSADATA wsaData;
  wVersionRequested = MAKEWORD(2, 2);
  WSAStartup(wVersionRequested, &wsaData);

  return kEipStatusOk;
}

void CloseSocketPlatform(int socket_handle) {
  closesocket(socket_handle);
}

int SetSocketToNonBlocking(int socket_handle) {
  u_long iMode = 1;
  return ioctlsocket(socket_handle, FIONBIO, &iMode);
}
