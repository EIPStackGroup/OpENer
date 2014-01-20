/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef ARCHNW_H_
#define ARCHNW_H_

#include "lwiplib.h"

extern void *mycalloc(unsigned);
#define cip_calloc(x,y) mycalloc((unsigned)((x)*(y)))

#define INLINE inline

#define SOCKET_ERROR -1

#define TIMERTICK 10 /* time between calls of manageConnections() in Milliseconds */

// define the socket address structure -- it is different between Berkley sockets and LWIP

typedef unsigned short int sa_family_t;
typedef unsigned short int in_port_t;
typedef unsigned int in_addr_t;

#define PF_INET         2					// protocol family
#define AF_INET         PF_INET					// address family
#define SOCK_DGRAM	2
#define SHUT_RDWR	2
#define INADDR_ANY	0

#endif /*ARCHNW_H_*/
