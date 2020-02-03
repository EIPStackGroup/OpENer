/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

/** @file
 * @brief Declare public interface of the DLR object
 *
 * @author Stefan Maetje <stefan.maetje@esd.eu>
 *
 */

#ifndef OPENER_CIPDLR_H_
#define OPENER_CIPDLR_H_

#include "typedefs.h"
#include "ciptypes.h"

/** @brief DLR object class code */
static const CipUint kCipDlrClassCode = 0x47U;

/* ********************************************************************
 * Type declarations
 */
/** @brief Provide bit masks for the Capability Flags attribute (#12)
 *
 * The @ref kDlrCapAnnounceBased and @ref kDlrCapBeaconBased capability
 *  flags are mutually exclusive but one of it must be set.
 */
typedef enum {
  /** Device is an announce based ring node. */
  kDlrCapAnnounceBased = 0x01,
  /** Device is a beacon based ring node. */
  kDlrCapBeaconBased = 0x02,
  /** The device is capable of providing the supervisor function. */
  kDlrCapSupervisor = 0x20,
  /** The device is capable of providing the redundant gateway function. */
  kDlrCapRedundantGateway = 0x40,
  /** The device is capable of supporting the Flush_Tables frame. */
  kDlrCapFlushTableFrame = 0x80,
} CipDlrCapabilityFlags;


/** @brief Node address information for a DLR node
 *
 * This is the node address information that uniquely identifies a
 *  participant of the DLR protocol.
 */
typedef struct {
  CipUdint  device_ip;  /**< IP address of a participating DLR node */
  CipUsint  device_mac[6]; /**< MAC address of a participating DLR node */
} CipNodeAddress;

/** @brief Type declaration for the DLR object
 *
 * This is the type declaration for the DLR object. It contains only the
 *  attributes needed for a non supervisor and non redundant gateway
 *  ring participant.
 */
typedef struct {
  CipUsint  network_topology; /**< Attribute #1: */
  CipUsint  network_status; /**< Attribute #2: */
  CipNodeAddress active_supervisor_address; /**< Attribute #10: */
  CipDword  capability_flags; /**< Attribute #12: */
} CipDlrObject;


/* ********************************************************************
 * global public variables
 */
extern CipDlrObject g_dlr;  /**< declaration of DLR object instance 1 data */


/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the DLR object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipDlrInit(void);

#endif /* of OPENER_CIPDLR_H_ */
