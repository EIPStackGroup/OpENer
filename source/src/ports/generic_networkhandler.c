/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file generic_networkhandler.c
 *  @author Martin Melik Merkumians
 *  @brief This file includes all platform-independent functions of the network handler to reduce code duplication
 *
 *  The generic network handler delegates platform-dependent tasks to the platform network handler
 */

#include <assert.h>
#include <stdbool.h>

#include "generic_networkhandler.h"

#include "typedefs.h"
#include "trace.h"
#include "opener_error.h"
#include "encap.h"
#include "ciptcpipinterface.h"
#include "opener_user_conf.h"
#include "cipqos.h"

#define MAX_NO_OF_TCP_SOCKETS 10

/** @brief handle any connection request coming in the TCP server socket.
 *
 */
void CheckAndHandleTcpListenerSocket(void);

/** @brief Checks and processes request received via the UDP unicast socket, currently the implementation is port-specific
 *
 */
void CheckAndHandleUdpUnicastSocket(void);

/** @brief Checks and handles incoming messages via UDP broadcast
 *
 */
void CheckAndHandleUdpGlobalBroadcastSocket(void);

/** @brief check if on one of the UDP consuming sockets data has been received and if yes handle it correctly
 *
 */
void CheckAndHandleConsumingUdpSockets(void);

/** @brief Handles data on an established TCP connection, processed connection is given by socket
 *
 *  @param socket The socket to be processed
 *  @return kEipStatusOk on success, or kEipStatusError on failure
 */
EipStatus HandleDataOnTcpSocket(int socket);

void CheckEncapsulationInactivity(int socket_handle);

void RemoveSocketTimerFromList(const int socket_handle);

/*************************************************
* Function implementations from now on
*************************************************/

EipStatus NetworkHandlerInitialize(void) {

  if( kEipStatusOk != NetworkHandlerInitializePlatform() ) {
    return kEipStatusError;
  }

  SocketTimerArrayInitialize(g_timestamps, OPENER_NUMBER_OF_SUPPORTED_SESSIONS);

  /* clear the master an temp sets */
  FD_ZERO(&master_socket);
  FD_ZERO(&read_socket);

  /* create a new TCP socket */
  if ( ( g_network_status.tcp_listener =
           socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) )
       == -1 ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("error allocating socket stream listener, %d - %s\n",
                     error_code,
                     error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  int set_socket_option_value = 1;  //Represents true for used set socket options
  /* Activates address reuse */
  if (setsockopt( g_network_status.tcp_listener, SOL_SOCKET, SO_REUSEADDR,
                  (char *) &set_socket_option_value,
                  sizeof(set_socket_option_value) ) == -1) {
    OPENER_TRACE_ERR(
      "error setting socket option SO_REUSEADDR on tcp_listener\n");
    return kEipStatusError;
  }

  if (SetSocketToNonBlocking(g_network_status.tcp_listener) < 0) {
    OPENER_TRACE_ERR(
      "error setting socket to non-blocking on new socket\n");
    return kEipStatusError;
  }

  /* create a new UDP socket */
  if ( ( g_network_status.udp_global_broadcast_listener = socket(AF_INET,
                                                                 SOCK_DGRAM,
                                                                 IPPROTO_UDP) )
       == -1 ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR(
      "error allocating UDP global broadcast listener socket, %d - %s\n",
      error_code,
      error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  /* create a new UDP socket */
  if ( ( g_network_status.udp_unicast_listener = socket(AF_INET, SOCK_DGRAM,
                                                        IPPROTO_UDP) ) ==
       -1 ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("error allocating UDP unicast listener socket, %d - %s\n",
                     error_code, error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  /* Activates address reuse */
  if (setsockopt( g_network_status.udp_global_broadcast_listener, SOL_SOCKET,
                  SO_REUSEADDR, (char *) &set_socket_option_value,
                  sizeof(set_socket_option_value) ) == -1) {
    OPENER_TRACE_ERR(
      "error setting socket option SO_REUSEADDR on udp_broadcast_listener\n");
    return kEipStatusError;
  }

  if (SetSocketToNonBlocking(g_network_status.udp_global_broadcast_listener) <
      0) {
    OPENER_TRACE_ERR(
      "error setting socket to non-blocking on new socket\n");
    return kEipStatusError;
  }

  /* Activates address reuse */
  if (setsockopt( g_network_status.udp_unicast_listener, SOL_SOCKET,
                  SO_REUSEADDR, (char *) &set_socket_option_value,
                  sizeof(set_socket_option_value) ) == -1) {
    OPENER_TRACE_ERR(
      "error setting socket option SO_REUSEADDR on udp_unicast_listener\n");
    return kEipStatusError;
  }

  if (SetSocketToNonBlocking(g_network_status.udp_unicast_listener) < 0) {
    OPENER_TRACE_ERR(
      "error setting socket to non-blocking on udp_unicast_listener\n");
    return kEipStatusError;
  }

  struct sockaddr_in my_address = { .sin_family = AF_INET, .sin_port = htons(
                                      kOpenerEthernetPort),
                                    .sin_addr.s_addr = interface_configuration_
                                                       .
                                                       ip_address };

  /* bind the new socket to port 0xAF12 (CIP) */
  if ( ( bind( g_network_status.tcp_listener, (struct sockaddr *) &my_address,
               sizeof(struct sockaddr) ) ) == -1 ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("error with TCP bind: %d - %s\n", error_code,
                     error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  if ( ( bind( g_network_status.udp_unicast_listener,
               (struct sockaddr *) &my_address,
               sizeof(struct sockaddr) ) ) == -1 ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR( "error with UDP unicast bind: %d - %s\n", error_code,
                      GetErrorMessage(
                        error_code) );
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  struct sockaddr_in global_broadcast_address = { .sin_family = AF_INET,
                                                  .sin_port = htons(
                                                    kOpenerEthernetPort),
                                                  .sin_addr.s_addr = htonl(
                                                    INADDR_ANY) };

  /* enable the UDP socket to receive broadcast messages */
  if ( 0
       > setsockopt( g_network_status.udp_global_broadcast_listener, SOL_SOCKET,
                     SO_BROADCAST,
                     (char *) &set_socket_option_value, sizeof(int) ) ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR(
      "error with setting broadcast receive for UDP socket: %d - %s\n",
      error_code, error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  if ( ( bind( g_network_status.udp_global_broadcast_listener,
               (struct sockaddr *) &global_broadcast_address,
               sizeof(struct sockaddr) ) ) == -1 ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("error with global broadcast UDP bind: %d - %s\n",
                     error_code,
                     error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  /* switch socket in listen mode */
  if ( ( listen(g_network_status.tcp_listener,
                MAX_NO_OF_TCP_SOCKETS) ) == -1 ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("networkhandler: error with listen: %d - %s\n",
                     error_code,
                     error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  /* add the listener socket to the master set */
  FD_SET(g_network_status.tcp_listener, &master_socket);
  FD_SET(g_network_status.udp_unicast_listener, &master_socket);
  FD_SET(g_network_status.udp_global_broadcast_listener, &master_socket);

  /* keep track of the biggest file descriptor */
  highest_socket_handle = GetMaxSocket(
    g_network_status.tcp_listener,
    g_network_status.udp_global_broadcast_listener,
    0,
    g_network_status.udp_unicast_listener);

  g_last_time = GetMilliSeconds(); /* initialize time keeping */
  g_network_status.elapsed_time = 0;

  return kEipStatusOk;
}

void CloseUdpSocket(int socket_handle) {
  CloseSocket(socket_handle);
}

void CloseTcpSocket(int socket_handle) {
  RemoveSocketTimerFromList(socket_handle);
  CloseSocket(socket_handle);
}

void RemoveSocketTimerFromList(const int socket_handle) {
  SocketTimer *socket_timer = NULL;
  while( NULL !=
         ( socket_timer =
             SocketTimerArrayGetSocketTimer(g_timestamps,
                                            OPENER_NUMBER_OF_SUPPORTED_SESSIONS,
                                            socket_handle) ) )
  {
    SocketTimerClear(socket_timer);
  }
}

EipBool8 CheckSocketSet(int socket) {
  EipBool8 return_value = false;
  if ( FD_ISSET(socket, &read_socket) ) {
    if ( FD_ISSET(socket, &master_socket) ) {
      return_value = true;
    } else {
      OPENER_TRACE_INFO("socket: %d closed with pending message\n", socket);
    }
    FD_CLR(socket, &read_socket);
    /* remove it from the read set so that later checks will not find it */
  }
  return return_value;
}

void CheckAndHandleTcpListenerSocket(void) {
  int new_socket = kEipInvalidSocket;
  /* see if this is a connection request to the TCP listener*/
  if ( true == CheckSocketSet(g_network_status.tcp_listener) ) {
    OPENER_TRACE_INFO("networkhandler: new TCP connection\n");

    new_socket = accept(g_network_status.tcp_listener, NULL, NULL);
    if (new_socket == kEipInvalidSocket) {
      int error_code = GetSocketErrorNumber();
      char *error_message = GetErrorMessage(error_code);
      OPENER_TRACE_ERR("networkhandler: error on accept: %d - %s\n",
                       error_code, error_message);
      FreeErrorMessage(error_message);
      return;
    }

    if (SetQosOnSocket( new_socket, GetPriorityForSocket(0xFFF) ) <= 0) { /* got error */
      int error_code = GetSocketErrorNumber();
      char *error_message = GetErrorMessage(error_code);
      OPENER_TRACE_ERR(
        "networkhandler: error on set QoS on on new socket %d: %d - %s\n",
        new_socket,
        error_code,
        error_message);
      FreeErrorMessage(error_message);
    }

    OPENER_TRACE_INFO(">>> network handler: accepting new TCP socket: %d \n",
                      new_socket);

    SocketTimer *socket_timer = SocketTimerArrayGetEmptySocketTimer(
      g_timestamps,
      OPENER_NUMBER_OF_SUPPORTED_SESSIONS);

//    OPENER_TRACE_INFO("Current time stamp: %ld\n", g_actual_time);
//    for(size_t i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; i++) {
//      OPENER_TRACE_INFO("Socket: %d - Last Update: %ld\n",
//                        g_timestamps[i].socket,
//                        g_timestamps[i].last_update);
//    }

    OPENER_ASSERT(socket_timer != NULL);

    FD_SET(new_socket, &master_socket);
    /* add newfd to master set */
    if (new_socket > highest_socket_handle) {
      OPENER_TRACE_INFO("New highest socket: %d\n", new_socket);
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
      int error_code = GetSocketErrorNumber();
      char *error_message = GetErrorMessage(error_code);
      OPENER_TRACE_ERR("networkhandler: error with select: %d - %s\n",
                       error_code,
                       error_message);
      FreeErrorMessage(error_message);
      return kEipStatusError;
    }
  }

  if (ready_socket > 0) {

    CheckAndHandleTcpListenerSocket();
    CheckAndHandleUdpUnicastSocket();
    CheckAndHandleUdpGlobalBroadcastSocket();
    CheckAndHandleConsumingUdpSockets();

    for (size_t socket = 0; socket <= highest_socket_handle; socket++) {
      if ( true == CheckSocketSet(socket) ) {
        /* if it is still checked it is a TCP receive */
        if ( kEipStatusError == HandleDataOnTcpSocket(socket) ) /* if error */
        {
          CloseTcpSocket(socket);
          RemoveSession(socket); /* clean up session and close the socket */
        }
      }
    }
  }

  for (size_t socket = 0; socket <= highest_socket_handle; socket++) {
    CheckEncapsulationInactivity(socket);
  }

  /* Check if all connections from one originator times out */
  //CheckForTimedOutConnectionsAndCloseTCPConnections();

  //OPENER_TRACE_INFO("Socket Loop done\n");

  g_actual_time = GetMilliSeconds();
  g_network_status.elapsed_time += g_actual_time - g_last_time;
  g_last_time = g_actual_time;
  //OPENER_TRACE_INFO("Elapsed time: %u\n", g_network_status.elapsed_time);

  /* check if we had been not able to update the connection manager for several OPENER_TIMER_TICK.
   * This should compensate the jitter of the windows timer
   */
  if (g_network_status.elapsed_time >= kOpenerTimerTickInMilliSeconds) {
    /* call manage_connections() in connection manager every kOpenerTimerTickInMilliSeconds ms */
    ManageConnections(g_network_status.elapsed_time);
    g_network_status.elapsed_time = 0;
  }
  return kEipStatusOk;
}

EipStatus NetworkHandlerFinish(void) {
  CloseSocket(g_network_status.tcp_listener);
  CloseSocket(g_network_status.udp_unicast_listener);
  CloseSocket(g_network_status.udp_global_broadcast_listener);
  return kEipStatusOk;
}

void CheckAndHandleUdpGlobalBroadcastSocket(void) {

  /* see if this is an unsolicited inbound UDP message */
  if ( true ==
       CheckSocketSet(g_network_status.udp_global_broadcast_listener) ) {
    struct sockaddr_in from_address = {0};
    socklen_t from_address_length = sizeof(from_address);

    OPENER_TRACE_STATE(
      "networkhandler: unsolicited UDP message on EIP global broadcast socket\n");

    /* Handle UDP broadcast messages */
    int received_size = recvfrom(g_network_status.udp_global_broadcast_listener,
                                 g_ethernet_communication_buffer,
                                 PC_OPENER_ETHERNET_BUFFER_SIZE,
                                 0, (struct sockaddr *) &from_address,
                                 &from_address_length);

    if (received_size <= 0) { /* got error */
      int error_code = GetSocketErrorNumber();
      char *error_message = GetErrorMessage(error_code);
      OPENER_TRACE_ERR(
        "networkhandler: error on recvfrom UDP global broadcast port: %d - %s\n",
        error_code,
        error_message);
      FreeErrorMessage(error_message);
      return;
    }

    OPENER_TRACE_INFO("Data received on global broadcast UDP:\n");

    EipUint8 *receive_buffer = &g_ethernet_communication_buffer[0];
    int remaining_bytes = 0;
    do {
      int reply_length = HandleReceivedExplictUdpData(
        g_network_status.udp_global_broadcast_listener, &from_address,
        receive_buffer, received_size, &remaining_bytes, false);

      receive_buffer += received_size - remaining_bytes;
      received_size = remaining_bytes;

      if (reply_length > 0) {
        OPENER_TRACE_INFO("UDP broadcast reply sent:\n");

        /* if the active socket matches a registered UDP callback, handle a UDP packet */
        if (sendto( g_network_status.udp_global_broadcast_listener,
                    (char *) g_ethernet_communication_buffer, reply_length, 0,
                    (struct sockaddr *) &from_address, sizeof(from_address) )
            != reply_length) {
          OPENER_TRACE_INFO(
            "networkhandler: UDP response was not fully sent\n");
        }
      }
    } while (remaining_bytes > 0);
  }
}

void CheckAndHandleUdpUnicastSocket(void) {


  /* see if this is an unsolicited inbound UDP message */
  if ( true == CheckSocketSet(g_network_status.udp_unicast_listener) ) {

    struct sockaddr_in from_address = {0};
    socklen_t from_address_length = sizeof(from_address);

    OPENER_TRACE_STATE(
      "networkhandler: unsolicited UDP message on EIP unicast socket\n");

    /* Handle UDP broadcast messages */
    int received_size = recvfrom(g_network_status.udp_unicast_listener,
                                 g_ethernet_communication_buffer,
                                 PC_OPENER_ETHERNET_BUFFER_SIZE,
                                 0, (struct sockaddr *) &from_address,
                                 &from_address_length);

    if (received_size <= 0) { /* got error */
      int error_code = GetSocketErrorNumber();
      char *error_message = GetErrorMessage(error_code);
      OPENER_TRACE_ERR(
        "networkhandler: error on recvfrom UDP unicast port: %d - %s\n",
        error_code,
        error_message);
      FreeErrorMessage(error_message);
      return;
    }

    OPENER_TRACE_INFO("Data received on UDP unicast:\n");

    EipUint8 *receive_buffer = &g_ethernet_communication_buffer[0];
    int remaining_bytes = 0;
    do {
      int reply_length = HandleReceivedExplictUdpData(
        g_network_status.udp_unicast_listener, &from_address, receive_buffer,
        received_size, &remaining_bytes, true);

      receive_buffer += received_size - remaining_bytes;
      received_size = remaining_bytes;

      if (reply_length > 0) {
        OPENER_TRACE_INFO("UDP unicast reply sent:\n");

        /* if the active socket matches a registered UDP callback, handle a UDP packet */
        if (sendto( g_network_status.udp_unicast_listener,
                    (char *) g_ethernet_communication_buffer, reply_length, 0,
                    (struct sockaddr *) &from_address, sizeof(from_address) )
            != reply_length) {
          OPENER_TRACE_INFO(
            "networkhandler: UDP unicast response was not fully sent\n");
        }
      }
    } while (remaining_bytes > 0);
  }
}

EipStatus SendUdpData(struct sockaddr_in *address,
                      int socket,
                      EipUint8 *data,
                      EipUint16 data_length) {

  int sent_length = sendto( socket, (char *) data, data_length, 0,
                            (struct sockaddr *) address, sizeof(*address) );

  if (sent_length < 0) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR(
      "networkhandler: error with sendto in sendUDPData: %d - %s\n",
      error_code,
      error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  if (sent_length != data_length) {
    OPENER_TRACE_WARN(
      "data length sent_length mismatch; probably not all data was sent in SendUdpData, sent %d of %d\n",
      sent_length,
      data_length);
    return kEipStatusError;
  }

  return kEipStatusOk;
}

EipStatus HandleDataOnTcpSocket(int socket) {
  OPENER_TRACE_INFO("Entering HandleDataOnTcpSocket for socket: %d\n", socket);
  int remaining_bytes = 0;
  long data_sent = PC_OPENER_ETHERNET_BUFFER_SIZE;

  /* We will handle just one EIP packet here the rest is done by the select
   * method which will inform us if more data is available in the socket
     because of the current implementation of the main loop this may not be
     the fastest way and a loop here with a non blocking socket would better
     fit*/

  /*Check how many data is here -- read the first four bytes from the connection */
  long number_of_read_bytes = recv(socket, g_ethernet_communication_buffer, 4,
                                   0); /*TODO we may have to set the socket to a non blocking socket */

  SocketTimer *socket_timer = SocketTimerArrayGetSocketTimer(
    g_timestamps,
    OPENER_NUMBER_OF_SUPPORTED_SESSIONS,
    socket);
  if (number_of_read_bytes == 0) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR(
      "networkhandler: socket: %d - connection closed by client: %d - %s\n",
      socket,
      error_code,
      error_message);
    FreeErrorMessage(error_message);
    RemoveSocketTimerFromList(socket);
    RemoveSession(socket);
    return kEipStatusError;
  }
  if (number_of_read_bytes < 0) {
    int error_code = GetSocketErrorNumber();
    if (OPENER_SOCKET_WOULD_BLOCK == error_code) {
      return kEipStatusOk;
    }
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("networkhandler: error on recv: %d - %s\n",
                     error_code,
                     error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  const EipUint8 *read_buffer = &g_ethernet_communication_buffer[2]; /* at this place EIP stores the data length */
  size_t data_size = GetIntFromMessage(&read_buffer)
                     + ENCAPSULATION_HEADER_LENGTH - 4; /* -4 is for the 4 bytes we have already read*/
  /* (NOTE this advances the buffer pointer) */
  if ( (PC_OPENER_ETHERNET_BUFFER_SIZE - 4) < data_size ) { /*TODO can this be handled in a better way?*/
    OPENER_TRACE_ERR(
      "too large packet received will be ignored, will drop the data\n");
    /* Currently we will drop the whole packet */

    do {
      OPENER_TRACE_INFO(
        "Entering consumption loop, remaining data to receive: %zu\n",
        data_sent);
      number_of_read_bytes = recv(socket, &g_ethernet_communication_buffer[0],
                                  data_sent, 0);

      if (number_of_read_bytes == 0) /* got error or connection closed by client */
      {
        int error_code = GetSocketErrorNumber();
        char *error_message = GetErrorMessage(error_code);
        OPENER_TRACE_ERR(
          "networkhandler: socket: %d - connection closed by client: %d - %s\n",
          socket,
          error_code,
          error_message);
        FreeErrorMessage(error_message);
        RemoveSocketTimerFromList(socket);
        return kEipStatusError;
      }
      if (number_of_read_bytes < 0) {
        int error_code = GetSocketErrorNumber();
        char *error_message = GetErrorMessage(error_code);
        if (OPENER_SOCKET_WOULD_BLOCK == error_code) {
          return kEipStatusOk;
        }
        OPENER_TRACE_ERR("networkhandler: error on recv: %d - %s\n",
                         error_code,
                         error_message);
        FreeErrorMessage(error_message);
        return kEipStatusError;
      }
      data_size -= number_of_read_bytes;
      if ( (data_size < PC_OPENER_ETHERNET_BUFFER_SIZE) && (data_size != 0) ) {
        data_sent = data_size;
      }
    } while (0 < data_size);
    SocketTimerSetLastUpdate(socket_timer, g_actual_time);
    return kEipStatusOk;
  }

  number_of_read_bytes = recv(socket, &g_ethernet_communication_buffer[4],
                              data_size, 0);

  if (0 == number_of_read_bytes) /* got error or connection closed by client */
  {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR(
      "networkhandler: socket: %d - connection closed by client: %d - %s\n",
      socket,
      error_code,
      error_message);
    FreeErrorMessage(error_message);
    RemoveSocketTimerFromList(socket);
    RemoveSession(socket);
    return kEipStatusError;
  }
  if (number_of_read_bytes < 0) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    if (OPENER_SOCKET_WOULD_BLOCK == error_code) {
      return kEipStatusOk;
    }
    OPENER_TRACE_ERR("networkhandler: error on recv: %d - %s\n",
                     error_code,
                     error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }

  if ( (unsigned) number_of_read_bytes == data_size ) {
    /*we got the right amount of data */
    data_size += 4;
    /*TODO handle partial packets*/
    OPENER_TRACE_INFO("Data received on tcp:\n");

    g_current_active_tcp_socket = socket;

    struct sockaddr sender_address;
    memset( &sender_address, 0, sizeof(sender_address) );
    socklen_t fromlen = sizeof(sender_address);
    if (getpeername(socket, (struct sockaddr *)&sender_address, &fromlen) < 0) {
      int error_code = GetSocketErrorNumber();
      char *error_message = GetErrorMessage(error_code);
      OPENER_TRACE_ERR("networkhandler: could not get peername: %d - %s\n",
                       error_code,
                       error_message);
      FreeErrorMessage(error_message);
    }

    number_of_read_bytes = HandleReceivedExplictTcpData(
      socket, g_ethernet_communication_buffer, data_size, &remaining_bytes,
      &sender_address);
    SocketTimer *socket_timer = SocketTimerArrayGetSocketTimer(
      g_timestamps,
      OPENER_NUMBER_OF_SUPPORTED_SESSIONS,
      socket);
    if(NULL != socket_timer) {
      SocketTimerSetLastUpdate(socket_timer, g_actual_time);
    }

    g_current_active_tcp_socket = kEipInvalidSocket;

    if (remaining_bytes != 0) {
      OPENER_TRACE_WARN(
        "Warning: received packet was to long: %d Bytes left!\n",
        remaining_bytes);
    }

    if (number_of_read_bytes > 0) {
      OPENER_TRACE_INFO("TCP reply sent:\n");

      data_sent = send(socket, (char *) &g_ethernet_communication_buffer[0],
                       number_of_read_bytes, 0);
      SocketTimer *socket_timer = SocketTimerArrayGetSocketTimer(
        g_timestamps,
        OPENER_NUMBER_OF_SUPPORTED_SESSIONS,
        socket);
      SocketTimerSetLastUpdate(socket_timer, g_actual_time);
      if (data_sent != number_of_read_bytes) {
        OPENER_TRACE_WARN("TCP response was not fully sent\n");
      }
    }

    return kEipStatusOk;
  } else {
    /* we got a fragmented packet currently we cannot handle this will
     * for this we would need a network buffer per TCP socket
     *
     * However with typical packet sizes of EIP this should't be a big issue.
     */
    /*TODO handle fragmented packets */
  }
  return kEipStatusError;
}

/** @brief create a new UDP socket for the connection manager
 *
 * @param communciation_direction Consuming or producing port
 * @param socket_data Data for socket creation
 *
 * @return the socket handle if successful, else -1 */
int CreateUdpSocket(UdpCommuncationDirection communication_direction,
                    struct sockaddr_in *socket_data,
                    CipUsint qos_for_socket) {
  struct sockaddr_in peer_address;
  int new_socket = kEipInvalidSocket;

  socklen_t peer_address_length = sizeof(struct sockaddr_in);
  /* create a new UDP socket */
  if ( ( new_socket =
           socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) == kEipInvalidSocket ) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR("networkhandler: cannot create UDP socket: %d- %s\n",
                     error_code,
                     error_message);
    FreeErrorMessage(error_message);
    return new_socket;
  }

  if (SetSocketToNonBlocking(new_socket) < 0) {
    OPENER_TRACE_ERR(
      "error setting socket to non-blocking on new socket\n");
    CloseSocket(new_socket);
    OPENER_ASSERT(false); /* This should never happen! */
    return kEipStatusError;
  }

  if (SetQosOnSocket(new_socket, GetPriorityForSocket(qos_for_socket) ) <= 0) { /* got error */
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    OPENER_TRACE_ERR(
      "networkhandler: error on set QoS on socket on new socket: %d - %s\n",
      error_code,
      error_message);
    FreeErrorMessage(error_message);
  }

  OPENER_TRACE_INFO("networkhandler: UDP socket %d\n", new_socket);

  /* check if it is sending or receiving */
  if (communication_direction == kUdpCommuncationDirectionConsuming) {
    int option_value = 1;
    if (setsockopt( new_socket, SOL_SOCKET, SO_REUSEADDR,
                    (char *) &option_value,
                    sizeof(option_value) ) == -1) {
      OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on consuming udp socket\n");
      CloseSocket(new_socket);
      return kEipStatusError;
    }

    /* bind is only for consuming necessary */
    if ( ( bind( new_socket, (struct sockaddr *) socket_data,
                 sizeof(struct sockaddr) ) ) == -1 ) {
      int error_code = GetSocketErrorNumber();
      char *error_message = GetErrorMessage(error_code);
      OPENER_TRACE_ERR("error on bind udp: %d - %s\n", error_code,
                       error_message);
      FreeErrorMessage(error_message);
      CloseSocket(new_socket);
      return new_socket;
    }

    OPENER_TRACE_INFO("networkhandler: bind UDP socket %d\n", new_socket);
  } else { /* we have a producing udp socket */

    if (socket_data->sin_addr.s_addr
        == g_multicast_configuration.starting_multicast_address) {
      if (1 != g_time_to_live_value) { /* we need to set a TTL value for the socket */
        if ( setsockopt(new_socket, IPPROTO_IP, IP_MULTICAST_TTL,
                        &g_time_to_live_value,
                        sizeof(g_time_to_live_value) ) < 0 ) {
          int error_code = GetSocketErrorNumber();
          char *error_message = GetErrorMessage(error_code);
          OPENER_TRACE_ERR(
            "networkhandler: could not set the TTL to: %d, error: %d - %s\n",
            g_time_to_live_value, error_code, error_message);
          FreeErrorMessage(error_message);
          return new_socket;
        }
      }
    }
  }

  if ( (communication_direction == kUdpCommuncationDirectionConsuming)
       || (0 == socket_data->sin_addr.s_addr) ) {
    /* we have a peer to peer producer or a consuming connection*/
    if (getpeername(g_current_active_tcp_socket,
                    (struct sockaddr *) &peer_address, &peer_address_length)
        < 0) {
      int error_code = GetSocketErrorNumber();
      char *error_message = GetErrorMessage(error_code);
      OPENER_TRACE_ERR("networkhandler: could not get peername: %d - %s\n",
                       error_code,
                       error_message);
      FreeErrorMessage(error_message);
      return new_socket;
    }
    /* store the originators address */
    socket_data->sin_addr.s_addr = peer_address.sin_addr.s_addr;
  }

  /* add new socket to the master list                                             */
  FD_SET(new_socket, &master_socket);
  if (new_socket > highest_socket_handle) {
    OPENER_TRACE_INFO("New highest socket: %d\n", new_socket);
    highest_socket_handle = new_socket;
  }
  return new_socket;
}

void CheckAndHandleConsumingUdpSockets(void) {
  DoublyLinkedListNode *iterator = connection_list.first;

  CipConnectionObject *current_connection_object = NULL;

  /* see a message on one of the registered UDP sockets has been received     */
  while (NULL != iterator) {
    current_connection_object = (CipConnectionObject *)iterator->data;
    iterator = iterator->next; /* do this at the beginning as the close function may can make the entry invalid */

    if ( (-1
          != current_connection_object->socket[
            kUdpCommuncationDirectionConsuming])
         && ( true
              == CheckSocketSet(
                current_connection_object->socket[
                  kUdpCommuncationDirectionConsuming]) ) ) {
      struct sockaddr_in from_address = {0};
      socklen_t from_address_length = sizeof(from_address);
      int received_size = recvfrom(
        current_connection_object->socket[kUdpCommuncationDirectionConsuming],
        g_ethernet_communication_buffer, PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
        (struct sockaddr *) &from_address, &from_address_length);
      if (0 == received_size) {
        int error_code = GetSocketErrorNumber();
        char *error_message = GetErrorMessage(error_code);
        OPENER_TRACE_ERR(
          "networkhandler: socket: %d - connection closed by client: %d - %s\n",
          current_connection_object->socket[
            kUdpCommuncationDirectionConsuming],
          error_code,
          error_message);
        FreeErrorMessage(error_message);
        current_connection_object->connection_close_function(
          current_connection_object);
        continue;
      }


      if (0 > received_size) {
        int error_code = GetSocketErrorNumber();
        char *error_message = GetErrorMessage(error_code);
        if (OPENER_SOCKET_WOULD_BLOCK == error_code) {
          return;       // No fatal error, resume execution
        }
        OPENER_TRACE_ERR("networkhandler: error on recv: %d - %s\n",
                         error_code,
                         error_message);
        FreeErrorMessage(error_message);
        current_connection_object->connection_close_function(
          current_connection_object);
        continue;
      }

      HandleReceivedConnectedData(g_ethernet_communication_buffer,
                                  received_size, &from_address);

    }
  }
}


void CloseSocket(const int socket_handle) {
  OPENER_TRACE_INFO("networkhandler: closing socket %d\n", socket_handle);

  if (kEipInvalidSocket != socket_handle) {
    FD_CLR(socket_handle, &master_socket);
    CloseSocketPlatform(socket_handle);
  }
  OPENER_TRACE_INFO("networkhandler: closing socket done %d\n", socket_handle);
}

int GetMaxSocket(int socket1,
                 int socket2,
                 int socket3,
                 int socket4) {
  if ( (socket1 > socket2) && (socket1 > socket3) && (socket1 > socket4) ) {
    return socket1;
  }

  if ( (socket2 > socket1) && (socket2 > socket3) && (socket2 > socket4) ) {
    return socket2;
  }

  if ( (socket3 > socket1) && (socket3 > socket2) && (socket3 > socket4) ) {
    return socket3;
  }

  return socket4;
}

void CheckEncapsulationInactivity(int socket_handle) {
  if (0 < g_encapsulation_inactivity_timeout) { //*< Encapsulation inactivity timeout is enabled
    SocketTimer *socket_timer = SocketTimerArrayGetSocketTimer(
      g_timestamps,
      OPENER_NUMBER_OF_SUPPORTED_SESSIONS,
      socket_handle);

//    OPENER_TRACE_INFO("Check socket %d - socket timer: %p\n",
//                      socket_handle,
//                      socket_timer);
    if(NULL != socket_timer) {
      MilliSeconds diff_milliseconds = g_actual_time - SocketTimerGetLastUpdate(
        socket_timer);

      if ( diff_milliseconds >=
           (MilliSeconds) (1000UL * g_encapsulation_inactivity_timeout) ) {

        size_t encapsulation_session_handle =
          GetSessionFromSocket(socket_handle);

        CloseClass3ConnectionBasedOnSession(encapsulation_session_handle);

        CloseTcpSocket(socket_handle);
        RemoveSession(socket_handle);
      }
    }
  }
}
