/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef SRC_PORTS_STM32_SAMPLE_APPLICATION_OPENER_USER_CONF_H_
#define SRC_PORTS_STM32_SAMPLE_APPLICATION_OPENER_USER_CONF_H_

/** @file STM32/sample_application/opener_user_conf.h
 * @brief OpENer STM32 platform configuration setup
 *
 * This file contains STM32-specific includes and configuration.
 * Common configuration is provided by opener_user_conf_common.h.
 *
 * Platform specific network include files are provided here.
 * OpENer needs definitions for the following data-types
 * and functions:
 *    - struct sockaddr_in
 *    - AF_INET
 *    - INADDR_ANY
 *    - htons
 *    - ntohl
 *    - inet_addr
 */
#include <assert.h>

/* Conflict resolution: in _default_fcntl.h defined as 0x4000,
 * but lwip defines 1 in sockets.h */
#undef O_NONBLOCK

#include "FreeRTOS.h"
#include "core/typedefs.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "lwip/arch.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "task.h"  // NOLINT(build/include_subdir)

#ifdef OPENER_UNIT_TEST
#include "tests/test_assert.h"
#endif /* OPENER_UNIT_TEST */

/* Include common configuration shared across all platforms */
#include "ports/opener_user_conf_common.h"

#endif  // SRC_PORTS_STM32_SAMPLE_APPLICATION_OPENER_USER_CONF_H_
