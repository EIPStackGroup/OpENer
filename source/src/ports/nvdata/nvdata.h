/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvdata.h
 *  @brief This file provides the interface to load all needed Non Volatile
 * data.
 *
 */

#ifndef SRC_PORTS_NVDATA_NVDATA_H_
#define SRC_PORTS_NVDATA_NVDATA_H_

#include "cip/ciptypes.h"
#include "core/typedefs.h"

EipStatus NvdataLoad(void);

EipStatus NvQosSetCallback(CipInstance* const instance,
                           CipAttributeStruct* const attribute,
                           CipByte service);

EipStatus NvTcpipSetCallback(CipInstance* const instance,
                             CipAttributeStruct* const attribute,
                             CipByte service);

#endif  // SRC_PORTS_NVDATA_NVDATA_H_
