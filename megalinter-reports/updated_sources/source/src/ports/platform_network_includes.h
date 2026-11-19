/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef SOURCE_SRC_PORTS_PLATFORM_NETWORK_INCLUDES_H_
#define SOURCE_SRC_PORTS_PLATFORM_NETWORK_INCLUDES_H_

#if defined(__unix__) || defined(__APPLE__)
#include "ports/POSIX/platform_network_includes.h"
#elif defined(__MINGW32__) || defined(__MINGW64__)
#include "ports/MINGW/platform_network_includes.h"
#elif defined(_WIN32)
#include "ports/WIN32/platform_network_includes.h"
#else
#include "ports/STM32/platform_network_includes.h"
#endif

#endif  // SOURCE_SRC_PORTS_PLATFORM_NETWORK_INCLUDES_H_
