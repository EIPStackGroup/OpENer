/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef PORTS_MINGW_SAMPLE_APPLICATION_OPENER_USER_CONF_H_
#define PORTS_MINGW_SAMPLE_APPLICATION_OPENER_USER_CONF_H_

/** @file
 * @brief OpENer MinGW platform configuration setup
 *
 * This file contains MinGW-specific configuration.
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

#include "core/typedefs.h"

#ifdef OPENER_UNIT_TEST
#include "tests/test_assert.h"
#endif /* OPENER_UNIT_TEST */

/* MinGW does not define POSIX in_port_t; must match WinSock's unsigned short */
typedef unsigned short in_port_t;  // NOLINT(runtime/int)

/* Include common configuration shared across all platforms */
#include "ports/opener_user_conf_common.h"

#endif  // PORTS_MINGW_SAMPLE_APPLICATION_OPENER_USER_CONF_H_
