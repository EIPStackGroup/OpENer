/*******************************************************************************
 * Copyright (c) 2022, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>

#include "cipstringi.h"

#include "opener_api.h"
#include "cipstring.h"
#include "trace.h"
#include "endianconv.h"

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
    case kCipString:
      return to->string = CipCalloc(1, sizeof(CipString) );
    case kCipString2:
      return to->string = CipCalloc(1, sizeof(CipString2) );
    case kCipStringN:
      return to->string = CipCalloc(1, sizeof(CipStringN) );
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

void CipStringIDecodeFromMessage(CipStringI *data_to,
                                 CipMessageRouterRequest *const message_router_request)
{

  CipStringI *target_stringI = data_to;

  target_stringI->number_of_strings = GetUsintFromMessage(
    &message_router_request->data);

  target_stringI->array_of_string_i_structs = CipCalloc(
    target_stringI->number_of_strings, sizeof(CipStringIStruct) );

  for (size_t i = 0; i < target_stringI->number_of_strings; ++i) {

    target_stringI->array_of_string_i_structs[i].language_char_1 =
      GetUsintFromMessage(&message_router_request->data);
    target_stringI->array_of_string_i_structs[i].language_char_2 =
      GetUsintFromMessage(&message_router_request->data);
    target_stringI->array_of_string_i_structs[i].language_char_3 =
      GetUsintFromMessage(&message_router_request->data);
    target_stringI->array_of_string_i_structs[i].char_string_struct =
      GetUsintFromMessage(&message_router_request->data);
    target_stringI->array_of_string_i_structs[i].character_set =
      GetUintFromMessage(&message_router_request->data);

    switch (target_stringI->array_of_string_i_structs[i].char_string_struct) {
      case kCipShortString: {
        target_stringI->array_of_string_i_structs[i].string = CipCalloc(1,
                                                                        sizeof(
                                                                          CipShortString) );
        CipShortString *short_string =
          (CipShortString *) (target_stringI->array_of_string_i_structs[i].
                              string);
        CipUsint length = GetUsintFromMessage(
          &message_router_request->data);
        SetCipShortStringByData(short_string, length,
                                message_router_request->data);
        message_router_request->data += length;

      }
      break;
      case kCipString: {
        target_stringI->array_of_string_i_structs[i].string = CipCalloc(1,
                                                                        sizeof(
                                                                          CipString) );
        CipString *const string =
          (CipString *const ) target_stringI->array_of_string_i_structs[i].
          string;
        CipUint length = GetUintFromMessage(&message_router_request->data);
        SetCipStringByData(string, length, message_router_request->data);
        message_router_request->data += length;
      }
      break;
      case kCipString2: {
        target_stringI->array_of_string_i_structs[i].string = CipCalloc(1,
                                                                        sizeof(
                                                                          CipString2) );
        CipString2 *const string =
          (CipString2 *const ) target_stringI->array_of_string_i_structs[i].
          string;
        CipUint length = GetUintFromMessage(&message_router_request->data);
        SetCipString2ByData(string, length, message_router_request->data);
        message_router_request->data += length * 2 * sizeof(CipOctet);
      }
      break;
      case kCipStringN: {
        CipUint size = GetUintFromMessage(&message_router_request->data);
        CipUint length = GetUintFromMessage(&message_router_request->data);

        target_stringI->array_of_string_i_structs[i].string = CipCalloc(1,
                                                                        sizeof(
                                                                          CipStringN) );
        CipStringN *const string =
          (CipStringN *const ) target_stringI->array_of_string_i_structs[i].
          string;
        SetCipStringNByData(string, length, size,
                            message_router_request->data);
        message_router_request->data += length * size;
      }
      break;
      default:
        OPENER_TRACE_ERR("CIP File: No valid String type received!\n");
    }
  }       //end for
}

bool CipStringICompare(const CipStringI *const stringI_1,
                       const CipStringI *const stringI_2) {

  /*loop through struct 1 strings*/
  for (size_t i = 0; i < stringI_1->number_of_strings; ++i) {
    // String 1
    void *string_1 = stringI_1->array_of_string_i_structs[i].string;
    void *string_1_data = NULL;
    CipUint len_1 = 0; //size of string-struct in bytes

    switch (stringI_1->array_of_string_i_structs[i].char_string_struct) {
      case kCipShortString: {
        len_1 = ((CipShortString *)string_1)->length;
        string_1_data = ((CipShortString *)string_1)->string;
      }
      break;
      case kCipString: {
        len_1 = ((CipString *)string_1)->length;
        string_1_data = ((CipString *)string_1)->string;
      }

      break;
      case kCipString2: {
        len_1 = ((CipString2 *)string_1)->length * 2;
        string_1_data = ((CipString2 *)string_1)->string;
      }
      break;
      case kCipStringN: {
        CipUint length = ((CipStringN *)string_1)->length;
        CipUint size = ((CipStringN *)string_1)->size; //bytes per symbol
        len_1 = length * size;
        string_1_data = ((CipStringN *)string_1)->string;

      }
      break;
      default:
        OPENER_TRACE_ERR("CIP File: No valid String type received!\n");
    }

    /*loop through struct 2 strings*/
    for (size_t j = 0; j < stringI_2->number_of_strings; ++j) {
      // String 2
      void *string_2 = stringI_2->array_of_string_i_structs[j].string;
      void *string_2_data = NULL;
      CipUint len_2 = 0; //size of string-struct in bytes

      switch (stringI_2->array_of_string_i_structs[j].char_string_struct) {
        case kCipShortString: {
          len_2 = ((CipShortString *)string_2)->length;
          string_2_data = ((CipShortString *)string_2)->string;
        }
        break;
        case kCipString: {
          len_2 = ((CipString *)string_2)->length;
          string_2_data = ((CipString *)string_2)->string;
        }

        break;
        case kCipString2: {
          len_2 = ((CipString2 *)string_2)->length * 2;
          string_2_data = ((CipString2 *)string_2)->string;
        }
        break;
        case kCipStringN: {
          CipUint length = ((CipStringN *)string_2)->length;
          CipUint size = ((CipStringN *)string_2)->size; //bytes per symbol
          len_2 = length * size;
          string_2_data = ((CipStringN *)string_2)->string;

        }
        break;
        default:
          OPENER_TRACE_ERR("CIP File: No valid String type received!\n");
      }

      /*compare strings*/ //TODO: compare works only for same data types
      if (len_1 == len_2 && string_1_data != NULL && string_2_data != NULL) {
        if (0 == memcmp(string_1_data, string_2_data, len_1) ) {
          return true;
        }
      }
    }             //end for 1
  }       //end for 2

  return false;
}

