/*******************************************************************************
 * Copyright (c) 2023, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_CERT_WRITE_H
#define OPENER_CERT_WRITE_H

#include "typedefs.h"
#include "ciptypes.h"


/* ********************************************************************
 * public functions
 */
/** @brief Certificate generation
 *
 *  @param subject_name_input  subject name
 *  @param serial_number_input  serial number in subject name
 *  @return status
 */
int MbedtlsGenerateCertificate(char *subject_name_input[], char *serial_number_input);

#endif  // OPENER_CERT_WRITE_H
