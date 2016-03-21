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

#include "encap.h"
#include "cipconnectionmanager.h"
#include "endianconv.h"
#include "trace.h"
#include "ciptcpipinterface.h"

fd_set master;
fd_set read_fds;
/* temporary file descriptor for select() */

int fdmax;

/*!< This var holds the TCP socket the received to last explicit message.
 * It is needed for opening point to point connection to determine the peer's
 * address.
 */
int g_current_active_tcp_socket;

static struct timeval time_value;
static MilliSeconds actual_time;
static MilliSeconds last_time;

/*!\brief handle any connection request coming in the TCP server socket.
 *
 */
void
CheckAndHandleTcpListenerSocket();

/*! \brief check if data has been received on the udp broadcast socket and if yes handle it correctly
 *
 */
void
CheckAndHandleUdpBroadCastSocket();

void
CheckAndHandleUdpUnicastSocket();

/*! \brief check if on one of the udp consuming sockets data has been received and if yes handle it correctly
 *
 */
void
CheckAndHandleConsumingUdpSockets();
/*! \brief check if the given socket is set in the read set
 *
 */
CipBool
checkSocketSet(int pa_nSocket);

/*!
 *
 */
EipStatus
handleDataOnTCPSocket(int pa_nSocket);

static MicroSeconds getMicroSeconds() {
  LARGE_INTEGER performance_counter;
  LARGE_INTEGER lPerformanceFrequency;

  QueryPerformanceCounter(&performance_counter);
  QueryPerformanceFrequency(&lPerformanceFrequency);

  return (MicroSeconds) (performance_counter.QuadPart * 1000000LL
      / lPerformanceFrequency.QuadPart);
}

static MilliSeconds GetMilliSeconds(void) {
  return (MilliSeconds) (getMicroSeconds() / 1000ULL);
}

/* INT8 Start_NetworkHandler()
 * 	start a TCP listening socket, accept connections, receive data in select loop, call manageConnections periodically.
 * 	return status
 * 			-1 .. error
 */

typedef struct {
  int tcp_listener;
  int udp_unicast_listener;
  int udp_broadcast_listener;
  MilliSeconds elapsed_time;
} NetworkStatus;

NetworkStatus g_network_status;

EipStatus NetworkHandlerInitialize(void) {
#ifdef WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  wVersionRequested = MAKEWORD(2, 2);
  WSAStartup(wVersionRequested, &wsaData);
#endif

  /* clear the master an temp sets                                            */
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  /* create a new TCP socket */
  if ((g_network_status.tcp_listener = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    OPENER_TRACE_ERR("error allocating socket stream listener, %d\n", errno);
    return kEipStatusError;
  }

  int nOptVal = 1;
  if (setsockopt(g_network_status.tcp_listener, SOL_SOCKET, SO_REUSEADDR,
                 (char *) &nOptVal, sizeof(nOptVal)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on nTCPListener\n");
    return kEipStatusError;
  }

  /* create a new UDP unicast socket */
  if ((g_network_status.udp_unicast_listener = socket(PF_INET, SOCK_DGRAM, 0))
      == -1) {
    OPENER_TRACE_ERR("error allocating udp listener socket, %d\n", errno);
    return kEipStatusError;
  }

  if (setsockopt(g_network_status.udp_unicast_listener, SOL_SOCKET,
                 SO_REUSEADDR, (char *) &nOptVal, sizeof(nOptVal)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on nUDPListener\n");
    return kEipStatusError;
  }

  /* create a new UDP broadcast socket */
  if ((g_network_status.udp_broadcast_listener = socket(PF_INET, SOCK_DGRAM, 0))
      == -1) {
    OPENER_TRACE_ERR("error allocating udp listener socket, %d\n", errno);
    return kEipStatusError;
  }

  if (setsockopt(g_network_status.udp_broadcast_listener, SOL_SOCKET,
                 SO_REUSEADDR, (char *) &nOptVal, sizeof(nOptVal)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on nUDPListener\n");
    return kEipStatusError;
  }

  /* set up unicast udp socket */
  struct sockaddr_in unicast_address = { .sin_family = AF_INET, .sin_port =
      htons(kOpenerEthernetPort), .sin_addr.s_addr = interface_configuration_
      .ip_address };

  if ((bind(g_network_status.udp_unicast_listener,
            (struct sockaddr *) &unicast_address, sizeof(struct sockaddr)))
      == -1) {
    int error_code = WSAGetLastError();
    OPENER_TRACE_ERR("error with udp bind: %d, %s\n", error_code,
                     strerror(error_code));
    return kEipStatusError;
  }

  struct sockaddr_in my_address = { .sin_family = AF_INET, .sin_port = htons(
      kOpenerEthernetPort), .sin_addr.s_addr = htonl(INADDR_ANY) };

  /* bind the new socket to port 0xAF12 (CIP) */
  if ((bind(g_network_status.tcp_listener, (struct sockaddr *) &my_address,
            sizeof(struct sockaddr))) == -1) {
    OPENER_TRACE_ERR("error with bind: %s\n", strerror(errno));
    return kEipStatusError;
  }

  /* enable the udp socket to receive broadcast messages*/
  int y = 1;
  if (0
      > setsockopt(g_network_status.udp_broadcast_listener, SOL_SOCKET,
                   SO_BROADCAST, (char *) &y, sizeof(int))) {
    OPENER_TRACE_ERR(
        "error with setting broadcast receive for udp socket: %s\n",
        strerror(errno));
    return kEipStatusError;
  }

  if ((bind(g_network_status.udp_broadcast_listener,
            (struct sockaddr *) &my_address, sizeof(struct sockaddr))) == -1) {
    OPENER_TRACE_ERR("error with udp bind: %s\n", strerror(errno));
    return kEipStatusError;
  }

  /* switch socket in listen mode */
  if ((listen(g_network_status.tcp_listener, MAX_NO_OF_TCP_SOCKETS)) == -1) {
    OPENER_TRACE_ERR("networkhandler: error with listen: %s\n",
                     strerror(errno));
    return kEipStatusError;
  }

  /* add the listener socket to the master set */
  FD_SET(g_network_status.tcp_listener, &master);
  FD_SET(g_network_status.udp_broadcast_listener, &master);
  FD_SET(g_network_status.udp_unicast_listener, &master);

  /* keep track of the biggest file descriptor */
  fdmax = GetMaxSocket(g_network_status.tcp_listener,
                       g_network_status.udp_unicast_listener,
                       g_network_status.udp_broadcast_listener, -1);
  last_time = GetMilliSeconds(); /* initialize time keeping */
  g_network_status.elapsed_time = 0;

  return kEipStatusOk;
}

EipStatus NetworkHandlerProcessOnce(void) {
  int fd;
  int res;

  read_fds = master;

  time_value.tv_sec = 0;
  time_value.tv_usec = (
      g_network_status.elapsed_time < kOpenerTimerTickInMilliSeconds ?
          kOpenerTimerTickInMilliSeconds - g_network_status.elapsed_time : 0)
      * 1000; /* 10 ms */

  res = select(fdmax + 1, &read_fds, 0, 0, &time_value);

  if (res == -1) {
    if (EINTR == errno) /* we have somehow been interrupted. The default behavior is to go back into the select loop. */
    {
      return kEipStatusOk;
    } else {
      OPENER_TRACE_ERR("networkhandler: error with select: %s\n",
                       strerror(errno));
      return kEipStatusError;
    }
  }

  if (res > 0) {

    CheckAndHandleTcpListenerSocket();
    CheckAndHandleUdpBroadCastSocket();
    CheckAndHandleUdpUnicastSocket();
    CheckAndHandleConsumingUdpSockets();

    for (fd = 0; fd <= fdmax; fd++) {
      if (true == checkSocketSet(fd)) {
        /* if it is still checked it is a TCP receive */
        if (kEipStatusError == handleDataOnTCPSocket(fd)) /* if error */
        {
          CloseSocket(fd);
          CloseSession(fd); /* clean up session and close the socket */
        }
      }
    }
  }

  actual_time = GetMilliSeconds();
  g_network_status.elapsed_time += actual_time - last_time;
  last_time = actual_time;

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
  CloseSocket(g_network_status.udp_broadcast_listener);
  return kEipStatusOk;
}

CipBool checkSocketSet(int pa_nSocket) {
  CipBool nRetVal = false;
  if (FD_ISSET(pa_nSocket, &read_fds)) {
    if (FD_ISSET(pa_nSocket, &master)) {
      nRetVal = true;
    } else {
      OPENER_TRACE_INFO("socket: %d closed with pending message\n", pa_nSocket);
    }
    FD_CLR(pa_nSocket, &read_fds);
    /* remove it from the read set so that later checks will not find it */
  }

  return nRetVal;
}

EipStatus SendUdpData(struct sockaddr_in *pa_pstAddr, int pa_nSockFd,
                      EipUint8*pa_acData, EipUint16 pa_nDataLength) {
  int sentlength;

  sentlength = sendto(pa_nSockFd, (char *) pa_acData, pa_nDataLength, 0,
                      (struct sockaddr *) pa_pstAddr, sizeof(*pa_pstAddr));

  if (sentlength < 0) {
    OPENER_TRACE_ERR("networkhandler: error with sendto in sendUDPData: %s\n",
                     strerror(errno));
    return kEipStatusError;
  } else if (sentlength != pa_nDataLength) {
    OPENER_TRACE_WARN("not all data was sent in sendUDPData, sent %d of %d\n",
                      sentlength, pa_nDataLength);
    return kEipStatusError;
  } else
    return kEipStatusError;
}

EipStatus handleDataOnTCPSocket(int pa_nSocket) {
  EipUint8 *rxp;
  long nCheckVal;
  size_t unDataSize;
  long nDataSent;
  int nRemainingBytes = 0;

  /* We will handle just one EIP packet here the rest is done by the select
   * method which will inform us if more data is available in the socket
   because of the current implementation of the main loop this may not be
   the fastest way and a loop here with a non blocking socket would better
   fit*/

  /*Check how many data is here -- read the first four bytes from the connection */
  nCheckVal = recv(pa_nSocket, g_ethernet_communciation_buffer, 4, 0); /*TODO we may have to set the socket to a non blocking socket */

  if (nCheckVal == 0) {
    OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                     strerror(errno));
    return kEipStatusError;
  }
  if (nCheckVal < 0) {
    OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
    return kEipStatusError;
  }

  rxp = &g_ethernet_communciation_buffer[2]; /* at this place EIP stores the data length */
  unDataSize = GetIntFromMessage(&rxp) + ENCAPSULATION_HEADER_LENGTH - 4; /* -4 is for the 4 bytes we have already read*/
  /* (NOTE this advances the buffer pointer) */
  if (PC_OPENER_ETHERNET_BUFFER_SIZE - 4 < unDataSize) { /*TODO can this be handled in a better way?*/
    OPENER_TRACE_ERR(
        "too large packet received will be ignored, will drop the data\n");
    /* Currently we will drop the whole packet */
    nDataSent = PC_OPENER_ETHERNET_BUFFER_SIZE;

    do {
      nCheckVal = recv(pa_nSocket, g_ethernet_communciation_buffer, nDataSent,
                       0);

      if (nCheckVal == 0) /* got error or connection closed by client */
      {
        OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                         strerror(errno));
        return kEipStatusError;
      }
      if (nCheckVal < 0) {
        OPENER_TRACE_ERR("networkhandler: error on recv: %s\n",
                         strerror(errno));
        return kEipStatusError;
      }
      unDataSize -= nCheckVal;
      if ((unDataSize < PC_OPENER_ETHERNET_BUFFER_SIZE) && (unDataSize != 0)) {
        nDataSent = unDataSize;
      }
    } while (0 != unDataSize); /*TODO fragile end statement */
    return kEipStatusOk;
  }

  nCheckVal = recv(pa_nSocket, &g_ethernet_communciation_buffer[4], unDataSize,
                   0);

  if (nCheckVal == 0) /* got error or connection closed by client */
  {
    OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                     strerror(errno));
    return kEipStatusError;
  }
  if (nCheckVal < 0) {
    OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
    return kEipStatusError;
  }

  if ((unsigned) nCheckVal == unDataSize) {
    /*we got the right amount of data */
    unDataSize += 4;
    /*TODO handle partial packets*/
    OPENER_TRACE_INFO("Data received on tcp:\n");

    g_current_active_tcp_socket = pa_nSocket;

    nCheckVal = HandleReceivedExplictTcpData(pa_nSocket,
                                             g_ethernet_communciation_buffer,
                                             unDataSize, &nRemainingBytes);

    g_current_active_tcp_socket = -1;

    if (nRemainingBytes != 0) {
      OPENER_TRACE_WARN(
          "Warning: received packet was to long: %d Bytes left!\n",
          nRemainingBytes);
    }

    if (nCheckVal > 0) {
      OPENER_TRACE_INFO("reply sent:\n");

      nDataSent = send(pa_nSocket, (char *) g_ethernet_communciation_buffer,
                       nCheckVal, 0);
      if (nDataSent != nCheckVal) {
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

/* create a new UDP socket for the connection manager
 returns the fd if successful, else -1 */
int CreateUdpSocket(int pa_nDirection, struct sockaddr_in *pa_pstAddr) {
  struct sockaddr_in stPeerAdr;
  int newfd;
#ifdef WIN32
  unsigned long nPeerAddrLen;
#else
  socklen_t nPeerAddrLen;
#endif

  nPeerAddrLen = sizeof(struct sockaddr_in);
  /* create a new UDP socket */
  if ((newfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    OPENER_TRACE_ERR("networkhandler: cannot create UDP socket: %s\n",
                     strerror(errno));
    return kEipInvalidSocket;
  }

  OPENER_TRACE_INFO("networkhandler: UDP socket %d\n", newfd);

  /* check if it is sending or receiving */
  if (pa_nDirection == kUdpCommuncationDirectionConsuming) {
    int nOptVal = 1;
    if (setsockopt(newfd, SOL_SOCKET, SO_REUSEADDR, (char *) &nOptVal,
                   sizeof(nOptVal)) == -1) {
      OPENER_TRACE_ERR(
          "error setting socket option SO_REUSEADDR on consuming udp socket\n");
      return kEipStatusError;
    }

    /* bind is only for consuming necessary */
    if ((bind(newfd, (struct sockaddr *) pa_pstAddr, sizeof(struct sockaddr)))
        == -1) {
      OPENER_TRACE_ERR("error on bind udp: %s\n", strerror(errno));
      return kEipInvalidSocket;
    }

    OPENER_TRACE_INFO("networkhandler: bind UDP socket %d\n", newfd);
  } else { /* we have a producing udp socket */

    if (pa_pstAddr->sin_addr.s_addr
        == g_multicast_configuration.starting_multicast_address) {
      if (1 != g_time_to_live_value) { /* we need to set a TTL value for the socket */
        if (setsockopt(newfd, IPPROTO_IP, IP_MULTICAST_TTL,
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

  if ((pa_nDirection == kUdpCommuncationDirectionConsuming)
      || (0 == pa_pstAddr->sin_addr.s_addr)) {
    /* we have a peer to peer producer or a consuming connection*/
    if (getpeername(g_current_active_tcp_socket, (struct sockaddr *) &stPeerAdr,
                    &nPeerAddrLen) < 0) {
      OPENER_TRACE_ERR("networkhandler: could not get peername: %s\n",
                       strerror(errno));
      return kEipInvalidSocket;
    }
    /* store the originators address */
    pa_pstAddr->sin_addr.s_addr = stPeerAdr.sin_addr.s_addr;
  }

  /* add new fd to the master list                                             */
  FD_SET(newfd, &master);
  if (newfd > fdmax) {
    fdmax = newfd;
  }
  return newfd;
}

void IApp_CloseSocket_udp(int pa_nSockFd) {
  CloseSocket(pa_nSockFd);
}

void IApp_CloseSocket_tcp(int pa_nSockFd) {
  CloseSocket(pa_nSockFd);
}

void CloseSocket(int pa_nSockFd) {

  OPENER_TRACE_INFO("networkhandler: closing socket %d\n", pa_nSockFd);
  if (kEipInvalidSocket != pa_nSockFd) {
    FD_CLR(pa_nSockFd, &master);
#ifdef WIN32
    closesocket(pa_nSockFd);
#else
    shutdown(pa_nSockFd, SHUT_RDWR);
    close(pa_nSockFd);
#endif
  }
}

void CheckAndHandleTcpListenerSocket() {
  int newfd;
  /* see if this is a connection request to the TCP listener*/
  if (true == checkSocketSet(g_network_status.tcp_listener)) {
    OPENER_TRACE_INFO("networkhandler: new TCP connection\n");

    newfd = accept(g_network_status.tcp_listener, NULL, NULL);
    if (newfd == -1) {
      OPENER_TRACE_ERR("networkhandler: error on accept: %s\n",
                       strerror(errno));
      return;
    }

    FD_SET(newfd, &master);
    /* add newfd to master set */
    if (newfd > fdmax) {
      fdmax = newfd;
    }

    OPENER_TRACE_STATE("networkhandler: opened new TCP connection on fd %d\n",
                       newfd);
  }
}

void CheckAndHandleUdpBroadCastSocket() {
  int nReceived_size;
  int nRemainingBytes;
  int nReplyLen;
  EipUint8 *rxp;
  struct sockaddr_in stFrom;
#ifndef WIN32
  socklen_t nFromLen;
#else
  unsigned long nFromLen;
#endif

  /* see if this is an unsolicited inbound UDP message */
  if (true == checkSocketSet(g_network_status.udp_broadcast_listener)) {

    nFromLen = sizeof(stFrom);

    OPENER_TRACE_STATE(
        "networkhandler: unsolicited UDP message on EIP broadcast socket\n");

    /*Handle udp broadcast messages */
    nReceived_size = recvfrom(g_network_status.udp_broadcast_listener,
                              g_ethernet_communciation_buffer,
                              PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
                              (struct sockaddr *) &stFrom, &nFromLen);

    if (nReceived_size <= 0) { /* got error */
      OPENER_TRACE_ERR(
          "networkhandler: error on recvfrom udp broadcast port: %s\n",
          strerror(errno));
      return;
    }

    OPENER_TRACE_INFO("Data received on udp:\n");

    rxp = &g_ethernet_communciation_buffer[0];
    do {
      nReplyLen = HandleReceivedExplictUdpData(
          g_network_status.udp_broadcast_listener, &stFrom, rxp, nReceived_size,
          &nRemainingBytes, false);

      rxp += nReceived_size - nRemainingBytes;
      nReceived_size = nRemainingBytes;

      if (nReplyLen > 0) {
        OPENER_TRACE_INFO("reply sent:\n");

        /* if the active fd matches a registered UDP callback, handle a UDP packet */
        if (sendto(g_network_status.udp_broadcast_listener,
                   (char *) g_ethernet_communciation_buffer, nReplyLen, 0,
                   (struct sockaddr *) &stFrom, sizeof(stFrom)) != nReplyLen) {
          OPENER_TRACE_INFO(
              "networkhandler: UDP response was not fully sent\n");
        }
      }
    } while (nRemainingBytes > 0);
  }
}

void CheckAndHandleUdpUnicastSocket() {
  int nReceived_size;
  int nRemainingBytes;
  int nReplyLen;
  EipUint8 *rxp;
  struct sockaddr_in stFrom;
  unsigned long nFromLen;

  /* see if this is an unsolicited inbound UDP message */
  if (true == checkSocketSet(g_network_status.udp_unicast_listener)) {

    nFromLen = sizeof(stFrom);

    OPENER_TRACE_STATE(
        "networkhandler: unsolicited UDP message on EIP broadcast socket\n");

    /*Handle udp broadcast messages */
    nReceived_size = recvfrom(g_network_status.udp_unicast_listener,
                              g_ethernet_communciation_buffer,
                              PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
                              (struct sockaddr *) &stFrom, &nFromLen);

    if (nReceived_size <= 0) { /* got error */
      OPENER_TRACE_ERR(
          "networkhandler: error on recvfrom udp broadcast port: %s\n",
          strerror(errno));
      return;
    }

    OPENER_TRACE_INFO("Data received on udp:\n");

    rxp = &g_ethernet_communciation_buffer[0];
    do {
      nReplyLen = HandleReceivedExplictUdpData(
          g_network_status.udp_unicast_listener, &stFrom, rxp, nReceived_size,
          &nRemainingBytes, true);

      rxp += nReceived_size - nRemainingBytes;
      nReceived_size = nRemainingBytes;

      if (nReplyLen > 0) {
        OPENER_TRACE_INFO("reply sent:\n");

        /* if the active fd matches a registered UDP callback, handle a UDP packet */
        if (sendto(g_network_status.udp_unicast_listener,
                   (char *) g_ethernet_communciation_buffer, nReplyLen, 0,
                   (struct sockaddr *) &stFrom, sizeof(stFrom)) != nReplyLen) {
          OPENER_TRACE_INFO(
              "networkhandler: UDP response was not fully sent\n");
        }
      }
    } while (nRemainingBytes > 0);
  }
}

void CheckAndHandleConsumingUdpSockets() {
  int nReceived_size;
  struct sockaddr_in stFrom;
#ifndef WIN32
  socklen_t nFromLen;
#else
  unsigned long nFromLen;
#endif

  ConnectionObject *pstRunner = g_active_connection_list;
  ConnectionObject *pstCurrent;

  /* see a message on one of the registered UDP sockets has been received     */
  while (NULL != pstRunner) {
    pstCurrent = pstRunner;
    pstRunner = pstRunner->next_connection_object; /* do this at the beginning as the close function may can make the entry invalid */

    if ((-1 != pstCurrent->socket[kUdpCommuncationDirectionConsuming])
        && (true
            == checkSocketSet(
                pstCurrent->socket[kUdpCommuncationDirectionConsuming]))) {
      nFromLen = sizeof(stFrom);
      nReceived_size = recvfrom(
          pstCurrent->socket[kUdpCommuncationDirectionConsuming],
          g_ethernet_communciation_buffer, PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
          (struct sockaddr *) &stFrom, &nFromLen);
      if (0 == nReceived_size) {
        OPENER_TRACE_STATE("connection closed by client\n");
        pstCurrent->connection_close_function(pstCurrent);
        continue;
      }

      if (0 > nReceived_size) {
        OPENER_TRACE_ERR("networkhandler: error on recv: %s\n",
                         strerror(errno));
        pstCurrent->connection_close_function(pstCurrent);
        continue;
      }

      HandleReceivedConnectedData(g_ethernet_communciation_buffer,
                                  nReceived_size, &stFrom);

    }
  }
}
