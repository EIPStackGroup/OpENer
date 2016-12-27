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

/** @brief Data of an CIP Ethernet Link object */
typedef struct {
  EipUint32 interface_speed; /**< 10/100/1000 Mbit/sec */
  EipUint32 interface_flags; /**< Inferface flags as defined in the CIP specification */
  EipUint8 physical_address[6]; /**< MAC address of the Ethernet link */
} CipEthernetLinkObject;

/* global private variables */
CipEthernetLinkObject g_ethernet_link;

/** @bried Configures the MAC address of the Ethernet Link object*
 *
 *  @param mac_address The MAC address of the Ethernet Link
 */
void ConfigureMacAddress(const EipUint8 *const mac_address) {
  memcpy( &g_ethernet_link.physical_address, mac_address,
          sizeof(g_ethernet_link.physical_address) );

}

EipStatus CipEthernetLinkInit() {
  CipClass *ethernet_link_class = CreateCipClass(CIP_ETHERNETLINK_CLASS_CODE, 0, /* # class attributes*/
                                                 0xffffffff, /* class getAttributeAll mask*/
                                                 0, /* # class services*/
                                                 3, /* # instance attributes*/
                                                 0xffffffff, /* instance getAttributeAll mask*/
                                                 0, /* # instance services*/
                                                 1, /* # instances*/
                                                 "Ethernet Link", 1);

  /* set attributes to initial values */
  g_ethernet_link.interface_speed = 100;
  g_ethernet_link.interface_flags = 0xF; /* successful speed and duplex neg, full duplex active link, TODO in future it should be checked if link is active */

  if (ethernet_link_class != NULL) {

    CipInstance *ethernet_link_instance =
      GetCipInstance(ethernet_link_class, 1);
    InsertAttribute(ethernet_link_instance, 1, kCipUdint,
                    &g_ethernet_link.interface_speed, kGetableSingleAndAll); /* bind attributes to the instance*/
    InsertAttribute(ethernet_link_instance, 2, kCipDword,
                    &g_ethernet_link.interface_flags, kGetableSingleAndAll);
    InsertAttribute(ethernet_link_instance, 3, kCip6Usint,
                    &g_ethernet_link.physical_address, kGetableSingleAndAll);
  } else {
    return kEipStatusError;
  }

  return kEipStatusOk;
}
