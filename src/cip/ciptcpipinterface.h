/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef CIPTCPIPINTERFACE_H_
#define CIPTCPIPINTERFACE_H_

#include "typedefs.h"
#include "ciptypes.h"

#define CIP_TCPIPINTERFACE_CLASS_CODE 0xF5

/* global public variables */

/* public functions */
/*!Initializing the data structures of the TCPIP interface object 
 */ 
EIP_STATUS CIP_TCPIP_Interface_Init(void);
/*!\brief Clean up the allocated data of the TCPIP interface object.
 *
 * Currently this is the host name string and the domain name string.
 *
 */
void shutdownTCPIP_Interface(void);


#endif /*CIPTCPIPINTERFACE_H_*/
