/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file generic_networkhandler.c
 *  @author Martin Melik Merkumians
 *  @brief This file includes all platform-independent functions of the network handler to reduce code duplication
 *
 *  @attention This file should only be used for implementing port-specific network handlers and is not intended to grant other parts of the OpENer access to the network layer
 */

#ifndef GENERIC_NETWORKHANDLER_H_
#define GENERIC_NETWORKHANDLER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "opener_api.h"
#include "typedefs.h"
#include "endianconv.h"
#include "cipconnectionmanager.h"
#include "networkhandler.h"
#include "appcontype.h"
#include "socket_timer.h"

SocketTimer g_timestamps[OPENER_NUMBER_OF_SUPPORTED_SESSIONS];

EipUint8 g_ethernet_communication_buffer[PC_OPENER_ETHERNET_BUFFER_SIZE]; /**< communication buffer */

fd_set master_socket;
fd_set read_socket;

int highest_socket_handle; /**< temporary file descriptor for select() */

/** @brief This variable holds the TCP socket the received to last explicit message.
 * It is needed for opening point to point connection to determine the peer's
 * address.
 */
int g_current_active_tcp_socket;

struct timeval g_time_value;
MilliSeconds g_actual_time;
MilliSeconds g_last_time;
/** @brief Struct representing the current network status
 *
 */
typedef struct {
  int tcp_listener; /**< TCP listener socket */
  int udp_unicast_listener; /**< UDP unicast listener socket */
  int udp_global_broadcast_listener; /**< UDP global network broadcast listener */
  MilliSeconds elapsed_time;
} NetworkStatus;

NetworkStatus g_network_status; /**< Global variable holding the current network status */

/** @brief The platform independent part of network handler initialization routine
 *
 *  @return Returns the OpENer status after the initialization routine
 */
EipStatus NetworkHandlerInitialize(void);

void CloseUdpSocket(int socket_handle);

void CloseTcpSocket(int socket_handle);


/** @brief Initializes the network handler, shall be implemented by a port-specific networkhandler
 *
 *  @return EipStatus, if initialization failed EipError is returned
 */
EipStatus NetworkHandlerInitialize(void);

EipStatus NetworkHandlerProcessOnce(void);

EipStatus NetworkHandlerFinish(void);

/** @brief check if the given socket is set in the read set
 * @param socket The socket to check
 * @return true if socket is set
 */
EipBool8 CheckSocketSet(int socket);

/** @brief Returns the socket with the highest id
 * @param socket1 First socket
 * @param socket2 Second socket
 * @param socket3 Third socket
 * @param socket4 Fourth socket
 *
 * @return Highest socket id from the provided sockets
 */
int GetMaxSocket(int socket1,
                 int socket2,
                 int socket3,
                 int socket4);

#endif /* GENERIC_NETWORKHANDLER_H_ */
