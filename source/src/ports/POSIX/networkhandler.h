/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef NETWORKHANDLER_H_
#define NETWORKHANDLER_H_

#include "typedefs.h"

/*! Start a TCP/UDP listening socket, accept connections, receive data in select loop, call manageConnections periodically.
 *  @return status
 *          EIP_ERROR .. error
 */
EipStatus NetworkHandler_Init(void);
EipStatus NetworkHandler_ProcessOnce(void);
EipStatus NetworkHandler_Finish(void);

#endif /*NETWORKHANDLER_H_*/
