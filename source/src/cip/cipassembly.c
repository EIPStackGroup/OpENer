/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>
#include <stdbool.h>

#include "cipassembly.h"

#include "cipcommon.h"
#include "opener_api.h"
#include "trace.h"
#include "cipconnectionmanager.h"

/** @brief Retrieve the given data according to CIP encoding from the
 *              message buffer.
 *
 *  Implementation of the decode function for the SetAttributeSingle CIP service for Assembly
 *  Objects.
 *  Currently only supports Attribute 3 (CIP_BYTE_ARRAY) of an Assembly
 *  @param data pointer to value to be written.
 *  @param message_router_request pointer to the request where the data should be taken from
 *  @param message_router_response pointer to the response where status should be set
 *  @return length of taken bytes
 *          -1 .. error
 */
int DecodeCipAssemblyAttribute3(void *const data,
                                CipMessageRouterRequest *const message_router_request,
                                CipMessageRouterResponse *const message_router_response);

static EipStatus AssemblyPreGetCallback(CipInstance *const instance,
                                        CipAttributeStruct *const attribute,
                                        CipByte service);

static EipStatus AssemblyPostSetCallback(CipInstance *const instance,
                                         CipAttributeStruct *const attribute,
                                         CipByte service);

/** @brief Constructor for the assembly object class
 *
 *  Creates an initializes Assembly class or object instances
 *  @return Pointer to the created Assembly object
 */
CipClass *CreateAssemblyClass(void) {
  /* create the CIP Assembly object with zero instances */
  CipClass *assembly_class = CreateCipClass(kCipAssemblyClassCode, 0, /* # class attributes*/
                                            7, /* # highest class attribute number*/
                                            1, /* # class services*/
                                            2, /* # instance attributes*/
                                            4, /* # highest instance attribute number*/
                                            2, /* # instance services*/
                                            0, /* # instances*/
                                            "assembly", /* name */
                                            2, /* Revision, according to the CIP spec currently this has to be 2 */
                                            NULL); /* # function pointer for initialization*/
  if(NULL != assembly_class) {
    InsertService(assembly_class,
                  kGetAttributeSingle,
                  &GetAttributeSingle,
                  "GetAttributeSingle");

    InsertService(assembly_class,
                  kSetAttributeSingle,
                  &SetAttributeSingle,
                  "SetAttributeSingle");

    InsertGetSetCallback(assembly_class, AssemblyPreGetCallback, kPreGetFunc);
    InsertGetSetCallback(assembly_class, AssemblyPostSetCallback, kPostSetFunc);
  }

  return assembly_class;
}

/** @brief create the CIP Assembly object with zero instances
 *
 */
EipStatus CipAssemblyInitialize(void) {
  return ( NULL != CreateAssemblyClass() ) ? kEipStatusOk : kEipStatusError;
}

void ShutdownAssemblies(void) {
  const CipClass *const assembly_class = GetCipClass(kCipAssemblyClassCode);

  if(NULL != assembly_class) {
    const CipInstance *instance = assembly_class->instances;
    while(NULL != instance) {
      const CipAttributeStruct *const attribute = GetCipAttribute(instance, 3);
      if(NULL != attribute) {
        CipFree(attribute->data);
      }
      instance = instance->next;
    }
  }
}

CipInstance *CreateAssemblyObject(const CipInstanceNum instance_id,
                                  EipByte *const data,
                                  const EipUint16 data_length) {
  CipClass *assembly_class = GetCipClass(kCipAssemblyClassCode);
  if(NULL == assembly_class) {
    assembly_class = CreateAssemblyClass();
  }

  if(NULL == assembly_class) {
    return NULL;
  }

  CipInstance *const instance = AddCipInstance(assembly_class, instance_id); /* add instances (always succeeds (or asserts))*/

  CipByteArray *const assembly_byte_array = (CipByteArray *) CipCalloc(1,
                                                                       sizeof(
                                                                         CipByteArray) );
  if(assembly_byte_array == NULL) {
    return NULL; /*TODO remove assembly instance in case of error*/
  }

  assembly_byte_array->length = data_length;
  assembly_byte_array->data = data;

  InsertAttribute(instance,
                  3,
                  kCipByteArray,
                  EncodeCipByteArray,
                  DecodeCipAssemblyAttribute3,
                  assembly_byte_array,
                  kSetAndGetAble | kPreGetFunc | kPostSetFunc);
  /* Attribute 4 Number of bytes in Attribute 3 */

  InsertAttribute(instance, 4, kCipUint, EncodeCipUint,
                  NULL, &(assembly_byte_array->length), kGetableSingle);

  return instance;
}

EipStatus NotifyAssemblyConnectedDataReceived(CipInstance *const instance,
                                              const EipUint8 *const data,
                                              const size_t data_length) {
  /* empty path (path size = 0) need to be checked and taken care of in future */
  /* copy received data to Attribute 3 */
  const CipByteArray *const assembly_byte_array =
    (CipByteArray *) instance->attributes->data;
  if(assembly_byte_array->length != data_length) {
    OPENER_TRACE_ERR("wrong amount of data arrived for assembly object\n");
    return kEipStatusError; /*TODO question should we notify the application that wrong data has been received???*/
  } else {
    memcpy(assembly_byte_array->data, data, data_length);
    /* call the application that new data arrived */
  }

  return AfterAssemblyDataReceived(instance);
}

int DecodeCipAssemblyAttribute3(void *const data,
                                CipMessageRouterRequest *const message_router_request,
                                CipMessageRouterResponse *const message_router_response)
{
  CipInstance *const instance =
    GetCipInstance(GetCipClass(
                     message_router_request->request_path.class_id),
                   message_router_request->request_path.instance_number);

  int number_of_decoded_bytes = -1;
  OPENER_TRACE_INFO(" -> set Assembly attribute byte array\r\n");
  CipByteArray *cip_byte_array = (CipByteArray *)data;

  if(message_router_request->request_data_size < cip_byte_array->length) {
    OPENER_TRACE_INFO(
      "DecodeCipByteArray: not enough data received.\n");
    message_router_response->general_status = kCipErrorNotEnoughData;
    return number_of_decoded_bytes;
  }
  if(message_router_request->request_data_size > cip_byte_array->length) {
    OPENER_TRACE_INFO(
      "DecodeCipByteArray: too much data received.\n");
    message_router_response->general_status = kCipErrorTooMuchData;
    return number_of_decoded_bytes;
  }

  // data-length is correct
  memcpy(cip_byte_array->data,
         message_router_request->data,
         cip_byte_array->length);

  if(AfterAssemblyDataReceived(instance) != kEipStatusOk) {
    /* punt early without updating the status... though I don't know
     * how much this helps us here, as the attribute's data has already
     * been overwritten.
     *
     * however this is the task of the application side which will
     * take the data. In addition we have to inform the sender that the
     * data was not ok.
     */
    message_router_response->general_status = kCipErrorInvalidAttributeValue;
  } else {
    message_router_response->general_status = kCipErrorSuccess;
  }

  number_of_decoded_bytes = cip_byte_array->length;

  return number_of_decoded_bytes;
}

static EipStatus AssemblyPreGetCallback(CipInstance *const instance,
                                        CipAttributeStruct *const attribute,
                                        CipByte service) {
  int rc;
  (void) attribute;
  (void) service; /* no unused parameter warnings */

  rc = BeforeAssemblyDataSend(instance);

  return rc;
}

static EipStatus AssemblyPostSetCallback(CipInstance *const instance,
                                         CipAttributeStruct *const attribute,
                                         CipByte service) {
  int rc;
  (void) attribute;
  (void) service; /* no unused parameter warnings */

  rc = AfterAssemblyDataReceived(instance);

  return rc;
}
