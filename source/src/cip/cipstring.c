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

CipStringN *ClearCipStringN(CipStringN *const cip_string) {
  if(NULL != cip_string) {
    if(NULL != cip_string->string) {
      CipFree(cip_string->string);
    }
    cip_string->string = NULL;
    cip_string->length = 0;
    cip_string->size = 0;
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipString2!\n");
  }
  return cip_string;
}

void FreeCipStringN(CipStringN *const cip_string) {
  if(NULL != cip_string) {
    ClearCipStringN(cip_string);
    CipFree(cip_string);
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipString2!\n");
  }
}

/** @brief Sets length and data for a CipStringN based on an octet stream, symbol length, and symbol width
 *
 * @param cip_string The string to be set
 * @param str_len Amount of CipStringN symbols
 * @param size Width of the CipStringN symbols
 * @param data The octet stream
 *
 * @return The CipStringN address
 */
CipStringN *SetCipStringNByData(CipStringN *const cip_string,
                                CipUint str_len,
                                CipUint size,
                                const CipOctet *const data) {
  CipStringN *result = cip_string;

  (void) ClearCipStringN(cip_string);

  if(0 != str_len) {
    /* No trailing '\0' character! */
    cip_string->length = str_len;
    cip_string->size = size;
    cip_string->string = CipCalloc(cip_string->length,
                                   cip_string->size * sizeof(CipOctet) );
    if(NULL == cip_string->string) {
      result = NULL;
      cip_string->length = 0;
      cip_string->size = 0;
    } else {
      memcpy(cip_string->string,
             data,
             cip_string->length * cip_string->size * sizeof(CipOctet) );
    }
  }
  return result;
}

CipStringN *SetCipStringNByCstr(CipStringN *const cip_string,
                                const char *const string,
                                const CipUint symbol_size) {
  if(0 != strlen(string) % symbol_size) {
    OPENER_TRACE_ERR("Not enough octets for %d width StringN\n", symbol_size);
    return cip_string;
  }
  /* We expect here, that the length of the string is the total length in Octets */
  return SetCipStringNByData(cip_string,
                             (CipUint) strlen(string) / symbol_size,
                             symbol_size,
                             (const CipOctet *) string);
}

void FreeCipString2(CipString2 *const cip_string) {
  if(NULL != cip_string) {
    ClearCipString2(cip_string);
    CipFree(cip_string);
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipString2!\n");
  }
}

/** @brief Frees a CipString2 structure
 *
 * @param cip_string The CipString2 structure to be freed
 *
 * @return Freed CipString2 structure
 *
 */
CipString2 *ClearCipString2(CipString2 *const cip_string) {
  if(NULL != cip_string) {
    if(NULL != cip_string->string) {
      CipFree(cip_string->string);
      cip_string->string = NULL;
      cip_string->length = 0;
    }
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipString2!\n");
  }
  return cip_string;
}

CipString2 *SetCipString2ByData(CipString2 *const cip_string,
                                CipUint str_len,
                                const CipOctet *const data) {
  CipString2 *result = cip_string;

  (void) ClearCipString2(cip_string);

  if(0 != str_len) {
    /* No trailing '\0' character! */
    cip_string->string = CipCalloc(str_len, 2 * sizeof(CipOctet) );
    if(NULL == cip_string->string) {
      result = NULL;
    } else {
      cip_string->length = str_len;
      memcpy(cip_string->string, data, str_len * 2 * sizeof(CipOctet) );
    }
  }
  return result;
}

CipString2 *SetCipString2ByCstr(CipString2 *const cip_string,
                                const char *const string) {
  return SetCipString2ByData(cip_string, (CipUint) strlen(string) / 2,
                             (const CipOctet *) string);
}

CipString *ClearCipString(CipString *const cip_string) {
  if(NULL != cip_string) {
    if(NULL != cip_string->string) {
      CipFree(cip_string->string);
      cip_string->string = NULL;
      cip_string->length = 0;
    }
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipString!\n");
  }
  return cip_string;
}

void FreeCipString(CipString *const cip_string) {
  if(NULL != cip_string) {
    ClearCipString(cip_string);
    CipFree(cip_string);
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipString2!\n");
  }
}

CipString *SetCipStringByData(CipString *const cip_string,
                              CipUint str_len,
                              const CipOctet *const data) {
  CipString *result = cip_string;

  (void) ClearCipString(cip_string);

  if(0 != str_len) {
    /* No trailing '\0' character. */
    cip_string->string = CipCalloc(str_len, sizeof(CipOctet) );
    if(NULL == cip_string->string) {
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
  return SetCipStringByData(cip_string, (CipUint) strlen(string),
                            (const CipOctet *) string);
}

CipShortString *ClearCipShortString(CipShortString *const cip_string) {
  if(NULL != cip_string) {
    if(NULL != cip_string->string) {
      CipFree(cip_string->string);
      cip_string->string = NULL;
      cip_string->length = 0;
    }
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipShortString!\n");
  }
  return cip_string;
}

/** @brief Frees a CipShortString structure
 *
 * @param cip_string The CipShortString structure to be freed
 *
 * @return Freed CipShortString structure
 *
 */
void FreeCipShortString(CipShortString *const cip_string) {
  if(NULL != cip_string) {
    ClearCipShortString(cip_string);
    CipFree(cip_string);
  } else {
    OPENER_TRACE_ERR("Trying to free NULL CipString2!\n");
  }
}

/** @brief Sets length and data for a CipShortString based on an octet stream and symbol length
 *
 * @param cip_string The CipShortString to be set
 * @param str_len Amount of CipShortString symbols
 * @param data The octet stream
 *
 * @return The CipShortString address
 */
CipShortString *SetCipShortStringByData(CipShortString *const cip_string,
                                        const CipUsint str_len,
                                        const CipOctet *const data) {
  CipShortString *result = cip_string;

  (void) ClearCipShortString(cip_string);

  if(0 != str_len) {
    /* No trailing '\0' character. */
    cip_string->string = CipCalloc(str_len, sizeof(CipOctet) );
    if(NULL == cip_string->string) {
      result = NULL;
    } else {
      cip_string->length = str_len;
      memcpy(cip_string->string, data, str_len);
    }
  }
  return result;
}

/** @brief Copies the content of C-string to a CipShortString under the expectation, that each C-String element is a CipShortString octet
 *
 * @param cip_string Target CipShortString
 * @param string Source C-string
 *
 * @return Target CipShortString
 *
 */
CipShortString *SetCipShortStringByCstr(CipShortString *const cip_string,
                                        const char *const string) {
  return SetCipShortStringByData(cip_string, (CipUsint) strlen(string),
                                 (const CipOctet *) string);
}

/* Ensures buf is NUL terminated, provided initial validation is successful */
int GetCstrFromCipShortString(CipShortString *const string, char *buf, size_t len) {
  size_t num;
  int rc = 0;

  if (!string || !buf || len < 1)
    return -1;

  num = (size_t)string->length;
  if (len <= num) {
    rc = (int)(num - len - 1);
    num = len - 1;
  }
  len = num;

  memcpy(buf, string->string, num);
  buf[len] = 0;

  return rc;
}
