/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvdata.h
 *  @brief This file provides the interface to load all needed Non Volatile data.
 *
 */

#ifndef NVDATA_H_
#define NVDATA_H_

#include "typedefs.h"
#include "ciptypes.h"

EipStatus NvdataLoad(void);

EipStatus NvQosSetCallback
(
  CipInstance *const instance,
  CipAttributeStruct *const attribute,
  CipByte service
);

EipStatus NvTcpipSetCallback
(
    CipInstance *const instance,
    CipAttributeStruct *const attribute,
    CipByte service
);

#endif  /* ifndef NVDATA_H_ */
