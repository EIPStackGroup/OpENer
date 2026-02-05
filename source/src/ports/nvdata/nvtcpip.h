/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvtcpip.h
 *  @brief This file provides the interface to handle TCP/IP object's NV data.
 *
 */
#ifndef SRC_PORTS_NVDATA_NVTCPIP_H_
#define SRC_PORTS_NVDATA_NVTCPIP_H_

#include "cip/ciptcpipinterface.h"
#include "core/typedefs.h"

int NvTcpipLoad(CipTcpIpObject* p_tcp_ip);

int NvTcpipStore(const CipTcpIpObject* p_tcp_ip);

#endif  // SRC_PORTS_NVDATA_NVTCPIP_H_
