/*******************************************************************************
 * Copyright (c) 2023, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_GEN_KEY_H
#define OPENER_GEN_KEY_H

#include "typedefs.h"
#include "ciptypes.h"

/* ********************************************************************
 * public functions
 */
/** @brief Key generation
 *
 *  @return Status
 */
int MbedtlsGenerateKey(void);

#endif  // OPENER_GEN_KEY_H
