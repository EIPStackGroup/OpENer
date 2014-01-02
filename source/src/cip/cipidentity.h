/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef CIPIDENTITY_H_
#define CIPIDENTITY_H_

#include "typedefs.h"
#include "ciptypes.h"

#define CIP_IDENTITY_CLASS_CODE 0x01

#define CIP_IDENTITY_STATUS_OWNED 0x0001
#define CIP_IDENTITY_STATUS_CONFIGURED 0x0004
#define CIP_IDENTITY_STATUS_MINOR_RECOV_FLT 0x0100
#define CIP_IDENTITY_STATUS_MINOR_UNRECOV_FLT 0x0200
#define CIP_IDENTITY_STATUS_MAJOR_RECOV_FLT 0x0400
#define CIP_IDENTITY_STATUS_MAJOR_UNRECOV_FLT 0x0800

#define CIP_IDENTITY_EXTENDED_STATUS_SELFTESTING_UNKNOWN                                     0x0000
#define CIP_IDENTITY_EXTENDED_STATUS_FIRMEWARE_UPDATE_IN_PROGRESS                            0x0010
#define CIP_IDENTITY_EXTENDED_STATUS_AT_LEAST_ONE_FAULTED_IO_CONNECTION                      0x0020
#define CIP_IDENTITY_EXTENDED_STATUS_NO_IO_CONNECTIONS_ESTABLISHED                           0x0030
#define CIP_IDENTITY_EXTENDED_STATUS_NON_VOLATILE_CONFIGURATION_BAD                          0x0040
#define CIP_IDENTITY_EXTENDED_STATUS_MAJOR_FAULT                                             0x0050
#define CIP_IDENTITY_EXTENDED_STATUS_AT_LEAST_ONE_IO_CONNECTION_IN_RUN_MODE                  0x0060
#define CIP_IDENTITY_EXTENDED_STATUS_AT_LEAST_ONE_IO_CONNECTION_ESTABLISHED_ALL_IN_IDLE_MODE 0x0070


/* global public variables */

/* public functions */
EIP_STATUS
CIP_Identity_Init(void);

#endif /*CIPIDENTITY_H_*/
