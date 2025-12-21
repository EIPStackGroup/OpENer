/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file generic_networkhandler.h
 *  @author Martin Melik Merkumians
 *  @brief This file includes all platform-independent functions of the network
 * handler to reduce code duplication
 *
 *  @attention This file should only be used for implementing port-specific
 * network handlers and is not intended to grant other parts of the OpENer
 * access to the network layer
 */

#ifndef PORTS_GENERIC_NETWORKHANDLER_H_
#define PORTS_GENERIC_NETWORKHANDLER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(STM32) /** Not STM32 target */
#include <errno.h>
#else /** STM32 target (GCC), lwip has its own error code list */
#include "lwip/errno.h"
#endif /* STM32 target */

#include "api/opener_api.h"
#include "cip/appcontype.h"
#include "cip/cipconnectionmanager.h"
#include "core/typedefs.h"
#include "enet_encap/endianconv.h"
#include "ports/networkhandler.h"
#include "ports/socket_timer.h"

/*The port to be used per default for I/O messages on UDP.*/
extern const uint16_t kOpenerEipIoUdpPort;
extern const uint16_t kOpenerEthernetPort;

extern SocketTimer g_timestamps[OPENER_NUMBER_OF_SUPPORTED_SESSIONS];
/// @brief Ethernet/IP standard ports

/// Port to be used per default for messages on TCP
#define kOpenerEthernetPort 44818
/// Port to be used per default for I/O messages on UDP.
#define kOpenerEipIoUdpPort 2222

// EipUint8 g_ethernet_communication_buffer[PC_OPENER_ETHERNET_BUFFER_SIZE];
// /**< communication buffer */

/// @brief Master socket set for select function
extern fd_set master_socket;
/// @brief Read socket set for select function
extern fd_set read_socket;

/// temporary file descriptor for select()
extern int highest_socket_handle;

/** @brief This variable holds the TCP socket the received to last explicit
 * message. It is needed for opening point to point connection to determine the
 * peer's address.
 */
extern int g_current_active_tcp_socket;

/// @brief Time value for select function
extern struct timeval g_time_value;
/// @brief Current time in milliseconds
extern MilliSeconds g_actual_time;
/// @brief Last time in milliseconds
extern MilliSeconds g_last_time;

/// @brief Struct representing the current network status
typedef struct {
  int tcp_listener;                   ///< TCP listener socket
  int udp_unicast_listener;           ///< UDP unicast listener socket
  int udp_global_broadcast_listener;  ///< UDP global network broadcast listener
  int udp_io_messaging;               ///< UDP IO messaging socket
  CipUdint ip_address;    ///< IP being valid during NetworkHandlerInitialize()
  CipUdint network_mask;  ///< network mask being valid during
                          ///< NetworkHandlerInitialize()
  MilliSeconds elapsed_time;  ///< elapsed time since last call of
                              ///< NetworkHandlerProcessCyclic()
} NetworkStatus;

extern NetworkStatus
  g_network_status; /**< Global variable holding the current network status */

/** @brief The platform independent part of network handler initialization
 * routine
 *
 *  @return Returns the OpENer status after the initialization routine
 */
EipStatus NetworkHandlerInitialize(void);

void CloseUdpSocket(int socket_handle);

void CloseTcpSocket(int socket_handle);

EipStatus NetworkHandlerProcessCyclic(void);

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
int GetMaxSocket(int socket1, int socket2, int socket3, int socket4);

/** @brief Set the Qos the socket for implicit IO messaging
 *
 * @return 0 if successful, else the error code */
int SetQos(CipUsint qos_for_socket);

/** @brief Set the socket options for Multicast Producer
 *
 * @return 0 if successful, else the error code */
int SetSocketOptionsMulticastProduce(void);

/** @brief Get the peer address
 *
 * @return peer address if successful, else any address (0) */
EipUint32 GetPeerAddress(void);

#endif  // PORTS_GENERIC_NETWORKHANDLER_H_
