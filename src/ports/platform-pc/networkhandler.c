/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>

#include <opener_api.h>
#include "networkhandler.h"
#include <encap.h>
#include <cipconnectionmanager.h>
#include <endianconv.h> 
#include <trace.h>

/* values needed from the connection manager */
extern S_CIP_ConnectionObject *g_pstActiveConnectionList;

/* communication buffer */
EIP_UINT8 g_acPCEthernetCommBuffer[PC_OPENER_ETHERNET_BUFFER_SIZE];

#define MAX_NO_OF_TCP_SOCKETS 10

typedef long MILLISECONDS;
fd_set master;
fd_set read_fds;
/* temporary file descriptor for select() */

int fdmax;
int newfd;

/*!< This var holds the TCP socket the received to last explicit message.
 * It is needed for opening point to point connection to determine the peer's
 * address.
 */
int g_nCurrentActiveTCPSocket;

int end = 0;

static struct timeval tv;
static MILLISECONDS actualtime, lasttime;

EIP_STATUS
handleDataOnTCPSocket(int pa_nSocket);

static MILLISECONDS
getmilliseconds(void)
{
  struct timeval tv;

  gettimeofday(&tv, 0);
  return (MILLISECONDS) tv.tv_sec * 1000 + (MILLISECONDS) tv.tv_usec / 1000;
}

/*! Check if the socket identifier is associate with an active connection
 * 
 *  @param pa_nFD  socket handle
 *  @return pointer to the connection if the given socket hanlder is an active connection otherwise NULL
 */
S_CIP_ConnectionObject *
isConnectedFd(int pa_nFD)
{
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if ((pstRunner->sockfd[0] == pa_nFD) || (pstRunner->sockfd[1] == pa_nFD))
        {
          break;
        }
      pstRunner = pstRunner->m_pstNext;
    }

  return pstRunner;
}

void
leave(int sig)
{
  sig = sig; /* kill unused parameter warning */
  fprintf(stderr, "got sig hup\n");
  end = 1;
}

/* INT8 Start_NetworkHandler()
 * 	start a TCP listening socket, accept connections, receive data in select loop, call manageConnections periodically.
 * 	return status
 * 			-1 .. error
 */

EIP_STATUS
Start_NetworkHandler()
{
  struct sockaddr_in remote_addr, from;
  struct sockaddr_in my_addr;
  socklen_t addrlen;
  socklen_t fromlen = sizeof(from);
  int nTCPListener;
  int nUDPListener;

  int nReceived_size;
  int res;
  MILLISECONDS elapsedtime = 0;
  int fd;
  EIP_UINT8 *rxp;
  int nRemainingBytes;
  int replylen;
  S_CIP_ConnectionObject *pstConnection;
  int y;

  /* clear the master an temp sets */
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  end = 0;
  signal(SIGHUP, leave);

  /* create a new TCP socket */
  if ((nTCPListener = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
      OPENER_TRACE_ERR("error allocating socket stream listener, %d\n", errno);
      return EIP_ERROR;
    }

  int nOptVal = 1;
  if (setsockopt(nTCPListener, SOL_SOCKET, SO_REUSEADDR, &nOptVal,
      sizeof(nOptVal)) == -1)
    {
      OPENER_TRACE_ERR("error setting socket option SO_REUSEADDR on nTCPListener\n");
      return EIP_ERROR;
    }

  /* create a new UDP socket */
  if ((nUDPListener = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      OPENER_TRACE_ERR("error allocating udp listener socket, %d\n", errno);
      return EIP_ERROR;
    }

  if (setsockopt(nUDPListener, SOL_SOCKET, SO_REUSEADDR, &nOptVal,
      sizeof(nOptVal)) == -1)
    {
      OPENER_TRACE_ERR("error setting socket option SO_REUSEADDR on nUDPListener\n");
      return EIP_ERROR;
    }

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(OPENER_ETHERNET_PORT);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&my_addr.sin_zero, 0, sizeof(my_addr.sin_zero));

  /* bind the new socket to port 0xAF12 (CIP) */
  if ((bind(nTCPListener, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)))
      == -1)
    {
      OPENER_TRACE_ERR("error with bind: %s\n", strerror(errno));
      return EIP_ERROR;
    }

  /* enable the udp socket to receive broadcast messages*/
  y = 1;
  if (0 > setsockopt(nUDPListener, SOL_SOCKET, SO_BROADCAST, &y, sizeof(int)))
    {
      OPENER_TRACE_ERR("error with setting broadcast receive for udp socket: %s\n", strerror(errno));
      return EIP_ERROR;
    }

  if ((bind(nUDPListener, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)))
      == -1)
    {
      OPENER_TRACE_ERR("error with udp bind: %s\n", strerror(errno));
      return EIP_ERROR;
    }

  /* switch socket in listen mode */
  if ((listen(nTCPListener, MAX_NO_OF_TCP_SOCKETS)) == -1)
    {
      OPENER_TRACE_ERR("networkhandler: error with listen: %s\n", strerror(errno));
      return EIP_ERROR;
    }

  /* add the listener socket to the master set */
  FD_SET(nTCPListener, &master);
  FD_SET(nUDPListener, &master);

  /* keep track of the biggest file descriptor */
  fdmax = (nTCPListener > nUDPListener) ? nTCPListener : nUDPListener;

  lasttime = getmilliseconds(); /* initialize time keeping */
  elapsedtime = 0;

  while (0 == end) /*TODO add code here allowing to end OpENer*/
    {
      read_fds = master;

      tv.tv_sec = 0;
      tv.tv_usec = (elapsedtime < OPENER_TIMER_TICK ? OPENER_TIMER_TICK
          - elapsedtime : 0) * 1000; /* 10 ms */

      res = select(fdmax + 1, &read_fds, 0, 0, &tv);
      if (res == -1)
        {
          if (EINTR == errno)
            {
              /* we have somehow been interrupted. The default behavior is to
               * go back into the select loop.
               */
              continue;
            }
          else
            {
              OPENER_TRACE_ERR("networkhandler: error with select: %s\n", strerror(errno));
              return EIP_ERROR;
            }
        }

      if (res > 0)
        for (fd = 0; fd <= fdmax; fd++)
          {
            if (FD_ISSET(fd, &read_fds))
              {
                if (!FD_ISSET(fd, &master))
                  {
                    OPENER_TRACE_INFO("socket fd %d closed with pending message\n", fd);
                    continue;
                  }

                /* see if this is a connection request to the TCP listener*/
                if (fd == nTCPListener) /* handle new TCP connection */
                  {
                    OPENER_TRACE_INFO("networkhandler: new TCP connection\n");
                    addrlen = sizeof(remote_addr);
                    newfd = accept(nTCPListener,
                        (struct sockaddr *) &remote_addr, &addrlen); /* remote_addr does not seem to be used*/
                    if (newfd == -1)
                      {
                        OPENER_TRACE_ERR("networkhandler: error on accept: %s\n", strerror(errno));
                        continue;
                      }

                    FD_SET(newfd, &master);
                    /* add newfd to master set */
                    if (newfd > fdmax)
                      fdmax = newfd;
                    OPENER_TRACE_STATE(
                        "networkhandler: opened new TCP connection on fd %d\n",
                        newfd);
                    continue;
                  }

                /* see if this is an unsolicited inbound UDP message */
                if (fd == nUDPListener)
                  {
                    OPENER_TRACE_STATE(
                        "networkhandler: unsolicited UDP message on fd %d\n",
                        fd);

                    /*Handle udp broadcast messages */
                    nReceived_size = recvfrom(fd, g_acPCEthernetCommBuffer,
                        PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
                        (struct sockaddr *) &from, &fromlen);

                    if (nReceived_size <= 0)
                      { /* got error */
                        OPENER_TRACE_ERR(
                            "networkhandler: error on recvfrom udp broadcast port: %s\n",
                            strerror(errno));
                        continue;
                      }

                    OPENER_TRACE_INFO("Data received on udp:\n");

                    rxp = &g_acPCEthernetCommBuffer[0];
                    do
                      {
                        replylen = handleReceivedExplictData(fd, rxp,
                            nReceived_size, &nRemainingBytes);

                        rxp += nReceived_size - nRemainingBytes;
                        nReceived_size = nRemainingBytes;

                        if (replylen > 0)
                          {
                            OPENER_TRACE_INFO("reply sent:\n");

                            /* if the active fd matches a registered UDP callback, handle a UDP packet */
                            res = sendto(fd, (char *) g_acPCEthernetCommBuffer,
                                replylen, 0, (struct sockaddr *) &from,
                                sizeof(from));
                            if (res != replylen)
                              {
                                OPENER_TRACE_INFO(
                                    "networkhandler: UDP response was not fully sent\n");
                              }
                          }
                      }
                    while (nRemainingBytes > 0);
                    continue;
                  }

                /* see if the message is from a registered UDP socket */
                pstConnection = isConnectedFd(fd);
                if (NULL != pstConnection)
                  {
                    fromlen = sizeof(from);
                    nReceived_size = recvfrom(fd, g_acPCEthernetCommBuffer,
                        PC_OPENER_ETHERNET_BUFFER_SIZE, 0,
                        (struct sockaddr *) &from, &fromlen);
                    if (nReceived_size == 0)
                      {
                        OPENER_TRACE_STATE("connection closed by client\n");
                        closeConnection(pstConnection);
                        continue;
                      }
                    if (nReceived_size <= 0)
                      {
                        OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
                        closeConnection(pstConnection);
                        continue;
                      }

                    handleReceivedConnectedData(g_acPCEthernetCommBuffer,
                            nReceived_size, &from);
                    continue;
                  }

                /* if not registered UDP, handle as a TCP receive */
                if (EIP_ERROR == handleDataOnTCPSocket(fd)) /* if error */
                  {
                    FD_CLR(fd, &master);
                    /* remove connection from master set */
                    closeSession(fd); /* clean up session and close the socket */
                  }
              }
          }

      actualtime = getmilliseconds();
      elapsedtime += actualtime - lasttime;
      lasttime = actualtime;

      /* check if we had been not able to update the connection manager for several OPENER_TIMER_TICK.
       * This should compensate the jitter of the windows timer
       */
      while (elapsedtime >= OPENER_TIMER_TICK)
        {
          /* call manage_connections() in connection manager every OPENER_TIMER_TICK ms */
          manageConnections();
          elapsedtime -= OPENER_TIMER_TICK;
        }
    }

  IApp_CloseSocket(nTCPListener);
  IApp_CloseSocket(nUDPListener);
  return EIP_OK;
}

EIP_STATUS
IApp_SendUDPData(struct sockaddr_in *pa_pstAddr, int pa_nSockFd,
    EIP_UINT8 *pa_acData, EIP_UINT16 pa_nDataLength)
{
  int sentlength;

  sentlength = sendto(pa_nSockFd, (char *) pa_acData, pa_nDataLength, 0,
      (struct sockaddr *) pa_pstAddr, sizeof(*pa_pstAddr));

  if (sentlength < 0)
    {
      OPENER_TRACE_ERR("networkhandler: error with sendto in sendUDPData: %s\n", strerror(errno));
      return EIP_ERROR;
    }
  else if (sentlength != pa_nDataLength)
    {
      OPENER_TRACE_WARN("not all data was sent in sendUDPData, sent %d of %d\n",
          sentlength, pa_nDataLength);
      return EIP_ERROR;
    }
  else
    return EIP_OK;
}

EIP_STATUS
handleDataOnTCPSocket(int pa_nSocket)
{
  EIP_UINT8 *rxp;
  ssize_t nCheckVal;
  size_t unDataSize;
  ssize_t nDataSent;
  int nRemainingBytes = 0;

  /* We will handle just one EIP packet here the rest is done by the select
   * method which will inform us if more data is available in the socket
   because of the current implementation of the main loop this may not be
   the fastest way and a loop here with a non blocking socket would better
   fit*/

  /*Check how many data is here -- read the first four bytes from the connection */
  nCheckVal = recv(pa_nSocket, g_acPCEthernetCommBuffer, 4, 0); /*TODO we may have to set the socket to a non blocking socket */

  if (nCheckVal == 0)
    {
      OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n", strerror(errno));
      return EIP_ERROR;
    }
  if (nCheckVal < 0)
    {
      OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
      return EIP_ERROR;
    }

  rxp = &g_acPCEthernetCommBuffer[2]; /* at this place EIP stores the data length */
  unDataSize = ltohs(&rxp) + ENCAPSULATION_HEADER_LENGTH - 4; /* -4 is for the 4 bytes we have already read*/
  /* (NOTE this advances the buffer pointer) */
  if (PC_OPENER_ETHERNET_BUFFER_SIZE - 4 < unDataSize)
    { /*TODO can this be handled in a better way?*/
      OPENER_TRACE_ERR("too large packet received will be ignored, will drop the data\n");
      /* Currently we will drop the whole packet */
      nDataSent = PC_OPENER_ETHERNET_BUFFER_SIZE;

      do
        {
          nCheckVal = recv(pa_nSocket, g_acPCEthernetCommBuffer, nDataSent, 0);

          if (nCheckVal == 0) /* got error or connection closed by client */
            {
              OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n", strerror(errno));
              return EIP_ERROR;
            }
          if (nCheckVal < 0)
            {
              OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
              return EIP_ERROR;
            }
          unDataSize -= nCheckVal;
          if ((unDataSize < PC_OPENER_ETHERNET_BUFFER_SIZE)
              && (unDataSize != 0))
            {
              nDataSent = unDataSize;
            }
        }
      while (0 != unDataSize); /*TODO fragile end statement */
      return EIP_OK;
    }

  nCheckVal = recv(pa_nSocket, &g_acPCEthernetCommBuffer[4], unDataSize, 0);

  if (nCheckVal == 0) /* got error or connection closed by client */
    {
      OPENER_TRACE_ERR("networkhandler: connection closed by client: %s\n", strerror(errno));
      return EIP_ERROR;
    }
  if (nCheckVal < 0)
    {
      OPENER_TRACE_ERR("networkhandler: error on recv: %s\n", strerror(errno));
      return EIP_ERROR;
    }

  if (nCheckVal == unDataSize)
    {
      /*we got the right amount of data */
      unDataSize += 4;
      /*TODO handle partial packets*/
      OPENER_TRACE_INFO("Data received on tcp:\n");

      g_nCurrentActiveTCPSocket = pa_nSocket;

      nCheckVal = handleReceivedExplictData(pa_nSocket,
          g_acPCEthernetCommBuffer, unDataSize, &nRemainingBytes);

      g_nCurrentActiveTCPSocket = -1;

      if (nRemainingBytes != 0)
        {
          OPENER_TRACE_WARN("Warning: received packet was to long: %d Bytes left!\n",
              nRemainingBytes);
        }

      if (nCheckVal > 0)
        {
          OPENER_TRACE_INFO("reply sent:\n");

          nDataSent = send(pa_nSocket, (char *) g_acPCEthernetCommBuffer,
              nCheckVal, 0);
          if (nDataSent != nCheckVal)
            {
              OPENER_TRACE_WARN("TCP response was not fully sent\n");
            }
        }

      return EIP_OK;
    }
  else
    {
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
int
IApp_CreateUDPSocket(int pa_nDirection, struct sockaddr_in *pa_pstAddr)
{
  struct sockaddr_in stPeerAdr;
  socklen_t nPeerAddrLen = sizeof(struct sockaddr_in);
  /* create a new UDP socket */
  if ((newfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
      OPENER_TRACE_ERR("networkhandler: cannot create UDP socket: %s\n", strerror(errno));
      return EIP_INVALID_SOCKET;
    }

  OPENER_TRACE_INFO("networkhandler: UDP socket %d\n", newfd);

  /* check if it is sending or receiving */
  if (pa_nDirection == CONSUMING)
    {
      int nOptVal = 1;
      if (setsockopt(newfd, SOL_SOCKET, SO_REUSEADDR, &nOptVal, sizeof(nOptVal))
          == -1)
        {
          OPENER_TRACE_ERR("error setting socket option SO_REUSEADDR on consuming udp socket\n");
          return EIP_ERROR;
        }

      /* bind is only for consuming necessary */
      if ((bind(newfd, (struct sockaddr *) pa_pstAddr, sizeof(struct sockaddr)))
          == -1)
        {
          OPENER_TRACE_ERR("error on bind udp: %s\n", strerror(errno));
          return EIP_INVALID_SOCKET;
        }

      OPENER_TRACE_INFO("networkhandler: bind UDP socket %d\n", newfd);
    }

  if ((pa_nDirection == CONSUMING) || (0 == pa_pstAddr->sin_addr.s_addr))
    {
      /* we have a peer to peer producer or a consuming connection*/
      if (getpeername(g_nCurrentActiveTCPSocket,
          (struct sockaddr *) &stPeerAdr, &nPeerAddrLen) < 0)
        {
          OPENER_TRACE_ERR("networkhandler: could not get peername: %s\n", strerror(errno));
          return EIP_INVALID_SOCKET;
        }
      /* store the orignators address */
      pa_pstAddr->sin_addr.s_addr = stPeerAdr.sin_addr.s_addr;
    }

  /* add new fd to the master list */
  FD_SET(newfd, &master);
  if (newfd > fdmax)
    {
      fdmax = newfd;
    }
  return newfd;
}

void
IApp_CloseSocket(int pa_nSockFd)
{

  OPENER_TRACE_INFO("networkhandler: closing socket %d\n", pa_nSockFd);
  if (EIP_INVALID_SOCKET != pa_nSockFd)
    {
      FD_CLR(pa_nSockFd, &master);
      shutdown(pa_nSockFd, SHUT_RDWR);
      close(pa_nSockFd);
    }
}
