/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file
 * @brief Implements functions to operate on CIP string types
 *
 * Some functions to create CIP string types from C strings or data buffers.
 */

#include "cipstring.h"

#include <stdlib.h>
#include <string.h>

#include "opener_api.h"

CipString *FreeCipString
(
  CipString *p_cip_string
)
{
  if (NULL != p_cip_string->string) {
    CipFree(p_cip_string->string);
    p_cip_string->string = NULL;
    p_cip_string->length = 0;
  }
  return p_cip_string;
}


CipString *SetCipStringByData
(
  CipString *p_cip_string,
  size_t str_len,
  const EipUint8 *p_data
)
{
  CipString   *p_result = p_cip_string;

  (void) FreeCipString(p_cip_string);

  if (str_len) {
    /* Allocate & clear +1 bytes for the trailing '\0' character. */
    p_cip_string->string = CipCalloc(str_len+1, sizeof (EipUint8));
    if (NULL == p_cip_string->string) {
      p_result = NULL;
    } else {
      p_cip_string->length = str_len;
      memcpy(p_cip_string->string, p_data, str_len);
    }
  }
  return p_result;
}


CipString *SetCipStringByCstr
(
  CipString *p_cip_string,
  const char *p_string
)
{
    return SetCipStringByData(p_cip_string, strlen(p_string),
                              (const EipByte *)p_string);
}


CipShortString *FreeCipShortString
(
  CipShortString *p_cip_string
)
{
  if (NULL != p_cip_string->string) {
    CipFree(p_cip_string->string);
    p_cip_string->string = NULL;
    p_cip_string->length = 0;
  }
  return p_cip_string;
}


CipShortString *SetCipShortStringByData
(
  CipShortString *p_cip_string,
  size_t str_len,
  const EipUint8 *p_data
)
{
  CipShortString   *p_result = p_cip_string;

  (void) FreeCipShortString(p_cip_string);

  if (str_len) {
    /* Allocate & clear +1 bytes for the trailing '\0' character. */
    p_cip_string->string = CipCalloc(str_len+1, sizeof (EipUint8));
    if (NULL == p_cip_string->string) {
      p_result = NULL;
    } else {
      p_cip_string->length = str_len;
      memcpy(p_cip_string->string, p_data, str_len);
    }
  }
  return p_result;
}


CipShortString *SetCipShortStringByCstr
(
  CipShortString *p_cip_string,
  const char *p_string
)
{
  return SetCipShortStringByData(p_cip_string, strlen(p_string),
                                 (const EipByte *)p_string);
}
