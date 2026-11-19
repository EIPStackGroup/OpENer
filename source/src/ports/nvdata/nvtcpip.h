/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvtcpip.h
 *  @brief This file provides the interface to handle TCP/IP object's NV data.
 *
 */
#ifndef SOURCE_SRC_PORTS_NVDATA_NVTCPIP_H_
#define SOURCE_SRC_PORTS_NVDATA_NVTCPIP_H_
#include "cip/ciptcpipinterface.h"
#include "src/typedefs.h"

int NvTcpipLoad(CipTcpIpObject* p_tcp_ip);

int NvTcpipStore(const CipTcpIpObject* p_tcp_ip);
#endif  // SOURCE_SRC_PORTS_NVDATA_NVTCPIP_H_
