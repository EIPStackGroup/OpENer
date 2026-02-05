/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_PORTS_STM32_NETWORKCONFIG_H_
#define SRC_PORTS_STM32_NETWORKCONFIG_H_
/** @file STM32/networkconfig.h
 * @brief Network configuration for STM32 platform
 * This file contains the network configuration for the STM32 platform.
 */

#define IfaceLinkIsUp(iface) netif_is_link_up(iface)

#endif  // SRC_PORTS_STM32_NETWORKCONFIG_H_
