/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvqos.h
 *  @brief This file provides the interface to handle QoS object's NV data.
 *
 */
#ifndef SRC_PORTS_NVDATA_NVQOS_H_
#define SRC_PORTS_NVDATA_NVQOS_H_

#include "cip/cipqos.h"
#include "core/typedefs.h"

int NvQosLoad(CipQosObject* p_qos);

int NvQosStore(const CipQosObject* p_qos);

#endif  // SRC_PORTS_NVDATA_NVQOS_H_
