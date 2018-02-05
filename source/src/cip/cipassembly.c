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

/** @brief Implementation of the SetAttributeSingle CIP service for Assembly
 *          Objects.
 *  Currently only supports Attribute 3 (CIP_BYTE_ARRAY) of an Assembly
 */
EipStatus SetAssemblyAttributeSingle(
  CipInstance *const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session);

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
  if (NULL != assembly_class) {
    InsertService(assembly_class, kGetAttributeSingle, &GetAttributeSingle,
                  "GetAttributeSingle");
    InsertService(assembly_class, kSetAttributeSingle,
                  &SetAssemblyAttributeSingle, "SetAssemblyAttributeSingle");
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
  CipClass *assembly_class = GetCipClass(kCipAssemblyClassCode);

  if (NULL != assembly_class) {
    CipInstance *instance = assembly_class->instances;
    while (NULL != instance) {
      CipAttributeStruct *attribute = GetCipAttribute(instance, 3);
      if (NULL != attribute) {
        CipFree(attribute->data);
      }
      instance = instance->next;
    }
  }
}

CipInstance *CreateAssemblyObject(const EipUint32 instance_id,
                                  EipByte *const data,
                                  const EipUint16 data_length) {
  CipClass *assembly_class = NULL;
  if ( NULL == ( assembly_class = GetCipClass(kCipAssemblyClassCode) ) ) {
    if ( NULL == ( assembly_class = CreateAssemblyClass() ) ) {
      return NULL;
    }
  }

  CipInstance *const instance = AddCIPInstance(assembly_class, instance_id);  /* add instances (always succeeds (or asserts))*/

  CipByteArray *const assembly_byte_array = (CipByteArray *) CipCalloc(
    1, sizeof(CipByteArray) );
  if (assembly_byte_array == NULL) {
    return NULL; /*TODO remove assembly instance in case of error*/
  }

  assembly_byte_array->length = data_length;
  assembly_byte_array->data = data;
  InsertAttribute(instance, 3, kCipByteArray, assembly_byte_array,
                  kSetAndGetAble);
  /* Attribute 4 Number of bytes in Attribute 3 */
  InsertAttribute(instance, 4, kCipUint, &(assembly_byte_array->length),
                  kGetableSingle);

  return instance;
}

EipStatus NotifyAssemblyConnectedDataReceived(CipInstance *const instance,
                                              const EipUint8 *const data,
                                              const EipUint16 data_length) {
  /* empty path (path size = 0) need to be checked and taken care of in future */
  /* copy received data to Attribute 3 */
  CipByteArray *assembly_byte_array = (CipByteArray *) instance->attributes
                                      ->data;
  if (assembly_byte_array->length != data_length) {
    OPENER_TRACE_ERR("wrong amount of data arrived for assembly object\n");
    return kEipStatusError; /*TODO question should we notify the application that wrong data has been received???*/
  } else {
    memcpy(assembly_byte_array->data, data, data_length);
    /* call the application that new data arrived */
  }

  return AfterAssemblyDataReceived(instance);
}

EipStatus SetAssemblyAttributeSingle(
  CipInstance *const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session) {
  OPENER_TRACE_INFO(" setAttribute %d\n",
                    message_router_request->request_path.attribute_number);

  const EipUint8 *const router_request_data = message_router_request->data;

  message_router_response->data_length = 0;
  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->general_status = kCipErrorAttributeNotSupported;
  message_router_response->size_of_additional_status = 0;

  CipAttributeStruct *attribute = GetCipAttribute(
    instance, message_router_request->request_path.attribute_number);

  if ( (attribute != NULL)
       && (3 == message_router_request->request_path.attribute_number) ) {
    if (attribute->data != NULL) {
      CipByteArray *data = (CipByteArray *) attribute->data;

      /* TODO: check for ATTRIBUTE_SET/GETABLE MASK */
      if ( true == IsConnectedOutputAssembly(instance->instance_number) ) {
        OPENER_TRACE_WARN(
          "Assembly AssemblyAttributeSingle: received data for connected output assembly\n\r");
        message_router_response->general_status = kCipErrorAttributeNotSetable;
      } else {
        if (message_router_request->request_path_size < data->length) {
          OPENER_TRACE_INFO(
            "Assembly setAssemblyAttributeSingle: not enough data received.\r\n");
          message_router_response->general_status = kCipErrorNotEnoughData;
        } else {
          if (message_router_request->request_path_size > data->length) {
            OPENER_TRACE_INFO(
              "Assembly setAssemblyAttributeSingle: too much data received.\r\n");
            message_router_response->general_status = kCipErrorTooMuchData;
          } else {
            memcpy(data->data, router_request_data, data->length);

            if (AfterAssemblyDataReceived(instance) != kEipStatusOk) {
              /* punt early without updating the status... though I don't know
               * how much this helps us here, as the attribute's data has already
               * been overwritten.
               *
               * however this is the task of the application side which will
               * take the data. In addition we have to inform the sender that the
               * data was not ok.
               */
              message_router_response->general_status =
                kCipErrorInvalidAttributeValue;
            } else {
              message_router_response->general_status = kCipErrorSuccess;
            }
          }
        }
      }
    } else {
      /* the attribute was zero we are a heartbeat assembly */
      message_router_response->general_status = kCipErrorTooMuchData;
    }
  }

  if ( (attribute != NULL)
       && (4 == message_router_request->request_path.attribute_number) ) {
    message_router_response->general_status = kCipErrorAttributeNotSetable;
  }

  return kEipStatusOkSend;
}
