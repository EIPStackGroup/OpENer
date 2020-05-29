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
static const CipUint kCipQoSClassCode = 0x48U;

/* public types */

/** This type represents the group of DSCP values of the QoS object. */
typedef struct cip_qos_dscp_values {
  CipUsint event; /**< Attr. #2: DSCP value for event messages */
  CipUsint general; /**< Attr. #3: DSCP value for general messages */
  CipUsint urgent; /**< Attr. #4: DSCP value for CIP transport class 0/1 Urgent priority messages */
  CipUsint scheduled; /**< Attr. #5: DSCP value for CIP transport class 0/1 Scheduled priority messages */
  CipUsint high; /**< Attr. #6: DSCP value for CIP transport class 0/1 High priority messages */
  CipUsint low; /**< Attr. #7: DSCP value for CIP transport class 0/1 low priority messages */
  CipUsint explicit_msg; /**< Attr. #8: DSCP value for CIP explicit messages (transport class 2/3 and UCMM)
                                        and all other EtherNet/IP encapsulation messages */
} CipQosDscpValues;

/** This type represents the QoS object */
typedef struct {
  CipUsint q_frames_enable; /**< Attr. #1: Enables or disable sending 802.1Q frames on CIP and IEEE 1588 messages */
  CipQosDscpValues dscp; /**< Attributes #2 ... #8 of DSCP values - beware! must not be the used set */
} CipQosObject;


/* public data */
extern CipQosObject g_qos;


/* public functions */
/** @brief Provide the matching DSCP value for a given connection object priority level
 */
CipUsint CipQosGetDscpPriority(ConnectionObjectPriority priority);

/** @brief Create and initialize the QoS object
 */
EipStatus CipQoSInit(void);

/** @brief Updates the currently used set of DSCP priority values
 */
void CipQosUpdateUsedSetQosValues(void);

/** @brief Reset attribute values to default. Does not update currently used set */
void CipQosResetAttributesToDefaultValues(void);

#endif  /* OPENER_CIPQOS_H_*/
