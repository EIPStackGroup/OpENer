/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file conffile.h
 *  @brief The interface to handle a configuration file in an abstracted way.
 *
 */
#ifndef SOURCE_SRC_PORTS_NVDATA_CONFFILE_H_
#define SOURCE_SRC_PORTS_NVDATA_CONFFILE_H_
#include <stdbool.h>
#include <stdio.h>

#include "src/typedefs.h"

FILE* ConfFileOpen(const bool write, const char* const p_name);

EipStatus ConfFileClose(FILE** p_filep);
#endif  // SOURCE_SRC_PORTS_NVDATA_CONFFILE_H_
