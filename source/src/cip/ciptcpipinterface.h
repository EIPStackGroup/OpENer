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

typedef struct
{
  EIP_UINT8  m_unAllocControl;
  EIP_UINT8  m_unReserved; /*!< shall be zereo */
  EIP_UINT16 m_unNumMcast;
  EIP_UINT32 m_unMcastStartAddr;
}SMcastConfig;

/* global public variables */
extern EIP_UINT8 g_unTTLValue;

extern SMcastConfig g_stMultiCastconfig;


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
