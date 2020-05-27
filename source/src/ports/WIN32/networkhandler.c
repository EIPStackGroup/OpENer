/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>

#include "networkhandler.h"

#include "generic_networkhandler.h"

MicroSeconds GetMicroSeconds() {
  LARGE_INTEGER performance_counter;
  LARGE_INTEGER performance_frequency;

  QueryPerformanceCounter(&performance_counter);
  QueryPerformanceFrequency(&performance_frequency);

  return (MicroSeconds) (performance_counter.QuadPart * 1000000LL
                         / performance_frequency.QuadPart);
}

MilliSeconds GetMilliSeconds(void) {
  return (MilliSeconds) (GetMicroSeconds() / 1000ULL);
}

EipStatus NetworkHandlerInitializePlatform(void) {
  WSADATA wsaData;
  const WORD wVersionRequested = MAKEWORD(2, 2);
  WSAStartup(wVersionRequested, &wsaData);

  return kEipStatusOk;
}

void ShutdownSocketPlatform(int socket_handle) {
#pragma \
  message "Untested. Is a shutdown() needed under Windows like for the POSIX port?"
}

void CloseSocketPlatform(int socket_handle) {
  closesocket(socket_handle);
}

int SetSocketToNonBlocking(int socket_handle) {
  u_long iMode = 1;
  return ioctlsocket(socket_handle, FIONBIO, &iMode);
}

int SetQosOnSocket(const int socket,
                   CipUsint qos_value) {
  return 0; // Dummy implementation, until a working one is viable
}
