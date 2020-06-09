/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "networkhandler.h"

#include "generic_networkhandler.h"

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
  WSADATA wsaData;
  const WORD wVersionRequested = MAKEWORD(2, 2);
  WSAStartup(wVersionRequested, &wsaData);

  return kEipStatusOk;
}

void ShutdownSocketPlatform(int socket_handle) {
#warning \
  "Untested. Is a shutdown() needed under Windows like for the POSIX port?"
}

void CloseSocketPlatform(int socket_handle) {
  closesocket(socket_handle);
}

int SetSocketToNonBlocking(int socket_handle) {
  u_long iMode = 1;
  return ioctlsocket(socket_handle, FIONBIO, &iMode);
}

int SetQosOnSocket(int socket,
                   CipUsint qos_value) {
  /* Quote from Vol. 2, Section 5-7.4.2 DSCP Value Attributes:
   *  Note that the DSCP value, if placed directly in the ToS field
   *  in the IP header, must be shifted left 2 bits. */
  DWORD set_tos = qos_value << 2;
  return setsockopt(socket,
                    IPPROTO_IP,
                    IP_TOS,
                    (char *)&set_tos,
                    sizeof(set_tos) );
}
