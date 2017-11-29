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
  struct sockaddr *originator_address);

#endif /* OPENER_CIPETHERNETLINK_H_*/
