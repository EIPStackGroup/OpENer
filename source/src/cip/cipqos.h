/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_CIPQOS_H_
#define OPENER_CIPQOS_H_

/** @file cipqos.h
 *  @brief Public interface of the QoS Object
 *
 */

#include "typedefs.h"
#include "ciptypes.h"
#include "cipconnectionmanager.h"

/** @brief QoS Object class id */
static const EipUint16 kCipQoSClassId = 0x48u;

/* public functions */
/** @brief Initializing the data structures of the TCP/IP interface object
 */
CipUsint CipQosGetDscpPriority(ConnectionObjectPriority priority);
EipStatus CipQoSInit(void);

#endif  /* OPENER_CIPQOS_H_*/
