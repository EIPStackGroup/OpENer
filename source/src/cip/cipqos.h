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

/** @brief QoS Object class code */
static const CipUint kCipQoSClassCode = 0x48u;

/* public functions */
/** @brief Initializing the data structures of the TCP/IP interface object
 */
CipUsint CipQosGetDscpPriority(ConnectionObjectPriority priority);

EipStatus CipQoSInit(void);

/** @brief Updates the currently used set of DSCP priority values
 *
 */
void CipQosUpdateUsedSetQosValues(
  );

/** @brief Reset attribute values to default. Does not update currently used set */
void CipQosResetAttributesToDefaultValues(
  );

#endif  /* OPENER_CIPQOS_H_*/
