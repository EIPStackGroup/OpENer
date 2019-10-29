/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file
 * @brief Declare functions to operate on CIP string types
 *
 * Some functions to create CIP string types from C strings or data buffers.
 */

#ifndef OPENER_CIPSTRING_H_
#define OPENER_CIPSTRING_H_

#include "typedefs.h"
#include "ciptypes.h"


CipString *FreeCipString(CipString *const cip_string);

CipString *SetCipStringByData(CipString *const cip_string,
                              size_t str_len,
                              const EipUint8 *const data);

CipString *SetCipStringByCstr(CipString *const cip_string,
                              const char *const string);

CipShortString *FreeCipShortString(CipShortString *const cip_string);

CipShortString *SetCipShortStringByData(CipShortString *const cip_string,
                                        size_t str_len,
                                        const CipOctet *const data);

CipShortString *SetCipShortStringByCstr(CipShortString *const cip_string,
                                        const char *const string);

#endif /* of OPENER_CIPSTRING_H_ */
