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

#endif /* OPENER_CIPETHERNETLINK_H_*/
