/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file generic_networkhandler.c
 *  @author Martin Melik Merkumians
 *  @brief This file includes all platform-independent functions of the network handler to reduce code duplication
 */

#include "generic_networkhandler.h"

#include "typedefs.h"
#include "trace.h"

/** @brief handle any connection request coming in the TCP server socket.
 *
 */
void CheckAndHandleTcpListenerSocket(void);

/** @brief Checks and processes request received via the UDP unicast socket, currently the implementation is port-specific
 *
 */
void CheckAndHandleUdpUnicastSocket(void);

/** @brief check if data has been received on the UDP broadcast socket and if yes handle it correctly, currently implemented port-specific
 *
 */
void CheckAndHandleUdpLocalBroadcastSocket(void);

/** @brief TODO: FILL IN!
 *
 */
void CheckAndHandleUdpGlobalBroadcastSocket(void);

/** @brief check if on one of the UDP consuming sockets data has been received and if yes handle it correctly
 *
 */
void CheckAndHandleConsumingUdpSockets(void);

/** @brief TODO: FILL IN!
 *
 */
EipStatus HandleDataOnTcpSocket(int socket);

EipBool8 CheckSocketSet(int socket) {
  EipBool8 return_value = false;
  if (FD_ISSET(socket, &read_socket)) {
    if (FD_ISSET(socket, &master_socket)) {
      return_value = true;
    } else {
      OPENER_TRACE_INFO("socket: %d closed with pending message\n", socket);
    }
    FD_CLR(socket, &read_socket);
    /* remove it from the read set so that later checks will not find it */
  }
  return return_value;
}

void CheckAndHandleTcpListenerSocket() {
  int new_socket;
  /* see if this is a connection request to the TCP listener*/
  if (true == CheckSocketSet(g_network_status.tcp_listener)) {
    OPENER_TRACE_INFO("networkhandler: new TCP connection\n");

    new_socket = accept(g_network_status.tcp_listener, NULL, NULL);
    if (new_socket == -1) {
      OPENER_TRACE_ERR("networkhandler: error on accept: %s\n",
                       strerror(errno));
      return;
    }

    FD_SET(new_socket, &master_socket);
    /* add newfd to master set */
    if (new_socket > highest_socket_handle) {
      highest_socket_handle = new_socket;
    }

    OPENER_TRACE_STATE("networkhandler: opened new TCP connection on fd %d\n",
                       new_socket);
  }
}

EipStatus NetworkHandlerProcessOnce(void) {

  read_socket = master_socket;

  g_time_value.tv_sec = 0;
  g_time_value.tv_usec = (
      g_network_status.elapsed_time < kOpenerTimerTickInMilliSeconds ?
          kOpenerTimerTickInMilliSeconds - g_network_status.elapsed_time : 0)
      * 1000; /* 10 ms */

  int ready_socket = select(highest_socket_handle + 1, &read_socket, 0, 0,
                            &g_time_value);

  if (ready_socket == kEipInvalidSocket) {
    if (EINTR == errno) /* we have somehow been interrupted. The default behavior is to go back into the select loop. */
    {
      return kEipStatusOk;
    } else {
      OPENER_TRACE_ERR("networkhandler: error with select: %s\n",
                       strerror(errno));
      return kEipStatusError;
    }
  }

  if (ready_socket > 0) {

    CheckAndHandleTcpListenerSocket();
    CheckAndHandleUdpUnicastSocket();
    CheckAndHandleUdpLocalBroadcastSocket();
    CheckAndHandleUdpGlobalBroadcastSocket();
    CheckAndHandleConsumingUdpSockets();

    for (int socket = 0; socket <= highest_socket_handle; socket++) {
      if (true == CheckSocketSet(socket)) {
        /* if it is still checked it is a TCP receive */
        if (kEipStatusError == HandleDataOnTcpSocket(socket)) /* if error */
        {
          CloseSocket(socket);
          CloseSession(socket); /* clean up session and close the socket */
        }
      }
    }
  }

  g_actual_time = GetMilliSeconds();
  g_network_status.elapsed_time += g_actual_time - g_last_time;
  g_last_time = g_actual_time;

  /* check if we had been not able to update the connection manager for several OPENER_TIMER_TICK.
   * This should compensate the jitter of the windows timer
   */
  while (g_network_status.elapsed_time >= kOpenerTimerTickInMilliSeconds) {
    /* call manage_connections() in connection manager every OPENER_TIMER_TICK ms */
    ManageConnections();
    g_network_status.elapsed_time -= kOpenerTimerTickInMilliSeconds;
  }
  return kEipStatusOk;
}

EipStatus NetworkHandlerFinish(void) {
  CloseSocket(g_network_status.tcp_listener);
  CloseSocket(g_network_status.udp_unicast_listener);
  CloseSocket(g_network_status.udp_local_broadcast_listener);
  CloseSocket(g_network_status.udp_global_broadcast_listener);
  return kEipStatusOk;
}

int GetMaxSocket(int socket1, int socket2, int socket3, int socket4) {
  if ((socket1 > socket2) && (socket1 > socket3) && (socket1 > socket4))
    return socket1;

  if ((socket2 > socket1) && (socket2 > socket3) && (socket2 > socket4))
    return socket2;

  if ((socket3 > socket1) && (socket3 > socket2) && (socket3 > socket4))
    return socket3;

  return socket4;
}
