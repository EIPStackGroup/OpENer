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


CipString *SetCipStringByData
(
   CipString *p_cip_string,
   size_t str_len,
   const EipUint8 *p_data
);

CipString *SetCipStringByCstr
(
  CipString *p_cip_string,
  const char *p_string
);

CipShortString *SetCipShortStringByData
(
   CipShortString *p_cip_string,
   size_t str_len,
   const EipUint8 *p_data
);

CipShortString *SetCipShortStringByCstr
(
  CipShortString *p_cip_string,
  const char *p_string
);

#endif /* of OPENER_CIPSTRING_H_ */
