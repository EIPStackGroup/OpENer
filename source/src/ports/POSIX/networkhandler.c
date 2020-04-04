/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "networkhandler.h"

#include "opener_error.h"
#include "trace.h"
#include "encap.h"
#include "opener_user_conf.h"

static EipStatus SocketResult(const int result,
                              size_t *const bytes_xfr);

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

void ShutdownSocketPlatform(socket_platform_t socket_handle) {
  if(0 != shutdown(socket_handle, SHUT_RDWR) ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("Failed shutdown() socket %d - Error Code: %d - %s\n",
                     socket_handle,
                     error_code,
                     error_message);
    FreeErrorMessage(error_message);
  }
}

void CloseSocketPlatform(socket_platform_t socket_handle) {
  close(socket_handle);
}

int SetSocketToNonBlocking(socket_platform_t socket_handle) {
  return fcntl(socket_handle, F_SETFL, fcntl(socket_handle,
                                             F_GETFL,
                                             0) | O_NONBLOCK);
}

int SetQosOnSocket(const socket_platform_t socket,
                   CipUsint qos_value) {
  /* Quote from Vol. 2, Section 5-7.4.2 DSCP Value Attributes:
   *  Note that the DSCP value, if placed directly in the ToS field
   *  in the IP header, must be shifted left 2 bits. */
  int set_tos = qos_value << 2;
  return setsockopt(socket, IPPROTO_IP, IP_TOS, &set_tos, sizeof(set_tos) );
}

/*
 * This wrapper only needs to translate the return value into one of
 * the EIP status constants.
 */
EipStatus SendToPlatform(const socket_platform_t socket,
                         const void *const buf,
                         const size_t buf_len,
                         const int flags,
                         const struct sockaddr *const dest_addr,
                         const socklen_t dest_len,
                         size_t *const bytes_sent) {
  const ssize_t result = sendto(socket,
                                buf,
                                buf_len,
                                flags,
                                dest_addr,
                                dest_len);

  return SocketResult(result, bytes_sent);
}

/*
 * This wrapper only needs to translate the return value into one of
 * the EIP status constants.
 */
EipStatus SendPlatform(const socket_platform_t socket,
                       const void *const buf,
                       const size_t buf_len,
                       const int flags,
                       size_t *const bytes_sent) {
  const int result = send(socket,
                          buf,
                          buf_len,
                          flags);

  return SocketResult(result, bytes_sent);
}

/*
 * This wrapper only needs to translate the return value into one of
 * the EIP status constants.
 */
EipStatus RecvPlatform(const socket_platform_t socket,
                       void *const buf,
                       const size_t buf_len,
                       const int flags,
                       size_t *const bytes_received) {
  const int result = recv(socket,
                          buf,
                          buf_len,
                          flags);

  return SocketResult(result, bytes_received);
}


/** @brief Clean up function to handle send/recv results.
 *
 * @param result Return value from the send/recv call.
 * @param bytes_sent Location to store the number of bytes transferred
 *                   if the underlying socket call was successful.
 *
 * @return kEipStatusOk or kEipStatusError.
 */
static EipStatus SocketResult(const int result,
                              size_t *const bytes_xfr) {
  /* Convert the return value to an EIP status constant. */
  const EipStatus status = (result == -1) ? kEipStatusError : kEipStatusOk;

  /* Update the number of bytes sent if successful. */
  if ( (status == kEipStatusOk) && (bytes_xfr != NULL) ) {
    OPENER_ASSERT(result <= SIZE_MAX);
    *bytes_xfr = (size_t)result;
  }

  return status;
}
