/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#ifndef CIPIDENTITY_H_
#define CIPIDENTITY_H_

#include "typedefs.h"
#include "ciptypes.h"

#define CIP_IDENTITY_CLASS_CODE 0x01

/* global public variables */

/* public functions */
EIP_STATUS CIP_Identity_Init(void);

typedef struct
  {
    EIP_UINT8 MajorRevision;
    EIP_UINT8 MinorRevision;
  } S_CIP_Revision;


#endif /*CIPIDENTITY_H_*/
