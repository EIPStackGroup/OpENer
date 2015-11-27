/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <opener_api.h>
#include "networkhandler.h"
#include <encap.h>
#include <cipconnectionmanager.h>
#include <endianconv.h>
#include <trace.h>
#include <ciptcpipinterface.h>

/* values needed from the connection manager */
extern ConnectionObject *g_active_connection_list;
/* communication buffer */EipUint8 g_ethernet_communication_buffer[PC_OPENER_ETHERNET_BUFFER_SIZE];

#define MAX_NO_OF_TCP_SOCKETS 10

typedef long MILLISECONDS;
typedef unsigned long long MICROSECONDS;
fd_set master_socket;
fd_set read_socket;
/* temporary file descriptor for select() */

int highest_socket_handle;

/*!< This var holds the TCP socket the received to last explicit message.
 * It is needed for opening point to point connection to determine the peer's
 * address.
 */
int g_current_active_tcp_socket;

static struct timeval tv;
static MILLISECONDS actualtime, lasttime;

/*!\brief handle any connection request coming in the TCP server socket.
 *
 */
void
CheckAndHandleTcpListenerSocket();

/*! \brief check if data has been received on the udp broadcast socket and if yes handle it correctly
 *
 */
void
CheckAndHandleUdpLocalBroadcastSocket();

/*! \brief check if on one of the udp consuming sockets data has been received and if yes handle it correctly
 *
 */
void
CheckAndHandleConsumingUdpSockets();
/*! \brief check if the given socket is set in the read set
 *
 */EipBool8
CheckSocketSet(int pa_nSocket);

/*!
 *
 */
EipStatus
HandleDataOnTcpSocket(int pa_nSocket);

static MICROSECONDS getMicroseconds() {
#ifdef WIN32
  LARGE_INTEGER lPerformanceCouner;
  LARGE_INTEGER lPerformanceFrequency;

  QueryPerformanceCounter(&lPerformanceCouner);
  QueryPerformanceFrequency(&lPerformanceFrequency);

  return (MICROSECONDS) (lPerformanceCouner.QuadPart * 1000000LL / lPerformanceFrequency.QuadPart);
#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (MICROSECONDS) tv.tv_sec * 1000000ULL + (MICROSECONDS) tv.tv_usec;
#endif
}

static MILLISECONDS getmilliseconds(void) {
  return (MILLISECONDS) (getMicroseconds() / 1000ULL);
}

/* INT8 Start_NetworkHandler()
 * 	start a TCP listening socket, accept connections, receive data in select loop, call manageConnections periodically.
 * 	return status
 * 			-1 .. error
 */

struct NetworkStatus {
  int nTCPListener;
  int nUDPListener;
  MILLISECONDS elapsedtime;
} PACKED;

struct NetworkStatus g_network_status;

EipStatus NetworkHandlerInitialize(void) {
  struct sockaddr_in my_addr;
  int y;
  int nOptVal;

#ifdef WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  wVersionRequested = MAKEWORD(2, 2);
  WSAStartup(wVersionRequested, &wsaData);
#endif

  /* clear the master an temp sets                                            */
  FD_ZERO(&master_socket);
  FD_ZERO(&read_socket);

  /* create a new TCP socket */
  if ((g_network_status.nTCPListener = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    OPENER_TRACE_ERR("error allocating socket stream listener, %d\n", errno);
    return EIP_ERROR;
  }

  nOptVal = 1;
  if (setsockopt(g_network_status.nTCPListener, SOL_SOCKET, SO_REUSEADDR,
                 (char *) &nOptVal, sizeof(nOptVal)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on nTCPListener\n");
    return EIP_ERROR;
  }

  /* create a new UDP socket */
  if ((g_network_status.nUDPListener = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    OPENER_TRACE_ERR("error allocating udp listener socket, %d\n", errno);
    return EIP_ERROR;
  }

  if (setsockopt(g_network_status.nUDPListener, SOL_SOCKET, SO_REUSEADDR,
                 (char *) &nOptVal, sizeof(nOptVal)) == -1) {
    OPENER_TRACE_ERR(
        "error setting socket option SO_REUSEADDR on nUDPListener\n");
    return EIP_ERROR;
  }

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(kOpenerEthernetPort);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&my_addr.sin_zero, 0, sizeof(my_addr.sin_zero));

  /* bind the new socket to port 0xAF12 (CIP) */
  if ((bind(g_network_status.nTCPListener, (struct sockaddr *) &my_addr,
            sizeof(struct sockaddr))) == -1) {
    OPENER_TRACE_ERR("error with bind: %s\n", strerror(errno));
    return EIP_ERROR;
  }

  /* enable the udp socket to receive broadcast messages*/
  y = 1;
  if (0
      > setsockopt(g_network_status.nUDPListener, SOL_SOCKET, SO_BROADCAST,
                   (char *) &y, sizeof(int))) {
    OPENER_TRACE_ERR(
        "error with setting broadcast receive for udp socket: %s\n",
        strerror(errno));
    return EIP_ERROR;
  }

  if ((bind(g_network_status.nUDPListener, (struct sockaddr *) &my_addr,
            sizeof(struct sockaddr))) == -1) {
    OPENER_TRACE_ERR("error with udp bind: %s\n", strerror(errno));
    return EIP_ERROR;
  }

  /* switch socket in listen mode */
  if ((listen(g_network_status.nTCPListener, MAX_NO_OF_TCP_SOCKETS)) == -1) {
    OPENER_TRACE_ERR("networkhandler: error with listen: %s\n",
                     strerror(errno));
    return EIP_ERROR;
  }

  /* add the listener socket to the master set */FD_SET(
      g_network_status.nTCPListener, &master_socket);
  FD_SET(g_network_status.nUDPListener, &master_socket);

  /* keep track of the biggest file descriptor */
  highest_socket_handle =
      (g_network_status.nTCPListener > g_network_status.nUDPListener) ?
          g_network_status.nTCPListener : g_network_status.nUDPListener;

  lasttime = getmilliseconds(); /* initialize time keeping */
  g_network_status.elapsedtime = 0;

  return EIP_OK;
}

EipStatus NetworkHandlerProcessOnce(void) {
  int fd;
  int res;

  read_socket = master_socket;

  tv.tv_sec = 0;
  tv.tv_usec = (
      g_network_status.elapsedtime < OPENER_TIMER_TICK ?
          OPENER_TIMER_TICK - g_network_status.elapsedtime : 0) * 1000; /* 10 ms */

  res = select(highest_socket_handle + 1, &read_socket, 0, 0, &tv);

  if (res == -1) {
    if (EINTR == errno) /* we have somehow been interrupted. The default behavior is to go back into the select loop. */
    {
      return EIP_OK;
    } else {
      OPENER_TRACE_ERR("networkhandler: error with select: %s\n",
                       strerror(errno));
      return EIP_ERROR;
    }
  }

  if (res > 0) {

    CheckAndHandleTcpListenerSocket();
    CheckAndHandleUdpLocalBroadcastSocket();
    CheckAndHandleConsumingUdpSockets();

    for (fd = 0; fd <= highest_socket_handle; fd++) {
      if (true == CheckSocketSet(fd)) {
        /* if it is still checked it is a TCP receive */
        if (EIP_ERROR == HandleDataOnTcpSocket(fd)) /* if error */
        {
          CloseSocket(fd);
          CloseSession(fd); /* clean up session and close the socket */
        }
      }
    }
  }

  actualtime = getmilliseconds();
  g_network_status.elapsedtime += actualtime - lasttime;
  lasttime = actualtime;

  /* check if we had been not able to update the connection manager for several OPENER_TIMER_TICK.
   * This should compensate the jitter of the windows timer
   */
  while (g_network_status.elapsedtime >= OPENER_TIMER_TICK) {
    /* call manage_connections() in connection manager every OPENER_TIMER_TICK ms */
    ManageConnections();
    g_network_status.elapsedtime -= OPENER_TIMER_TICK;
  }
  return EIP_OK;
}

EipStatus NetworkHandlerFinish(void) {
  CloseSocket(g_network_status.nTCPListener);
  CloseSocket(g_network_status.nUDPListener);
  return EIP_OK;
}

EipBool8 CheckSocketSet(int pa_nSocket) {
  EipBool8 nRetVal = false;
  if (FD_ISSET(pa_nSocket, &read_socket)) {
    if (FD_ISSET(pa_nSocket, &master_socket)) {
      nRetVal = true;
    } else {
      OPENER_TRACE_INFO("socket: %d closed with pending message\n", pa_nSocket);
    }
    FD_CLR(pa_nSocket, &read_socket);
    /* remove it from the read set so that later checks will not find it */
  }

  return nRetVal;
}
EipStatus SendUdpData(struct sockaddr_in *pa_pstAddr, int pa_nSockFd,
                      EipUint8 *pa_acData, EipUint16 pa_nDataLength) {
  int sentlength;

  sentlength = sendto(pa_nSockFd, (char *) pa_acData, pa_nDataLength, 0,
                      (struct sockaddr *) pa_pstAddr, sizeof(*pa_pstAddr));

  if (sentlength < 0) {
    OPENER_TRACE_ERR("networkhandler: error with sendto in sendUDPData: %s\n",
                     strerror(errno));
    return EIP_ERROR;
  } else if (sentlength != pa_nDataLength) {
    OPENER_TRACE_WARN("not all data was sent in sendUDPData, sent %d of %d\n",
                      sentlength, pa_nDataLength);
    return EIP_ERROR;
  } else
    return EIP_OK;
}

EipStatus HandleDataOnTcpSocket(int pa_nSocket) {
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
  nCheckVal = recv(pa_nSocket, g_ethernet_communication_buffer, 4, 0); /*TODO we may have to set the socket to a non blocking socket */

  if (nCheckVal == 0) {
    OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                     strerror(errno));
    return EIP_ERROR;
  }
  if (nCheckVal < 0) {
    OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
    return EIP_ERROR;
  }

  rxp = &g_ethernet_communication_buffer[2]; /* at this place EIP stores the data length */
  unDataSize = GetIntFromMessage(&rxp) + ENCAPSULATION_HEADER_LENGTH - 4; /* -4 is for the 4 bytes we have already read*/
  /* (NOTE this advances the buffer pointer) */
  if (PC_OPENER_ETHERNET_BUFFER_SIZE - 4 < unDataSize) { /*TODO can this be handled in a better way?*/
    OPENER_TRACE_ERR(
        "too large packet received will be ignored, will drop the data\n");
    /* Currently we will drop the whole packet */
    nDataSent = PC_OPENER_ETHERNET_BUFFER_SIZE;

    do {
      nCheckVal = recv(pa_nSocket, g_ethernet_communication_buffer, nDataSent, 0);

      if (nCheckVal == 0) /* got error or connection closed by client */
      {
        OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                         strerror(errno));
        return EIP_ERROR;
      }
      if (nCheckVal < 0) {
        OPENER_TRACE_ERR("networkhandler: error on recv: %s\n",
                         strerror(errno));
        return EIP_ERROR;
      }
      unDataSize -= nCheckVal;
      if ((unDataSize < PC_OPENER_ETHERNET_BUFFER_SIZE) && (unDataSize != 0)) {
        nDataSent = unDataSize;
      }
    } while (0 != unDataSize); /*TODO fragile end statement */
    return EIP_OK;
  }

  nCheckVal = recv(pa_nSocket, &g_ethernet_communication_buffer[4], unDataSize, 0);

  if (nCheckVal == 0) /* got error or connection closed by client */
  {
    OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n",
                     strerror(errno));
    return EIP_ERROR;
  }
  if (nCheckVal < 0) {
    OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
    return EIP_ERROR;
  }

  if ((unsigned) nCheckVal == unDataSize) {
    /*we got the right amount of data */
    unDataSize += 4;
    /*TODO handle partial packets*/
    OPENER_TRACE_INFO("Data received on tcp:\n");

    g_current_active_tcp_socket = pa_nSocket;

    nCheckVal = HandleReceivedExplictTcpData(pa_nSocket,
                                             g_ethernet_communication_buffer,
                                             unDataSize, &nRemainingBytes);

    g_current_active_tcp_socket = -1;

    if (nRemainingBytes != 0) {
      OPENER_TRACE_WARN(
          "Warning: received packet was to long: %d Bytes left!\n",
          nRemainingBytes);
    }

    if (nCheckVal > 0) {
      OPENER_TRACE_INFO("reply sent:\n");

      nDataSent = send(pa_nSocket, (char *) g_ethernet_communication_buffer, nCheckVal,
                       0);
      if (nDataSent != nCheckVal) {
        OPENER_TRACE_WARN("TCP response was not fully sent\n");
      }
    }

    return EIP_OK;
  } else {
    /* we got a fragmented packet currently we cannot handle this will
     * for this we would need a network buffer per TCP socket
     *
     * However with typical packet sizes of EIP this should't be a big issue.
     */
    /*TODO handle fragmented packets */
  }
  return EIP_ERROR;
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
    return EIP_INVALID_SOCKET;
  }

  OPENER_TRACE_INFO("networkhandler: UDP socket %d\n", newfd);

  /* check if it is sending or receiving */
  if (pa_nDirection == CONSUMING) {
    int nOptVal = 1;
    if (setsockopt(newfd, SOL_SOCKET, SO_REUSEADDR, (char *) &nOptVal,
                   sizeof(nOptVal)) == -1) {
      OPENER_TRACE_ERR(
          "error setting socket option SO_REUSEADDR on consuming udp socket\n");
      return EIP_ERROR;
    }

    /* bind is only for consuming necessary */
    if ((bind(newfd, (struct sockaddr *) pa_pstAddr, sizeof(struct sockaddr)))
        == -1) {
      OPENER_TRACE_ERR("error on bind udp: %s\n", strerror(errno));
      return EIP_INVALID_SOCKET;
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
          return EIP_INVALID_SOCKET;
        }
      }
    }
  }

  if ((pa_nDirection == CONSUMING) || (0 == pa_pstAddr->sin_addr.s_addr)) {
    /* we have a peer to peer producer or a consuming connection*/
    if (getpeername(g_current_active_tcp_socket, (struct sockaddr *) &stPeerAdr,
                    &nPeerAddrLen) < 0) {
      OPENER_TRACE_ERR("networkhandler: could not get peername: %s\n",
                       strerror(errno));
      return EIP_INVALID_SOCKET;
    }
    /* store the originators address */
    pa_pstAddr->sin_addr.s_addr = stPeerAdr.sin_addr.s_addr;
  }

  /* add new fd to the master list                                             */
  FD_SET(newfd, &master_socket);
  if (newfd > highest_socket_handle) {
    highest_socket_handle = newfd;
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
  if (EIP_INVALID_SOCKET != pa_nSockFd) {
    FD_CLR(pa_nSockFd, &master_socket);
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
  if (true == CheckSocketSet(g_network_status.nTCPListener)) {
    OPENER_TRACE_INFO("networkhandler: new TCP connection\n");

    newfd = accept(g_network_status.nTCPListener, NULL, NULL);
    if (newfd == -1) {
      OPENER_TRACE_ERR("networkhandler: error on accept: %s\n",
                       strerror(errno));
      return;
    }

    FD_SET(newfd, &master_socket);
    /* add newfd to master set */
    if (newfd > highest_socket_handle) {
      highest_socket_handle = newfd;
    }

    OPENER_TRACE_STATE("networkhandler: opened new TCP connection on fd %d\n",
                       newfd);
  }
}

void CheckAndHandleUdpLocalBroadcastSocket() {
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
  if (true == CheckSocketSet(g_network_status.nUDPListener)) {

    nFromLen = sizeof(stFrom);

    OPENER_TRACE_STATE(
        "networkhandler: unsolicited UDP message on EIP broadcast socket\n");

    /*Handle udp broadcast messages */
    nReceived_size = recvfrom(g_network_status.nUDPListener,
                              g_ethernet_communication_buffer,
                              PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
                              (struct sockaddr *) &stFrom, &nFromLen);

    if (nReceived_size <= 0) { /* got error */
      OPENER_TRACE_ERR(
          "networkhandler: error on recvfrom udp broadcast port: %s\n",
          strerror(errno));
      return;
    }

    OPENER_TRACE_INFO("Data received on udp:\n");

    rxp = &g_ethernet_communication_buffer[0];
    do {
      nReplyLen = HandleReceivedExplictUdpData(g_network_status.nUDPListener,
                                               &stFrom, rxp, nReceived_size,
                                               &nRemainingBytes);

      rxp += nReceived_size - nRemainingBytes;
      nReceived_size = nRemainingBytes;

      if (nReplyLen > 0) {
        OPENER_TRACE_INFO("reply sent:\n");

        /* if the active fd matches a registered UDP callback, handle a UDP packet */
        if (sendto(g_network_status.nUDPListener,
                   (char *) g_ethernet_communication_buffer, nReplyLen, 0,
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

    if ((-1 != pstCurrent->socket[CONSUMING])
        && (true == CheckSocketSet(pstCurrent->socket[CONSUMING]))) {
      nFromLen = sizeof(stFrom);
      nReceived_size = recvfrom(pstCurrent->socket[CONSUMING],
                                g_ethernet_communication_buffer,
                                PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
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

      HandleReceivedConnectedData(g_ethernet_communication_buffer, nReceived_size,
                                  &stFrom);

    }
  }
}
