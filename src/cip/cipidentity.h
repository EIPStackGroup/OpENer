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

#define CIP_IDENTITY_STATE_NONEXISTANT          0x00
#define CIP_IDENTITY_STATE_SELF_TEST            0x01
#define CIP_IDENTITY_STATE_STANDBY              0x02
#define CIP_IDENTITY_STATE_OPERATIONAL          0x03
#define CIP_IDENTITY_STATE_MAJOR_RECOV_FAULT    0x04
#define CIP_IDENTITY_STATE_MAJOR_UNRECOV_FAULT  0x05
#define CIP_IDENTITY_STATE_DEFAULT              0xFF

/* global public variables */

/* public functions */
EIP_STATUS
CIP_Identity_Init(void);

#endif /*CIPIDENTITY_H_*/
