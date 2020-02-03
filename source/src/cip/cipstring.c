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

#include "trace.h"
#include "opener_api.h"

CipString *FreeCipString(CipString *const cip_string) {
  if (NULL != cip_string) {
    if (NULL != cip_string->string) {
      CipFree(cip_string->string);
      cip_string->string = NULL;
      cip_string->length = 0;
    }
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipString!\n");
  }
  return cip_string;
}


CipString *SetCipStringByData(CipString *const cip_string,
                              size_t str_len,
                              const CipOctet *const data) {
  CipString *result = cip_string;

  (void) FreeCipString(cip_string);

  if (0 != str_len) {
    /* Allocate & clear +1 bytes for the trailing '\0' character. */
    cip_string->string = CipCalloc(str_len + 1, sizeof (CipOctet));
    if (NULL == cip_string->string) {
      result = NULL;
    } else {
      cip_string->length = str_len;
      memcpy(cip_string->string, data, str_len);
    }
  }
  return result;
}


CipString *SetCipStringByCstr(CipString *const cip_string,
                              const char *const string) {
    return SetCipStringByData(cip_string, strlen(string), (const CipOctet*)string);
}


CipShortString *FreeCipShortString(CipShortString *const cip_string) {
  if (NULL != cip_string) {
    if (NULL != cip_string->string) {
      CipFree(cip_string->string);
      cip_string->string = NULL;
      cip_string->length = 0;
    }
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipShortString!\n");
  }
  return cip_string;
}


CipShortString *SetCipShortStringByData(CipShortString *const cip_string,
                                        size_t str_len,
                                        const CipOctet *const data) {
  CipShortString *result = cip_string;

  (void) FreeCipShortString(cip_string);

  if (0 != str_len) {
    /* Allocate & clear +1 bytes for the trailing '\0' character. */
    cip_string->string = CipCalloc(str_len + 1, sizeof (CipOctet));
    if (NULL == cip_string->string) {
      result = NULL;
    } else {
      cip_string->length = str_len;
      memcpy(cip_string->string, data, str_len);
    }
  }
  return result;
}


CipShortString *SetCipShortStringByCstr(CipShortString *const cip_string,
                                        const char *const string) {
  return SetCipShortStringByData(cip_string, strlen(string),
                                 (const CipOctet*)string);
}
