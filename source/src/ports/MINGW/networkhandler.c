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


static EipStatus SocketResult(
   const int result,
   size_t *const bytes_xfr
);


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

void ShutdownSocketPlatform(socket_platform_t socket_handle) {
#warning "Untested. Is a shutdown() needed under Windows like for the POSIX port?"
}

void CloseSocketPlatform(socket_platform_t socket_handle) {
  closesocket(socket_handle);
}

int SetSocketToNonBlocking(socket_platform_t socket_handle) {
  u_long iMode = 1;
  return ioctlsocket(socket_handle, FIONBIO, &iMode);
}

int SetQosOnSocket(socket_platform_t socket,
                   CipUsint qos_value) {
  /* Quote from Vol. 2, Section 5-7.4.2 DSCP Value Attributes:
   *  Note that the DSCP value, if placed directly in the ToS field
   *  in the IP header, must be shifted left 2 bits. */
  DWORD set_tos = qos_value << 2;
  return setsockopt(socket, IPPROTO_IP, IP_TOS, (char *)&set_tos, sizeof(set_tos) );
}


/*
 * This wrapper needs to convert the two length parameters from size_t
 * to int, and uses the Windows SOCKET_ERROR constant to detect success
 * or failure.
 */
EipStatus SendToPlatform(
   const socket_platform_t socket,
   const void *const buf,
   const size_t buf_len,
   const int flags,
   const struct sockaddr *const dest_addr,
   const socklen_t dest_len,
   size_t *const bytes_sent
) {
   /* Sanity check for parameters cast to a different type. */
   OPENER_ASSERT(buf_len <= INT_MAX);
   OPENER_ASSERT(dest_len <= INT_MAX);

   const int result = sendto(
      socket,
      buf,
      (int)buf_len,
      flags,
      dest_addr,
      (int)dest_len
   );

   return SocketResult(result, bytes_sent);
}


/*
 * This wrapper needs to convert the length parameter from size_t
 * to int, and uses the Windows SOCKET_ERROR constant to detect success
 * or failure.
 */
EipStatus SendPlatform(
   const socket_platform_t socket,
   const void *const buf,
   const size_t buf_len,
   const int flags,
   size_t *const bytes_sent
) {
   /* Sanity check for parameters cast to a different type. */
   OPENER_ASSERT(buf_len <= INT_MAX);

   const int result = send(
      socket,
      buf,
      (int)buf_len,
      flags
   );

   return SocketResult(result, bytes_sent);
}


/*
 * This wrapper needs to convert the length parameter from size_t to int,
 * and use the Windows SOCKET_ERROR constant to detect success or failure.
 */
EipStatus RecvPlatform(
   const socket_platform_t socket,
   void *const buf,
   const size_t buf_len,
   const int flags,
   size_t *const bytes_received
) {
   /* Sanity check for parameters cast to a different type. */
   OPENER_ASSERT(buf_len <= INT_MAX);

   const int result = recv(
      socket,
      buf,
      (int)buf_len,
      flags
   );

   return SocketResult(result, bytes_received);
}


/** @brief Clean up function to handle send/recv results.
 *
 * @param result Return value from the send/recv call.
 * @param bytes_xfr Location to store the number of bytes transferred
 *                  if the underlying socket call was successful.
 *
 * @return kEipStatusOk or kEipStatusError.
 */
static EipStatus SocketResult(
   const int result,
   size_t *const bytes_xfr
) {
   /* Convert the return value to an EIP status constant. */
   const EipStatus status =
     (result == SOCKET_ERROR) ? kEipStatusError : kEipStatusOk;

   /* Update the number of bytes sent if successful. */
   if ((status == kEipStatusOk) && (bytes_xfr != NULL)) {
      OPENER_ASSERT(result <= MAXSIZE_T);
      *bytes_xfr = (size_t)result;
   }

   return status;
}
