/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <winsock2.h>
#include <ws2tcpip.h>


/** @brief Platform-specific network socket descriptor type.
 *
 * Windows defines its own (unsigned) data type for network sockets.
 */
typedef SOCKET socket_platform_t;


/** @brief Platform-specific invalid socket constant.
 *
 * Windows provides a constant dedicated to the invalid socket value.
 */
#define kEipInvalidSocket INVALID_SOCKET
