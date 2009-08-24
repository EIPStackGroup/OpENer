/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#ifndef CIPETHERNETLINK_H_
#define CIPETHERNETLINK_H_

#include "typedefs.h"
#include "ciptypes.h"

#define CIP_ETHERNETLINK_CLASS_CODE 0xF6

typedef struct
  {
    EIP_UINT32 InterfaceSpeed;
    EIP_UINT32 InterfaceFlags;
    EIP_UINT8 PhysicalAddress[6];
  } S_CIP_EthernetLinkObject;

/* public functions */
/*!Initialize the Ethernet Link Objects data
 */
EIP_STATUS CIP_Ethernet_Link_Init(void);


#endif /*CIPETHERNETLINK_H_*/
