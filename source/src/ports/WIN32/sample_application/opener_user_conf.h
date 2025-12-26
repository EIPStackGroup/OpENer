/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef PORTS_WIN32_SAMPLE_APPLICATION_OPENER_USER_CONF_H_
#define PORTS_WIN32_SAMPLE_APPLICATION_OPENER_USER_CONF_H_

/** @file WIN32/sample_application/opener_user_conf.h
 * @brief OpENer WIN32 platform configuration setup
 *
 * This file contains WIN32-specific includes and configuration.
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
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

/* MSVC compatibility: in_port_t must match network stack's unsigned short */
typedef unsigned short in_port_t;  // NOLINT(runtime/int)

#include "core/typedefs.h"

#ifdef OPENER_UNIT_TEST
#include "tests/test_assert.h"
#endif /* OPENER_UNIT_TEST */

/* Include common configuration shared across all platforms */
#include "ports/opener_user_conf_common.h"

#endif  // PORTS_WIN32_SAMPLE_APPLICATION_OPENER_USER_CONF_H_
