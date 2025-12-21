/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file conffile.h
 *  @brief The interface to handle a configuration file in an abstracted way.
 *
 */
#ifndef PORTS_NVDATA_CONFFILE_H_
#define PORTS_NVDATA_CONFFILE_H_

#include <stdbool.h>
#include <stdio.h>

#include "core/typedefs.h"

FILE* ConfFileOpen(const bool write, const char* const p_name);

EipStatus ConfFileClose(FILE** p_filep);

#endif  // PORTS_NVDATA_CONFFILE_H_
