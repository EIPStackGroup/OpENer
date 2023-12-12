/*******************************************************************************
 * Copyright (c) 2023, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_CERT_REQ_H
#define OPENER_CERT_REQ_H

#include "typedefs.h"
#include "ciptypes.h"


/* ********************************************************************
 * public functions
 */
/** @brief  Certificate signing request generation
 *
 *  @param short_strings structure containing CSR Parameters
 *  @return status
 */
int MbedtlsWriteCSR(CipShortString *short_strings);

#endif  // OPENER_CERT_REQ_H
