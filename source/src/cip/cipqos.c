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

CipUsint q_frames_enable = 0; /**< Enables or disable sending 802.1Q frames on CIP and IEEE 1588 messages */
CipUsint dscp_event = 59; /**< DSCP value for event messages*/
CipUsint dscp_general = 47; /**< DSCP value for general messages*/
CipUsint dscp_urgent = 55; /**< DSCP value for CIP transport class 0/1 Urgent priority messages */
CipUsint dscp_scheduled = 47; /**< DSCP value for CIP transport class 0/1 Scheduled priority messages*/
CipUsint dscp_high = 43; /**< DSCP value for CIP transport class 0/1 High priority messages */
CipUsint dscp_low = 31; /**< DSCP value for CIP transport class 0/1 low priority messages */
CipUsint dscp_explicit = 27; /**< DSCP value for CIP explicit messages (transport class 2/3 and UCMM)
                                and all other EtherNet/IP encapsulation messages */

/************** Functions ****************************************/
EipStatus GetAttributeSingleQoS(
  CipInstance *const RESTRICT instance,
  CipMessageRouterRequest *RESTRICT const message_router_request,
  CipMessageRouterResponse *RESTRICT const message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session) {

  return GetAttributeSingle(instance, message_router_request,
                            message_router_response, originator_address,
                            encapsulation_session);
}

EipStatus SetAttributeSingleQoS(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session) {

  CipAttributeStruct *attribute = GetCipAttribute(
    instance, message_router_request->request_path.attribute_number);
  (void) instance;   /*Suppress compiler warning */
  EipUint16 attribute_number =
    message_router_request->request_path.attribute_number;
  uint8_t set_bit_mask = (instance->cip_class->set_bit_mask[CalculateIndex(
                                                              attribute_number)
                          ]);

  if( NULL != attribute &&
      ( set_bit_mask & ( 1 << ( (attribute_number) % 8 ) ) ) ) {
    CipUint attribute_value_recieved = GetDintFromMessage(
      &(message_router_request->data) );

    if( !( (attribute_value_recieved <= 0) ||
           (attribute_value_recieved >= 63) ) ) {
      OPENER_TRACE_INFO(" setAttribute %d\n", attribute_number);

      if(NULL != attribute->data) {
        CipUsint *data = (CipUsint *) attribute->data;
        *(data) = attribute_value_recieved;

        message_router_response->general_status = kCipErrorSuccess;
      } else {
        message_router_response->general_status = kCipErrorNotEnoughData;
        OPENER_TRACE_INFO("CIP QoS not enough data\n");
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

CipUsint GetPriorityForSocket(ConnectionObjectPriority priority) {
  switch (priority) {
    case kConnectionObjectPriorityLow: {
      return dscp_low;
      break;
    }
    case kConnectionObjectPriorityHigh: {
      return dscp_high;
      break;
    }
    case kConnectionObjectPriorityScheduled: {
      return dscp_scheduled;
      break;
    }
    case kConnectionObjectPriorityUrgent: {
      return dscp_urgent;
      break;
    }
    default: {
      return dscp_explicit;
      break;
    }
  }
}

void InitializeCipQos(CipClass *class) {

  CipClass *meta_class = class->class_instance.cip_class;
}

EipStatus CipQoSInit() {

  CipClass *qos_class = NULL;

  if( ( qos_class = CreateCipClass(kCipQoSClassCode,
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
                                   ) ) == 0 ) {

    return kEipStatusError;
  }

  CipInstance *instance = GetCipInstance(qos_class, 1); /* bind attributes to the instance #1 that was created above*/

  InsertAttribute(instance, 1, kCipUsint, (void *) &q_frames_enable,
                  kNotSetOrGetable);
  InsertAttribute(instance, 2, kCipUsint, (void *) &dscp_event,
                  kNotSetOrGetable);
  InsertAttribute(instance, 3, kCipUsint, (void *) &dscp_general,
                  kNotSetOrGetable);
  InsertAttribute(instance, 4, kCipUsint, (void *) &dscp_urgent,
                  kGetableSingle | kSetable);
  InsertAttribute(instance, 5, kCipUsint, (void *) &dscp_scheduled,
                  kGetableSingle | kSetable);
  InsertAttribute(instance, 6, kCipUsint, (void *) &dscp_high,
                  kGetableSingle | kSetable);
  InsertAttribute(instance, 7, kCipUsint, (void *) &dscp_low,
                  kGetableSingle | kSetable);
  InsertAttribute(instance, 8, kCipUsint, (void *) &dscp_explicit,
                  kGetableSingle | kSetable);

  InsertService(qos_class, kGetAttributeSingle, &GetAttributeSingleQoS,
                "GetAttributeSingleQoS");
  InsertService(qos_class, kSetAttributeSingle, &SetAttributeSingleQoS,
                "SetAttributeSingleQoS");

  return kEipStatusOk;
}
