/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#include "networkhandler.h"

#include "opener_api.h"
#include "encap.h"
#include "cipconnectionmanager.h"
#include "endianconv.h"
#include "trace.h"
#include "ciptcpipinterface.h"

/* values needed from the connection manager */
extern ConnectionObject *g_active_connection_list;

EipUint8 g_ethernet_communication_buffer[PC_OPENER_ETHERNET_BUFFER_SIZE]; /**< communication buffer */

#define MAX_NO_OF_TCP_SOCKETS 10

typedef unsigned long MilliSeconds;
typedef unsigned long long MicroSeconds;

fd_set master_socket;
fd_set read_socket;
/* temporary file descriptor for select() */

int highest_socket_handle;

/** @brief This variable holds the TCP socket the received to last explicit message.
 * It is needed for opening point to point connection to determine the peer's
 * address.
 */
int g_current_active_tcp_socket;

static struct timeval g_time_value;
static MilliSeconds g_actual_time;
static MilliSeconds g_last_time;

/** @brief handle any connection request coming in the TCP server socket.
 *
 */
void CheckAndHandleTcpListenerSocket(void);

/** @brief check if data has been received on the UDP broadcast socket and if yes handle it correctly
 *
 */
void CheckAndHandleUdpLocalBroadcastSocket(void);

void CheckAndHandleUdpUnicastSocket(void);

void CheckAndHandleUdpGlobalBroadcastSocket(void);

/** @brief check if on one of the UDP consuming sockets data has been received and if yes handle it correctly
 *
 */
void CheckAndHandleConsumingUdpSockets(void);

/** @brief check if the given socket is set in the read set
 * @param socket The socket to check
 * @return
 */
EipBool8 CheckSocketSet(int socket);

/** @brief
 *
 */
EipStatus HandleDataOnTcpSocket(int socket);

int GetMaxSocket(int socket1, int socket2, int socket3, int socket4);

static MicroSeconds GetMicroSeconds(void) {
#ifdef WIN32
  LARGE_INTEGER lPerformanceCouner;
  LARGE_INTEGER lPerformanceFrequency;

  QueryPerformanceCounter(&lPerformanceCouner);
  QueryPerformanceFrequency(&lPerformanceFrequency);

  return (MicroSeconds) (lPerformanceCouner.QuadPart * 1000000LL / lPerformanceFrequency.QuadPart);
#else
  struct timeval time_value;
  gettimeofday(&time_value, 0);
  return (MicroSeconds) time_value.tv_sec * 1000000ULL
      + (MicroSeconds) time_value.tv_usec;
#endif
}

static MilliSeconds GetMilliSeconds(void) {
  return (MilliSeconds) (GetMicroSeconds() / 1000ULL);
}

typedef struct {
  int tcp_listener;
  int udp_unicast_listener;
  int udp_local_broadcast_listener;
  int udp_global_broadcast_listener;
  MilliSeconds elapsed_time;
} NetworkStatus;

NetworkStatus g_network_status;

EipStatus NetworkHandlerInitialize(void) {

  /* clear the master an temp sets */
  FD_ZERO(&master_socket);
  FD_ZERO(&read_socket);

  /* create a new TCP socket */
  if ((g_network_status.tcp_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
      == -1) {
    OPENER_TRACE_ERR("error allocating socket stream listener, %d\n", errno);
    return kEipStatusError;
  }

  int set_socket_option_value = 1;  //Represents true for used set socket options
  /* Activates address reuse */
  if (setsockopt(g_network_status.tcp_listener, SOL_SOCKET, SO_REUSEADDR,
                 (char *) &set_socket_option_value,
                 sizeof(set_socket_option_value)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on tcp_listener\n");
    return kEipStatusError;
  }

  /* create a new UDP socket */
  if ((g_network_status.udp_global_broadcast_listener = socket(AF_INET,
                                                               SOCK_DGRAM,
                                                               IPPROTO_UDP))
      == -1) {
    OPENER_TRACE_ERR("error allocating UDP broadcast listener socket, %d\n",
                     errno);
    return kEipStatusError;
  }

  /* create a new UDP socket */
  if ((g_network_status.udp_local_broadcast_listener = socket(AF_INET,
                                                              SOCK_DGRAM,
                                                              IPPROTO_UDP))
      == -1) {
    OPENER_TRACE_ERR("error allocating UDP broadcast listener socket, %d\n",
                     errno);
    return kEipStatusError;
  }

  /* create a new UDP socket */
  if ((g_network_status.udp_unicast_listener = socket(AF_INET, SOCK_DGRAM,
                                                      IPPROTO_UDP)) == -1) {
    OPENER_TRACE_ERR("error allocating UDP unicast listener socket, %d\n",
                     errno);
    return kEipStatusError;
  }

  /* Activates address reuse */
  if (setsockopt(g_network_status.udp_global_broadcast_listener, SOL_SOCKET,
                 SO_REUSEADDR, (char *) &set_socket_option_value,
                 sizeof(set_socket_option_value)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on udp_broadcast_listener\n");
    return kEipStatusError;
  }

  /* Activates address reuse */
  if (setsockopt(g_network_status.udp_local_broadcast_listener, SOL_SOCKET,
                 SO_REUSEADDR, (char *) &set_socket_option_value,
                 sizeof(set_socket_option_value)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on udp_broadcast_listener\n");
    return kEipStatusError;
  }

  /* Activates address reuse */
  if (setsockopt(g_network_status.udp_unicast_listener, SOL_SOCKET,
                 SO_REUSEADDR, (char *) &set_socket_option_value,
                 sizeof(set_socket_option_value)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on udp_unicast_listener\n");
    return kEipStatusError;
  }

  struct sockaddr_in my_address = { .sin_family = AF_INET, .sin_port = htons(
      kOpenerEthernetPort), .sin_addr.s_addr = interface_configuration_
      .ip_address };

  /* bind the new socket to port 0xAF12 (CIP) */
  if ((bind(g_network_status.tcp_listener, (struct sockaddr *) &my_address,
            sizeof(struct sockaddr))) == -1) {
    OPENER_TRACE_ERR("error with bind: %s\n", strerror(errno));
    return kEipStatusError;
  }

  if ((bind(g_network_status.udp_unicast_listener,
            (struct sockaddr *) &my_address, sizeof(struct sockaddr))) == -1) {
    OPENER_TRACE_ERR("error with UDP bind: %s\n", strerror(errno));
    return kEipStatusError;
  }

  struct sockaddr_in local_broadcast_address = { .sin_family = AF_INET,
      .sin_port = htons(kOpenerEthernetPort), .sin_addr.s_addr =
          interface_configuration_.ip_address
              | ~interface_configuration_.network_mask };

  if ((bind(g_network_status.udp_local_broadcast_listener,
            (struct sockaddr *) &local_broadcast_address,
            sizeof(struct sockaddr))) == -1) {
    OPENER_TRACE_ERR("error with UDP bind: %s\n", strerror(errno));
    return kEipStatusError;
  }

  struct sockaddr_in global_broadcast_address = { .sin_family = AF_INET,
      .sin_port = htons(kOpenerEthernetPort), .sin_addr.s_addr= htonl(INADDR_BROADCAST) };

  /* enable the UDP socket to receive broadcast messages */
   if (0
       > setsockopt(g_network_status.udp_global_broadcast_listener, SOL_SOCKET, SO_BROADCAST,
                    (char *) &set_socket_option_value, sizeof(int))) {
     OPENER_TRACE_ERR(
         "error with setting broadcast receive for UDP socket: %s\n",
         strerror(errno));
     return kEipStatusError;
   }

  if ((bind(g_network_status.udp_global_broadcast_listener,
            (struct sockaddr *) &global_broadcast_address,
            sizeof(struct sockaddr))) == -1) {
    OPENER_TRACE_ERR("error with global broadcast UDP bind: %s\n", strerror(errno));
    return kEipStatusError;
  }

  /* switch socket in listen mode */
  if ((listen(g_network_status.tcp_listener, MAX_NO_OF_TCP_SOCKETS)) == -1) {
    OPENER_TRACE_ERR("networkhandler: error with listen: %s\n",
                     strerror(errno));
    return kEipStatusError;
  }

  /* add the listener socket to the master set */
  FD_SET(g_network_status.tcp_listener, &master_socket);
  FD_SET(g_network_status.udp_unicast_listener, &master_socket);
  FD_SET(g_network_status.udp_local_broadcast_listener, &master_socket);
  FD_SET(g_network_status.udp_global_broadcast_listener, &master_socket);

  /* keep track of the biggest file descriptor */
  highest_socket_handle = GetMaxSocket(
      g_network_status.tcp_listener,
      g_network_status.udp_global_broadcast_listener,
      g_network_status.udp_local_broadcast_listener,
      g_network_status.udp_unicast_listener);

  g_last_time = GetMilliSeconds(); /* initialize time keeping */
  g_network_status.elapsed_time = 0;

  return kEipStatusOk;
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

EipStatus SendUdpData(struct sockaddr_in *address, int socket, EipUint8 *data,
                      EipUint16 data_length) {

  int sent_length = sendto(socket, (char *) data, data_length, 0,
                           (struct sockaddr *) address, sizeof(*address));

  if (sent_length < 0) {
    OPENER_TRACE_ERR("networkhandler: error with sendto in sendUDPData: %s\n",
                     strerror(errno));
    return kEipStatusError;
  }

  if (sent_length != data_length) {
    OPENER_TRACE_WARN(
        "data length sent_length mismatch; probably not all data was sent in SendUdpData, sent %d of %d\n",
        sent_length, data_length);
    return kEipStatusError;
  }

  return kEipStatusOk;
}

EipStatus HandleDataOnTcpSocket(int socket) {
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

  if (number_of_read_bytes == 0) {
    OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                     strerror(errno));
    return kEipStatusError;
  }
  if (number_of_read_bytes < 0) {
    OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
    return kEipStatusError;
  }

  EipUint8 *read_buffer = &g_ethernet_communication_buffer[2]; /* at this place EIP stores the data length */
  size_t data_size = GetIntFromMessage(&read_buffer)
      + ENCAPSULATION_HEADER_LENGTH - 4; /* -4 is for the 4 bytes we have already read*/
  /* (NOTE this advances the buffer pointer) */
  if ((PC_OPENER_ETHERNET_BUFFER_SIZE - 4) < data_size) { /*TODO can this be handled in a better way?*/
    OPENER_TRACE_ERR(
        "too large packet received will be ignored, will drop the data\n");
    /* Currently we will drop the whole packet */

    do {
      number_of_read_bytes = recv(socket, &g_ethernet_communication_buffer[0],
                                  data_sent, 0);

      if (number_of_read_bytes == 0) /* got error or connection closed by client */
      {
        OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                         strerror(errno));
        return kEipStatusError;
      }
      if (number_of_read_bytes < 0) {
        OPENER_TRACE_ERR("networkhandler: error on recv: %s\n",
                         strerror(errno));
        return kEipStatusError;
      }
      data_size -= number_of_read_bytes;
      if ((data_size < PC_OPENER_ETHERNET_BUFFER_SIZE) && (data_size != 0)) {
        data_sent = data_size;
      }
    } while (0 != data_size); /* TODO: fragile end statement */
    return kEipStatusOk;
  }

  number_of_read_bytes = recv(socket, &g_ethernet_communication_buffer[4],
                              data_size, 0);

  if (number_of_read_bytes == 0) /* got error or connection closed by client */
  {
    OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                     strerror(errno));
    return kEipStatusError;
  }
  if (number_of_read_bytes < 0) {
    OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
    return kEipStatusError;
  }

  if ((unsigned) number_of_read_bytes == data_size) {
    /*we got the right amount of data */
    data_size += 4;
    /*TODO handle partial packets*/
    OPENER_TRACE_INFO("Data received on tcp:\n");

    g_current_active_tcp_socket = socket;

    number_of_read_bytes = HandleReceivedExplictTcpData(
        socket, g_ethernet_communication_buffer, data_size, &remaining_bytes);

    g_current_active_tcp_socket = -1;

    if (remaining_bytes != 0) {
      OPENER_TRACE_WARN(
          "Warning: received packet was to long: %d Bytes left!\n",
          remaining_bytes);
    }

    if (number_of_read_bytes > 0) {
      OPENER_TRACE_INFO("reply sent:\n");

      data_sent = send(socket, (char *) &g_ethernet_communication_buffer[0],
                       number_of_read_bytes, 0);
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
                    struct sockaddr_in *socket_data) {
  struct sockaddr_in peer_address;
  int new_socket;

  socklen_t peer_address_length;

  peer_address_length = sizeof(struct sockaddr_in);
  /* create a new UDP socket */
  if ((new_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    OPENER_TRACE_ERR("networkhandler: cannot create UDP socket: %s\n",
                     strerror(errno));
    return kEipInvalidSocket;
  }

  OPENER_TRACE_INFO("networkhandler: UDP socket %d\n", new_socket);

  /* check if it is sending or receiving */
  if (communication_direction == kUdpCommuncationDirectionConsuming) {
    int option_value = 1;
    if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &option_value,
                   sizeof(option_value)) == -1) {
      OPENER_TRACE_ERR(
          "error setting socket option SO_REUSEADDR on consuming udp socket\n");
      return kEipStatusError;
    }

    /* bind is only for consuming necessary */
    if ((bind(new_socket, (struct sockaddr *) socket_data,
              sizeof(struct sockaddr))) == -1) {
      OPENER_TRACE_ERR("error on bind udp: %s\n", strerror(errno));
      return kEipInvalidSocket;
    }

    OPENER_TRACE_INFO("networkhandler: bind UDP socket %d\n", new_socket);
  } else { /* we have a producing udp socket */

    if (socket_data->sin_addr.s_addr
        == g_multicast_configuration.starting_multicast_address) {
      if (1 != g_time_to_live_value) { /* we need to set a TTL value for the socket */
        if (setsockopt(new_socket, IPPROTO_IP, IP_MULTICAST_TTL,
                       &g_time_to_live_value,
                       sizeof(g_time_to_live_value) < 0)) {
          OPENER_TRACE_ERR(
              "networkhandler: could not set the TTL to: %d, error: %s\n",
              g_time_to_live_value, strerror(errno));
          return kEipInvalidSocket;
        }
      }
    }
  }

  if ((communication_direction == kUdpCommuncationDirectionConsuming)
      || (0 == socket_data->sin_addr.s_addr)) {
    /* we have a peer to peer producer or a consuming connection*/
    if (getpeername(g_current_active_tcp_socket,
                    (struct sockaddr *) &peer_address, &peer_address_length)
        < 0) {
      OPENER_TRACE_ERR("networkhandler: could not get peername: %s\n",
                       strerror(errno));
      return kEipInvalidSocket;
    }
    /* store the originators address */
    socket_data->sin_addr.s_addr = peer_address.sin_addr.s_addr;
  }

  /* add new socket to the master list                                             */
  FD_SET(new_socket, &master_socket);
  if (new_socket > highest_socket_handle) {
    highest_socket_handle = new_socket;
  }
  return new_socket;
}

void IApp_CloseSocket_udp(int socket_handle) {
  CloseSocket(socket_handle);
}

void IApp_CloseSocket_tcp(int socket_handle) {
  CloseSocket(socket_handle);
}

void CloseSocket(int socket_handle) {

  OPENER_TRACE_INFO("networkhandler: closing socket %d\n", socket_handle);
  if (kEipInvalidSocket != socket_handle) {
    FD_CLR(socket_handle, &master_socket);
#ifdef WIN32
    closesocket(socket_handle);
#else
    shutdown(socket_handle, SHUT_RDWR);
    close(socket_handle);
#endif
  }
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

void CheckAndHandleUdpLocalBroadcastSocket(void) {

  struct sockaddr_in from_address;
#ifndef WIN32
  socklen_t from_address_length;
#else
  unsigned long from_address_length;
#endif

  /* see if this is an unsolicited inbound UDP message */
  if (true == CheckSocketSet(g_network_status.udp_local_broadcast_listener)) {

    from_address_length = sizeof(from_address);

    OPENER_TRACE_STATE(
        "networkhandler: unsolicited UDP message on EIP broadcast socket\n");

    /* Handle UDP broadcast messages */
    int received_size = recvfrom(g_network_status.udp_local_broadcast_listener,
                                 g_ethernet_communication_buffer,
                                 PC_OPENER_ETHERNET_BUFFER_SIZE,
                                 0, (struct sockaddr *) &from_address,
                                 &from_address_length);

    if (received_size <= 0) { /* got error */
      OPENER_TRACE_ERR(
          "networkhandler: error on recvfrom UDP broadcast port: %s\n",
          strerror(errno));
      return;
    }

    OPENER_TRACE_INFO("Data received on UDP:\n");

    EipUint8 *receive_buffer = &g_ethernet_communication_buffer[0];
    int remaining_bytes = 0;
    do {
      int reply_length = HandleReceivedExplictUdpData(
          g_network_status.udp_local_broadcast_listener, &from_address,
          receive_buffer, received_size, &remaining_bytes, false);

      receive_buffer += received_size - remaining_bytes;
      received_size = remaining_bytes;

      if (reply_length > 0) {
        OPENER_TRACE_INFO("reply sent:\n");

        /* if the active socket matches a registered UDP callback, handle a UDP packet */
        if (sendto(g_network_status.udp_local_broadcast_listener,
                   (char *) g_ethernet_communication_buffer, reply_length, 0,
                   (struct sockaddr *) &from_address, sizeof(from_address))
            != reply_length) {
          OPENER_TRACE_INFO(
              "networkhandler: UDP response was not fully sent\n");
        }
      }
    } while (remaining_bytes > 0);
  }
}

void CheckAndHandleUdpGlobalBroadcastSocket(void) {

  struct sockaddr_in from_address;
#ifndef WIN32
  socklen_t from_address_length;
#else
  unsigned long from_address_length;
#endif

  /* see if this is an unsolicited inbound UDP message */
  if (true == CheckSocketSet(g_network_status.udp_global_broadcast_listener)) {

    from_address_length = sizeof(from_address);

    OPENER_TRACE_STATE(
        "networkhandler: unsolicited UDP message on EIP global broadcast socket\n");

    /* Handle UDP broadcast messages */
    int received_size = recvfrom(g_network_status.udp_global_broadcast_listener,
                                 g_ethernet_communication_buffer,
                                 PC_OPENER_ETHERNET_BUFFER_SIZE,
                                 0, (struct sockaddr *) &from_address,
                                 &from_address_length);

    if (received_size <= 0) { /* got error */
      OPENER_TRACE_ERR(
          "networkhandler: error on recvfrom UDP global broadcast port: %s\n",
          strerror(errno));
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
        OPENER_TRACE_INFO("reply sent:\n");

        /* if the active socket matches a registered UDP callback, handle a UDP packet */
        if (sendto(g_network_status.udp_global_broadcast_listener,
                   (char *) g_ethernet_communication_buffer, reply_length, 0,
                   (struct sockaddr *) &from_address, sizeof(from_address))
            != reply_length) {
          OPENER_TRACE_INFO(
              "networkhandler: UDP response was not fully sent\n");
        }
      }
    } while (remaining_bytes > 0);
  }
}

void CheckAndHandleUdpUnicastSocket(void) {

  struct sockaddr_in from_address;
#ifndef WIN32
  socklen_t from_address_length;
#else
  unsigned long from_address_length;
#endif

  /* see if this is an unsolicited inbound UDP message */
  if (true == CheckSocketSet(g_network_status.udp_unicast_listener)) {

    from_address_length = sizeof(from_address);

    OPENER_TRACE_STATE(
        "networkhandler: unsolicited UDP message on EIP unicast socket\n");

    /* Handle UDP broadcast messages */
    int received_size = recvfrom(g_network_status.udp_unicast_listener,
                                 g_ethernet_communication_buffer,
                                 PC_OPENER_ETHERNET_BUFFER_SIZE,
                                 0, (struct sockaddr *) &from_address,
                                 &from_address_length);

    if (received_size <= 0) { /* got error */
      OPENER_TRACE_ERR(
          "networkhandler: error on recvfrom UDP unicast port: %s\n",
          strerror(errno));
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
        OPENER_TRACE_INFO("reply sent:\n");

        /* if the active socket matches a registered UDP callback, handle a UDP packet */
        if (sendto(g_network_status.udp_unicast_listener,
                   (char *) g_ethernet_communication_buffer, reply_length, 0,
                   (struct sockaddr *) &from_address, sizeof(from_address))
            != reply_length) {
          OPENER_TRACE_INFO(
              "networkhandler: UDP unicast response was not fully sent\n");
        }
      }
    } while (remaining_bytes > 0);
  }
}

void CheckAndHandleConsumingUdpSockets(void) {
  struct sockaddr_in from_address;
#ifndef WIN32
  socklen_t from_address_length;
#else
  unsigned long from_address_length;
#endif

  ConnectionObject *connection_object_iterator = g_active_connection_list;
  ConnectionObject *current_connection_object = NULL;

  /* see a message on one of the registered UDP sockets has been received     */
  while (NULL != connection_object_iterator) {
    current_connection_object = connection_object_iterator;
    connection_object_iterator = connection_object_iterator
        ->next_connection_object; /* do this at the beginning as the close function may can make the entry invalid */

    if ((-1
        != current_connection_object->socket[kUdpCommuncationDirectionConsuming])
        && (true
            == CheckSocketSet(
                current_connection_object->socket[kUdpCommuncationDirectionConsuming]))) {
      from_address_length = sizeof(from_address);
      int received_size = recvfrom(
          current_connection_object->socket[kUdpCommuncationDirectionConsuming],
          g_ethernet_communication_buffer, PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
          (struct sockaddr *) &from_address, &from_address_length);
      if (0 == received_size) {
        OPENER_TRACE_STATE("connection closed by client\n");
        current_connection_object->connection_close_function(
            current_connection_object);
        continue;
      }

      if (0 > received_size) {
        OPENER_TRACE_ERR("networkhandler: error on recv: %s\n",
                         strerror(errno));
        current_connection_object->connection_close_function(
            current_connection_object);
        continue;
      }

      HandleReceivedConnectedData(g_ethernet_communication_buffer,
                                  received_size, &from_address);

    }
  }
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
