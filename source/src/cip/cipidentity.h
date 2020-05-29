/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPIDENTITY_H_
#define OPENER_CIPIDENTITY_H_

#include "typedefs.h"
#include "ciptypes.h"

/** @brief Identity class code */
static const CipUint kCipIdentityClassCode = 0x01U;

/** @brief Status of the CIP Identity object */
typedef enum {
  kOwned = 0x0001, /**< Indicates that the device has an owner */
  kConfigured = 0x0004, /**< Indicates that the device is configured to do
                           something different, than the out-of-the-box default. */
  kMinorRecoverableFault = 0x0100, /**< Indicates that the device detected a
                                      fault with itself, which was thought to be recoverable. The device did not
                                      switch to a faulted state. */
  kMinorUncoverableFault = 0x0200, /**< Indicates that the device detected a
                                      fault with itself, which was thought to be recoverable. The device did not
                                      switch to a faulted state. */
  kMajorRecoverableFault = 0x0400, /**< Indicates that the device detected a
                                      fault with itself,which was thought to be recoverable. The device changed
                                      to the "Major Recoverable Fault" state */
  kMajorUnrecoverableFault = 0x0800 /**< Indicates that the device detected a
                                       fault with itself,which was thought to be recoverable. The device changed
                                       to the "Major Unrecoverable Fault" state */
} CipIdentityStatus;

/** @brief Constants for the extended status field in the Status word */
typedef enum {
  kSelftestingUnknown = 0x0000U,
  kFirmwareUpdateInProgress = 0x0010U,
  kStatusAtLeastOneFaultedIoConnection = 0x0020U,
  kNoIoConnectionsEstablished = 0x0030U,
  kNonVolatileConfigurationBad = 0x0040U,
  kMajorFault = 0x0050U,
  kAtLeastOneIoConnectionInRunMode = 0x0060U,
  kAtLeastOneIoConnectionEstablishedAllInIdleMode = 0x0070U,
  kExtStatusMask = 0x00F0U
} CipIdentityExtendedStatus;

/** @brief Constants for the state member of the Identity object. */
typedef enum {
  kStateNonExistent = 0U,
  kStateSelfTesting = 1U,
  kStateStandby = 2U,
  kStateOperational = 3U,
  kStateMajorRecoverableFault = 4U,
  kStateMajorUnrecoverableFault = 5U,
  kStateDefault = 255U
} CipIdentityState;

/** @brief Declaration of the Identity object's structure type
 */
typedef struct {
  CipUint vendor_id; /**< Attribute 1: Vendor ID */
  CipUint device_type; /**< Attribute 2: Device Type */
  CipUint product_code; /**< Attribute 3: Product Code */
  CipRevision revision; /**< Attribute 4: Revision / CipUsint Major, CipUsint Minor */
  CipWord status; /**< Attribute 5: Status */
  CipWord ext_status;   /**< Attribute 5: last set extended status, needed for Status handling */
  CipUdint serial_number; /**< Attribute 6: Serial Number, has to be set prior to OpENer's network initialization */
  CipShortString product_name; /**< Attribute 7: Product Name */
  CipUsint state; /** Attribute 8: state, this member could control the Module Status LED blink pattern */
} CipIdentityObject;


/* global public variables */
extern CipIdentityObject g_identity;


/* public functions */
/** @brief CIP Identity object constructor
 *
 * @returns kEipStatusError if the class could not be created, otherwise kEipStatusOk
 */
EipStatus CipIdentityInit(void);

void CipIdentitySetStatusFlags(const CipWord status_flags);
void CipIdentityClearStatusFlags(const CipWord status_flags);
void CipIdentitySetExtendedDeviceStatus(
  CipIdentityExtendedStatus extended_status);

#endif /* OPENER_CIPIDENTITY_H_ */
