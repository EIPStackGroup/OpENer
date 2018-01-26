/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPETHERNETLINK_H_
#define OPENER_CIPETHERNETLINK_H_

#include "typedefs.h"
#include "ciptypes.h"

#define CIP_ETHERNETLINK_CLASS_CODE 0xF6

/* public functions */
/** @brief Initialize the Ethernet Link Objects data
 *
 *  @return kEipStatusOk if initialization was successful, otherwise kEipStatusError
 */
EipStatus CipEthernetLinkInit(void);

int EncodeInterfaceCounters(EipUint8 **pa_acMsg);

int EncodeMediaCounters(EipUint8 **pa_acMsg);

int EncodeInterfaceControl(EipUint8 **pa_acMsg);

int EncodeInterfaceCapability(EipUint8 **pa_acMsg);

EipStatus GetAttributeSingleEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session);

/** @brief Data of an CIP Ethernet Link object */
typedef struct {
  EipUint32 interface_speed; /**< 10/100/1000 Mbit/sec */
  EipUint32 interface_flags; /**< Inferface flags as defined in the CIP specification */
  EipUint8 physical_address[6]; /**< MAC address of the Ethernet link */
} CipEthernetLinkObject;

/* global private variables */

CipEthernetLinkObject g_ethernet_link;

#endif /* OPENER_CIPETHERNETLINK_H_*/
