/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <string.h>

#include "cipethernetlink.h"

#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

typedef struct {
  CipWord control_bits;
  CipUint forced_interface_speed;
} CipEthernetLinkInterfaceControl;

typedef struct speed_duplex_array_entry {
  CipUint interface_speed;
  CipUint interface_duplex_mode;
} CipEthernetLinkSpeedDuplexArrayEntry;

typedef struct speed_duplex_options {
  CipUsint speed_duplex_array_count;
  struct speed_duplex_array_entry *speed_duplex_array;
} CipEthernetLinkSpeedDuplexOptions;

typedef struct {
  CipDword capability_bits;
  struct speed_duplex_options speed_duplex_options;
} CipEthernetLinkInterfaceCapability;

EipStatus GetAttributeSingleEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session);

/** @bried Configures the MAC address of the Ethernet Link object*
 *
 *  @param mac_address The MAC address of the Ethernet Link
 */

CipUsint dummy_attribute_usint = 0;
CipUdint dummy_attribute_udint = 0;

CipShortString interface_label = { .length = 0, .string = NULL };
CipEthernetLinkInterfaceControl interface_control = { .control_bits = 0,
                                                      .forced_interface_speed =
                                                        0 };

CipEthernetLinkSpeedDuplexArrayEntry speed_duplex_object = { .interface_speed =
                                                               100, .
                                                             interface_duplex_mode
                                                               = 1 };
CipEthernetLinkInterfaceCapability interface_capability = {
  .capability_bits = 1, .speed_duplex_options = { .speed_duplex_array_count =
                                                    1,
                                                  .speed_duplex_array =
                                                    &speed_duplex_object }
};

EipStatus CipEthernetLinkInit() {
  CipClass *ethernet_link_class = CreateCipClass(CIP_ETHERNETLINK_CLASS_CODE, 0, /* # class attributes*/
                                                 7, /* # highest class attribute number*/
                                                 2, /* # class services*/
                                                 11, /* # instance attributes*/
                                                 11, /* # highest instance attribute number*/
                                                 2, /* # instance services*/
                                                 1, /* # instances*/
                                                 "Ethernet Link", 4, /* # class revision*/
                                                 NULL); /* # function pointer for initialization*/

  /* set attributes to initial values */
  g_ethernet_link.interface_speed = 100;
  g_ethernet_link.interface_flags = 0xF; /* successful speed and duplex neg, full duplex active link, TODO in future it should be checked if link is active */

  if (ethernet_link_class != NULL) {
    CipInstance *ethernet_link_instance = GetCipInstance(ethernet_link_class,
                                                         1);
    InsertAttribute(ethernet_link_instance, 1, kCipUdint,
                    &g_ethernet_link.interface_speed, kGetableSingleAndAll); /* bind attributes to the instance*/
    InsertAttribute(ethernet_link_instance, 2, kCipDword,
                    &g_ethernet_link.interface_flags, kGetableSingleAndAll);
    InsertAttribute(ethernet_link_instance, 3, kCip6Usint,
                    &g_ethernet_link.physical_address, kGetableSingleAndAll);
    InsertAttribute(ethernet_link_instance, 4, kCipUsint,
                    &dummy_attribute_udint, kGetableAll);
    InsertAttribute(ethernet_link_instance, 5, kCipUsint,
                    &dummy_attribute_udint, kGetableAll);
    InsertAttribute(ethernet_link_instance, 6, kCipUdint, &interface_control,
                    kGetableAll);
    InsertAttribute(ethernet_link_instance, 7, kCipUsint,
                    &dummy_attribute_usint, kGetableAll);
    InsertAttribute(ethernet_link_instance, 8, kCipUsint,
                    &dummy_attribute_usint, kGetableAll);
    InsertAttribute(ethernet_link_instance, 9, kCipUsint,
                    &dummy_attribute_usint, kGetableAll);
    InsertAttribute(ethernet_link_instance, 10, kCipShortString,
                    &interface_label, kGetableAll);
    InsertAttribute(ethernet_link_instance, 11, kCipAny, &interface_capability,
                    kGetableSingleAndAll);

    InsertService(ethernet_link_class, kGetAttributeSingle,
                  &GetAttributeSingleEthernetLink, "GetAttributeSingle");
    InsertService(ethernet_link_class, kGetAttributeAll, &GetAttributeAll,
                  "GetAttributeAll");
  } else {
    return kEipStatusError;
  }

  return kEipStatusOk;
}

int EncodeInterfaceCounters(EipUint8 **pa_acMsg) {
// Returns default value 0
  int return_value = 0;
  for (int i = 0; i < 11; i++) {
    return_value += EncodeData(kCipUdint, &dummy_attribute_udint, pa_acMsg);
  }
  return return_value;
}

int EncodeMediaCounters(EipUint8 **pa_acMsg) {
// Returns default value 0
  int return_value = 0;
  for (int i = 0; i < 12; i++) {
    return_value += EncodeData(kCipUdint, &dummy_attribute_udint, pa_acMsg);
  }
  return return_value;
}

int EncodeInterfaceControl(EipUint8 **pa_acMsg) {
// Returns default value 0
  int return_value = 0;
  return_value += EncodeData(kCipWord, &interface_control.control_bits,
                             pa_acMsg);
  return_value += EncodeData(kCipUint,
                             &interface_control.forced_interface_speed,
                             pa_acMsg);
  return return_value;
}

int EncodeInterfaceCapability(EipUint8 **pa_acMsg) {
  int return_value = 0;
  return_value += EncodeData(kCipDword, &interface_capability.capability_bits,
                             pa_acMsg);
  return_value += EncodeData(
    kCipUsint,
    &interface_capability.speed_duplex_options.speed_duplex_array_count,
    pa_acMsg);

  for (int i = 0;
       i < interface_capability.speed_duplex_options.speed_duplex_array_count;
       i++) {
    return_value += EncodeData(
      kCipUint,
      &(interface_capability.speed_duplex_options.speed_duplex_array[i]
        .interface_speed),
      pa_acMsg);
    return_value += EncodeData(
      kCipUsint,
      &(interface_capability.speed_duplex_options.speed_duplex_array[i]
        .interface_duplex_mode),
      pa_acMsg);
  }
  return return_value;
}

EipStatus GetAttributeSingleEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session) {
  /* Mask for filtering get-ability */

  CipAttributeStruct *attribute = GetCipAttribute(
    instance, message_router_request->request_path.attribute_number);
  EipByte *message = message_router_response->data;

  message_router_response->data_length = 0;
  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->general_status = kCipErrorAttributeNotSupported;
  message_router_response->size_of_additional_status = 0;

  EipUint16 attribute_number = message_router_request->request_path
                               .attribute_number;

  if ( (NULL != attribute) && (NULL != attribute->data) ) {
    uint8_t get_bit_mask = 0;
    if (kGetAttributeAll == message_router_request->service) {
      get_bit_mask = (instance->cip_class->get_all_bit_mask[CalculateIndex(
                                                              attribute_number)]);
      message_router_response->general_status = kCipErrorSuccess;
    } else {
      get_bit_mask = (instance->cip_class->get_single_bit_mask[CalculateIndex(
                                                                 attribute_number)
                      ]);
    }
    if ( 0 != ( get_bit_mask & ( 1 << (attribute_number % 8) ) ) ) {
      OPENER_TRACE_INFO("getAttribute %d\n",
                        message_router_request->request_path.attribute_number); /* create a reply message containing the data*/

      switch (attribute_number) {
        case 4:
          message_router_response->data_length = EncodeInterfaceCounters(
            &message);
          message_router_response->general_status = kCipErrorSuccess;
          break;
        case 5:
          message_router_response->data_length = EncodeMediaCounters(&message);
          message_router_response->general_status = kCipErrorSuccess;
          break;
        case 6:
          message_router_response->data_length = EncodeInterfaceControl(
            &message);
          message_router_response->general_status = kCipErrorSuccess;
          break;
        case 11:
          message_router_response->data_length = EncodeInterfaceCapability(
            &message);
          message_router_response->general_status = kCipErrorSuccess;
          break;

        default:
          GetAttributeSingle(instance, message_router_request,
                             message_router_response,
                             originator_address,
                             encapsulation_session);
      }

    }
  }

  return kEipStatusOkSend;
}

