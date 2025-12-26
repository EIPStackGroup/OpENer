/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef PORTS_POSIX_SAMPLE_APPLICATION_OPENER_USER_CONF_H_
#define PORTS_POSIX_SAMPLE_APPLICATION_OPENER_USER_CONF_H_

/** @file POSIX/sample_application/opener_user_conf.h
 * @brief OpENer POSIX platform configuration setup
 *
 * This file contains POSIX-specific includes and configuration.
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
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "core/typedefs.h"

#ifdef OPENER_UNIT_TEST
#include "tests/test_assert.h"
#endif /* OPENER_UNIT_TEST */

/* Include common configuration shared across all platforms */
#include "ports/opener_user_conf_common.h"

#endif  // PORTS_POSIX_SAMPLE_APPLICATION_OPENER_USER_CONF_H_
