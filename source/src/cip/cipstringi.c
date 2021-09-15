/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>

#include "cipstringi.h"

#include "opener_api.h"
#include "cipstring.h"
#include "trace.h"

void CipStringIDelete(CipStringI *const string) {
  for(size_t i = 0; i < string->number_of_strings; ++i) {
    string->array_of_string_i_structs[i].language_char_1 = '\0';
    string->array_of_string_i_structs[i].language_char_2 = '\0';
    string->array_of_string_i_structs[i].language_char_3 = '\0';
    string->array_of_string_i_structs[i].character_set = '\0';
    switch(string->array_of_string_i_structs[i].char_string_struct) {
      case kCipShortString:
        ClearCipShortString(
           (CipShortString *) &string->array_of_string_i_structs[i].string );
        break;
      case kCipString:
        ClearCipString(
           (CipString *) &string->array_of_string_i_structs[i].string );
        break;
      case kCipString2:
        ClearCipString2(
           (CipString2 *) &string->array_of_string_i_structs[i].string );
        break;
      case kCipStringN:
        ClearCipStringN(
           (CipStringN *) &string->array_of_string_i_structs[i].string );
        break;
      default:
        OPENER_TRACE_ERR("CIP File: No valid String type received!\n");
    }
    string->array_of_string_i_structs[i].char_string_struct = 0x00;
  }
  string->number_of_strings = 0;
  CipFree(string->array_of_string_i_structs);
  string->array_of_string_i_structs = NULL;
}

bool CipStringIsAnyStringEmpty(const CipStringI *const string) {
  for(size_t i = 0; i < string->number_of_strings; ++i) {
    size_t length = 0;
    void *pointer_to_string = string->array_of_string_i_structs[i].string;
    switch(string->array_of_string_i_structs[i].char_string_struct) {
      case kCipShortString:
        length = ( (CipShortString *) pointer_to_string )->length;
        break;
      case kCipString:
        length = ( (CipString *) pointer_to_string )->length;
        break;
      case kCipString2:
        length = ( (CipString2 *) pointer_to_string )->length;
        break;
      case kCipStringN:
        length = ( (CipStringN *) pointer_to_string )->length;
        break;
      default:
        OPENER_TRACE_ERR("CIP File: No valid String type received!\n");
    }
    if(0 == length) {
      return true;
    }
  }
  return false;
}

void *CipStringICreateStringStructure(CipStringIStruct *const to) {
  switch(to->char_string_struct) {
    case kCipShortString:
      return to->string = CipCalloc(1, sizeof(CipShortString) );
      break;
    case kCipString:
      return to->string = CipCalloc(1, sizeof(CipString) );
      break;
    case kCipString2:
      return to->string = CipCalloc(1, sizeof(CipString2) );
      break;
    case kCipStringN:
      return to->string = CipCalloc(1, sizeof(CipStringN) );
      break;
    default:
      OPENER_TRACE_ERR("CIP File: No valid String type received!\n");
  }
  return NULL;
}

void CipStringIDeepCopyInternalString(CipStringIStruct *const to,
                                      const CipStringIStruct *const from) {
  switch(to->char_string_struct) {
    case kCipShortString: {
      CipShortString *toString = (CipShortString *) to->string;
      CipShortString *fromString = (CipShortString *) from->string;
      toString->length = fromString->length;
      toString->string = CipCalloc(toString->length, sizeof(CipOctet) );
      memcpy(toString->string,
             fromString->string,
             sizeof(CipOctet) * toString->length);
    }
    break;
    case kCipString: {
      CipString *toString = (CipString *) to->string;
      CipString *fromString = (CipString *) from->string;
      toString->length = fromString->length;
      toString->string = CipCalloc(toString->length, sizeof(CipOctet) );
      memcpy(toString->string,
             fromString->string,
             sizeof(CipOctet) * toString->length);
    }
    break;
    case kCipString2: {
      CipString2 *toString = (CipString2 *) to->string;
      CipString2 *fromString = (CipString2 *) from->string;
      toString->length = fromString->length;
      toString->string = CipCalloc(toString->length, 2 * sizeof(CipOctet) );
      memcpy(toString->string,
             fromString->string,
             2 * sizeof(CipOctet) * toString->length);
    }
    break;
    case kCipStringN: {
      CipStringN *toString = (CipStringN *) to->string;
      CipStringN *fromString = (CipStringN *) from->string;
      toString->length = fromString->length;
      toString->size = fromString->size;
      toString->string =
        CipCalloc(toString->length, toString->size * sizeof(CipOctet) );
      memcpy(toString->string, fromString->string,
             toString->size * sizeof(CipOctet) * toString->length);
    }
    break;
    default:
      OPENER_TRACE_ERR("CIP File: No valid String type received!\n");
  }
}

void CipStringICopy(CipStringI *const to,
                    const CipStringI *const from) {
  to->number_of_strings = from->number_of_strings;
  to->array_of_string_i_structs =
    CipCalloc(to->number_of_strings, sizeof(CipStringIStruct) );
  for(size_t i = 0; i < to->number_of_strings; ++i) {
    CipStringIStruct *const toStruct = to->array_of_string_i_structs + i;
    CipStringIStruct *const fromStruct = from->array_of_string_i_structs + i;
    toStruct->language_char_1 = fromStruct->language_char_1;
    toStruct->language_char_2 = fromStruct->language_char_2;
    toStruct->language_char_3 = fromStruct->language_char_3;
    toStruct->char_string_struct = fromStruct->char_string_struct;
    toStruct->character_set = fromStruct->character_set;
    CipStringICreateStringStructure(toStruct);
    CipStringIDeepCopyInternalString(toStruct, fromStruct);
  }
}
