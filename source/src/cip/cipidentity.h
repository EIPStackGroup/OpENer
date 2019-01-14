/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPIDENTITY_H_
#define OPENER_CIPIDENTITY_H_

#include "typedefs.h"
#include "ciptypes.h"

static const int kIdentityClassCode = 0x01;

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
  kMajorRecoveralbeFault = 0x0400, /**< Indicates that the device detected a
                                      fault with itself,which was thought to be recoverable. The device changed
                                      to the "Major Recoverable Fault" state */
  kMajorUnrecoverableFault = 0x0800 /**< Indicates that the device detected a
                                       fault with itself,which was thought to be recoverable. The device changed
                                       to the "Major Unrecoverable Fault" state */
} CipIdentityStatus;

typedef enum {
  kSelftestingUnknown = 0x0000,
  kFirmwareUpdateInProgress = 0x0010,
  kStatusAtLeastOneFaultedIoConnection = 0x0020,
  kNoIoConnectionsEstablished = 0x0030,
  kNonVolatileConfigurationBad = 0x0040,
  kMajorFault = 0x0050,
  kAtLeastOneIoConnectionInRunMode = 0x0060,
  kAtLeastOneIoConnectionEstablishedAllInIdleMode = 0x0070
} CipIdentityExtendedStatus;

/* global public variables */

/* public functions */
/** @brief CIP Identity object constructor
 *
 * @returns kEipStatusError if the class could not be created, otherwise kEipStatusOk
 */
EipStatus CipIdentityInit(void);

void CipIdentitySetExtendedDeviceStatus(
  CipIdentityExtendedStatus extended_status);

#endif /* OPENER_CIPIDENTITY_H_ */
