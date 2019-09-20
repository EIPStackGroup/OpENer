/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPETHERNETLINK_H_
#define OPENER_CIPETHERNETLINK_H_

#include "typedefs.h"
#include "ciptypes.h"

/** @brief This Ethernet Link class id as #define is still needed for a static
 *  initialization. */
#define CIP_ETHERNETLINK_CLASS_CODE   0xF6u
/** @brief Ethernet Link class code */
static const CipUint kCipEthernetLinkClassCode = CIP_ETHERNETLINK_CLASS_CODE;

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

/** @brief Data of an CIP Ethernet Link object */
typedef struct {
  EipUint32 interface_speed; /**< 10/100/1000 Mbit/sec */
  EipUint32 interface_flags; /**< Interface flags as defined in the CIP specification */
  EipUint8 physical_address[6]; /**< MAC address of the Ethernet link */
} CipEthernetLinkObject;

/* global private variables */

CipEthernetLinkObject g_ethernet_link;

#endif /* OPENER_CIPETHERNETLINK_H_*/
