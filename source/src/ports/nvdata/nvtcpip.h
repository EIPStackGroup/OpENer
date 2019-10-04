/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvtcpip.h
 *  @brief This file provides the interface to handle TCP/IP object's NV data.
 *
 */
#ifndef _NVTCPIP_H_
#define _NVTCPIP_H_

#include "typedefs.h"

#include "ciptcpipinterface.h"

int NvTcpipLoad(CipTcpIpObject *p_tcp_ip);

int NvTcpipStore(const CipTcpIpObject *p_tcp_ip);

#endif  /* _NVTCPIP_H_ */
