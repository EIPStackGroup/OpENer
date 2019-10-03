/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvqos.h
 *  @brief This file provides the interface to handle QoS object's NV data.
 *
 */
#ifndef _NVQOS_H_
#define _NVQOS_H_

#include "typedefs.h"

#include "cipqos.h"

int NvQosLoad(CipQosObject *p_qos);

int NvQosStore(const CipQosObject *p_qos);

#endif  /* _NVQOS_H_ */
