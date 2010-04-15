/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <string.h>

#include "cipethernetlink.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "opener_api.h"

typedef struct
  {
    EIP_UINT32 InterfaceSpeed;
    EIP_UINT32 InterfaceFlags;
    EIP_UINT8 PhysicalAddress[6];
  } S_CIP_EthernetLinkObject;

/* global private variables */
S_CIP_EthernetLinkObject stEthernetLink;


void configureMACAddress(const EIP_UINT8 *pa_acMACAddress){
  memcpy(&stEthernetLink.PhysicalAddress, pa_acMACAddress,
      sizeof(stEthernetLink.PhysicalAddress));

}

EIP_STATUS CIP_Ethernet_Link_Init()
  {
    S_CIP_Class *pstEthernetLinkClass;
    S_CIP_Instance *pstEthernetLinkInstance;

    /* set attributes to initial values */
    stEthernetLink.InterfaceSpeed = 100;
    stEthernetLink.InterfaceFlags = 3; /* full duplex active link, in future it should be checked if link is active */

    if ((pstEthernetLinkClass = createCIPClass(CIP_ETHERNETLINK_CLASS_CODE, 0, /* # class attributes*/
        0xffffffff, /* class getAttributeAll mask*/
        0, /* # class services*/
        3, /* # instance attributes*/
        0xffffffff, /* instance getAttributeAll mask*/
        0, /* # instance services*/
        1, /* # instances*/
        "Ethernet link", 1)) != 0)
      {

        pstEthernetLinkInstance = getCIPInstance(pstEthernetLinkClass, 1);
        insertAttribute(pstEthernetLinkInstance, 1, CIP_UDINT,
            &stEthernetLink.InterfaceSpeed); /* bind attributes to the instance*/
        insertAttribute(pstEthernetLinkInstance, 2, CIP_DWORD,
            &stEthernetLink.InterfaceFlags);
        insertAttribute(pstEthernetLinkInstance, 3, CIP_6USINT,
            &stEthernetLink.PhysicalAddress);
      }
    else
      {
        return EIP_ERROR;
      }

    return EIP_OK;
  }
