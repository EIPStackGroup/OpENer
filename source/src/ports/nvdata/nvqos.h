/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvqos.h
 *  @brief This file provides the interface to handle QoS object's NV data.
 *
 */
#ifndef SOURCE_SRC_PORTS_NVDATA_NVQOS_H_
#define SOURCE_SRC_PORTS_NVDATA_NVQOS_H_
#include "cip/cipqos.h"
#include "src/typedefs.h"

int NvQosLoad(CipQosObject* p_qos);

int NvQosStore(const CipQosObject* p_qos);
#endif  // SOURCE_SRC_PORTS_NVDATA_NVQOS_H_
