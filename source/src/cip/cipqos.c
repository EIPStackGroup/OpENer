/*******************************************************************************
 * Copyright (c) 2009/, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "cipqos.h"

#include "opener_user_conf.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "cipethernetlink.h"
#include "opener_api.h"
#include "trace.h"

CipUsint QFramesEnable = 0; /* Enables or disable sending 802.1Q frames on CIP and IEEE 1588 messages */
CipUsint DscpEvent = 59; /* DSCP value for event messages*/
CipUsint DscpGeneral = 47; /* DSCP value for general messages*/
CipUsint kDscpUrgent = 55; /* DSCP value for CIP transport class 0/1 Urgent priority messages */
CipUsint kDscpScheduled = 47; /* DSCP value for CIP transport class 0/1 Scheduled priority messages*/
CipUsint kDscpHigh = 43; /* DSCP value for CIP transport class 0/1 High priority messages */
CipUsint kDscpLow = 31; /* DSCP value for CIP transport class 0/1 low priority messages */
CipUsint kDscpExplicit = 27; /* DSCP value for CIP explicit messages (transport class 2/3 and UCMM)
                              and all other EtherNet/IP encapsulation messages */

/************** Functions ****************************************/
EipStatus GetAttributeSingleQoS(
  CipInstance *const RESTRICT instance,
  CipMessageRouterRequest *RESTRICT const message_router_request,
  CipMessageRouterResponse *RESTRICT const message_router_response,
  struct sockaddr *originator_address) {

    return GetAttributeSingle(instance, message_router_request,
      message_router_response, originator_address);
}

EipStatus SetAttributeSingleQoS(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  struct sockaddr *originator_address) {

    CipAttributeStruct *attribute = GetCipAttribute(
      instance, message_router_request->request_path.attribute_number);
    (void) instance; /*Suppress compiler warning */
    EipUint16 attribute_number = message_router_request->request_path.attribute_number;
    uint8_t set_bit_mask = (instance->cip_class->set_bit_mask[CalculateIndex(
      attribute_number)]);

    if(NULL != attribute && (set_bit_mask & (1 << ((attribute_number - 1) % 8)))) {
      CipUint attribute_value_recieved = GetDintFromMessage(
        &(message_router_request->data));

      if(!((attribute_value_recieved <= 0) || (attribute_value_recieved >= 63)) {
        OPENER_TRACE_INFO(" setAttribute %d\n", attribute_number);

        if(NULL != attribute->data) {
          CipUsint *data = (CipUsint *) attribute->data;
          *(data) = attribute_value_recieved;

          message_router_response->general_status = kCipErrorSuccess;
        } else {
          message_router_response->general_status = kCipErrorNotEnoughData;
        }
      } else {
        message_router_response->general_status = kCipErrorInvalidAttributeValue;
      }
    } else {
      /* we don't have this attribute */
      message_router_response->general_status = kCipErrorAttributeNotSupported;
    }

    message_router_response->size_of_additional_status = 0;
    message_router_response->data_length = 0;
    message_router_response->reply_service = (0x80
      | message_router_request->service);

    return kEipStatusOkSend;
}

CipUsint GetPriorityForSocket(ForwardOpenPriority priority) {
  switch (priority) {
    case kForwardOpenPriorityLow: {
      return kDscpLow;
      break;
    }
    case kForwardOpenPriorityHigh: {
      return kDscpHigh;
      break;
    }
    case kForwardOpenPriorityScheduled: {
      return kDscpScheduled;
      break;
    }
    case kForwardOpenPriorityUrgent: {
      return kDscpUrgent;
      break;
    }
    default: {
      return kDscpExplicit;
      break;
    }
  }
}

void InitializeCipQos(CipClass *class) {

  CipClass *meta_class = class->class_instance.cip_class;
}

EipStatus CipQoSInit() {

  CipClass *qos_class = NULL;

  if((qos_class = CreateCipClass(kCipQoSClassCode,
                                  0, /* # class attributes*/
                                  7, /* # highest class attribute number*/
                                  0, /* # class services*/
                                  8, /* # instance attributes*/
                                  8, /* # highest instance attribute number*/
                                  2, /* # instance services*/
                                  1, /* # instances*/
                                  "Quality of Service",
                                  1, /* # class revision*/
                                  &InitializeCipQos /* # function pointer for initialization*/
                                  )) == 0) {

    return kEipStatusError;
  }

  CipInstance *instance = GetCipInstance(qos_class, 1); /* bind attributes to the instance #1 that was created above*/

  InsertAttribute(instance, 1, kCipUsint, (void *) &QFramesEnable,
                  kNotSetOrGetable);
  InsertAttribute(instance, 2, kCipUsint, (void *) &DscpEvent,
                  kNotSetOrGetable);
  InsertAttribute(instance, 3, kCipUsint, (void *) &DscpGeneral,
                  kNotSetOrGetable);
  InsertAttribute(instance, 4, kCipUsint, (void *) &kDscpUrgent,
                  kGetableSingle | kSetable);
  InsertAttribute(instance, 5, kCipUsint, (void *) &kDscpScheduled,
                  kGetableSingle | kSetable);
  InsertAttribute(instance, 6, kCipUsint, (void *) &kDscpHigh,
                  kGetableSingle | kSetable);
  InsertAttribute(instance, 7, kCipUsint, (void *) &kDscpLow,
                  kGetableSingle | kSetable);
  InsertAttribute(instance, 8, kCipUsint, (void *) &kDscpExplicit,
                  kGetableSingle | kSetable);

  InsertService(qos_class, kGetAttributeSingle, &GetAttributeSingleQoS,
                  "GetAttributeSingleQoS");
  InsertService(qos_class, kSetAttributeSingle, &SetAttributeSingleQoS,
                  "SetAttributeSingleQoS");

  return kEipStatusOk;
}
