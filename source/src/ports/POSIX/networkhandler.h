/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_NETWORKHANDLER_H_
#define OPENER_NETWORKHANDLER_H_

#include "typedefs.h"
#include "generic_networkhandler.h"



void IApp_CloseSocket_udp(int socket_handle);

void IApp_CloseSocket_tcp(int socket_handle);


#endif /* OPENER_NETWORKHANDLER_H_ */
