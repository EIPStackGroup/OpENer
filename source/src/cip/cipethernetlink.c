/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/*
 * CIP Ethernet Link Object
 * ========================
 *
 * Implemented Attributes
 * ----------------------
 * - Attribute  1: Interface Speed
 * - Attribute  2: Interface Flags
 * - Attribute  3: Physical Address (Ethernet MAC)
 * - Attribute 10: Interface Label
 * - Attribute 11: Interface Capabilities
 *
 */

#include "cipethernetlink.h"

#include <string.h>

#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

/** @brief Type definition of the Interface Control attribute (#6)
 *
 *  This is used only internally at the moment.
 */
typedef struct {
  CipWord control_bits;
  CipUint forced_interface_speed;
} CipEthernetLinkInterfaceControl;

/** @brief Type definition of one entry in the speed / duplex array
 */
typedef struct speed_duplex_array_entry {
  CipUint interface_speed;  /**< the interface speed in Mbit/s */
  CipUsint interface_duplex_mode;  /**< the interface's duplex mode: 0 = half duplex, 1 = full duplex, 2-255 = reserved */
} CipEthernetLinkSpeedDuplexArrayEntry;


/* forward declaration of functions to encode certain attribute objects */
static int EncodeInterfaceCounters(EipUint8 **pa_acMsg);

static int EncodeMediaCounters(EipUint8 **pa_acMsg);

static int EncodeInterfaceControl(EipUint8 **pa_acMsg);

static int EncodeInterfaceCapability(EipUint8 **pa_acMsg);


/* forward declaration for the GetAttributeSingle service handler function */
EipStatus GetAttributeSingleEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session);


/** @brief This is the internal table of possible speed / duplex combinations
 *
 *  This table contains all possible speed / duplex combinations of today.
 *  Which entries of this table are transmitted during the GetService
 *  is controlled by the
 *  CipEthernetLinkMetaInterfaceCapability::speed_duplex_selector bit mask.
 *  Therefore you need to keep this array in sync with the bit masks of
 *  CipEthLinkSpeedDpxSelect.
 */
static const CipEthernetLinkSpeedDuplexArrayEntry speed_duplex_table[] =
{
  { /* Index 0: 10Mbit/s half duplex*/
    .interface_speed = 10,
    .interface_duplex_mode = 0
  },
  { /* Index 1: 10Mbit/s full duplex*/
    .interface_speed = 10,
    .interface_duplex_mode = 1
  },
  { /* Index 2: 100Mbit/s half duplex*/
    .interface_speed = 100,
    .interface_duplex_mode = 0
  },
  { /* Index 3: 100Mbit/s full duplex*/
    .interface_speed = 100,
    .interface_duplex_mode = 1
  },
  { /* Index 4: 1000Mbit/s half duplex*/
    .interface_speed = 1000,
    .interface_duplex_mode = 0
  },
  { /* Index 5: 1000Mbit/s full duplex*/
    .interface_speed = 1000,
    .interface_duplex_mode = 1
  },
};


/* Two dummy variables to provide fill data for the GetAttributeAll service. */
static CipUsint dummy_attribute_usint = 0;
static CipUdint dummy_attribute_udint = 0;

/* Constant dummy data for attribute #6 */
static CipEthernetLinkInterfaceControl interface_control =
{
  .control_bits = 0,
  .forced_interface_speed = 0,
};

/** @brief Definition of the Ethernet Link object instance(s) */
CipEthernetLinkObject g_ethernet_link;

EipStatus CipEthernetLinkInit() {
  CipClass *ethernet_link_class = CreateCipClass(kCipEthernetLinkClassCode,
                                                 0, /* # class attributes*/
                                                 7, /* # highest class attribute number*/
                                                 2, /* # class services*/
                                                 11, /* # instance attributes*/
                                                 11, /* # highest instance attribute number*/
                                                 2, /* # instance services*/
                                                 1, /* # instances*/
                                                 "Ethernet Link", /* # class name */
                                                 4, /* # class revision*/
                                                 NULL); /* # function pointer for initialization*/

  /* set attributes to initial values */
  g_ethernet_link.interface_speed = 100;
  g_ethernet_link.interface_flags = 0xF; /* successful speed and duplex neg, full duplex active link, TODO in future it should be checked if link is active */
  g_ethernet_link.interface_caps.capability_bits = kEthLinkCapAutoNeg;
  g_ethernet_link.interface_caps.speed_duplex_selector =
    kEthLinkSpeedDpx_100_FD;

  if (ethernet_link_class != NULL) {
    CipInstance *ethernet_link_instance = GetCipInstance(ethernet_link_class,
                                                         1);
    /* add services to the class */
    InsertService(ethernet_link_class, kGetAttributeSingle,
                  &GetAttributeSingleEthernetLink, "GetAttributeSingle");
    InsertService(ethernet_link_class, kGetAttributeAll, &GetAttributeAll,
                  "GetAttributeAll");

    /* bind attributes to the instance*/
    InsertAttribute(ethernet_link_instance, 1, kCipUdint,
                    &g_ethernet_link.interface_speed, kGetableSingleAndAll);
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
                    &g_ethernet_link.interface_label, kGetableAll);
    InsertAttribute(ethernet_link_instance, 11, kCipAny,
                    &g_ethernet_link.interface_caps, kGetableSingleAndAll);
  } else {
    return kEipStatusError;
  }

  return kEipStatusOk;
}

static int EncodeInterfaceCounters(EipUint8 **pa_acMsg) {
// Returns default value 0
  int return_value = 0;
  for (int i = 0; i < 11; i++) {
    return_value += EncodeData(kCipUdint, &dummy_attribute_udint, pa_acMsg);
  }
  return return_value;
}

static int EncodeMediaCounters(EipUint8 **pa_acMsg) {
// Returns default value 0
  int return_value = 0;
  for (int i = 0; i < 12; i++) {
    return_value += EncodeData(kCipUdint, &dummy_attribute_udint, pa_acMsg);
  }
  return return_value;
}

static int EncodeInterfaceControl(EipUint8 **pa_acMsg) {
// Returns default value 0
  int return_value = 0;
  return_value += EncodeData(kCipWord, &interface_control.control_bits,
                             pa_acMsg);
  return_value += EncodeData(kCipUint,
                             &interface_control.forced_interface_speed,
                             pa_acMsg);
  return return_value;
}

#define NELEMENTS(x)  ((sizeof(x)/sizeof(x[0])))
static int EncodeInterfaceCapability(EipUint8 **pa_acMsg)
{
  int return_value = 0;
  return_value += EncodeData(kCipDword,
                             &g_ethernet_link.interface_caps.capability_bits,
                             pa_acMsg);
  {
    unsigned selected = g_ethernet_link.interface_caps.speed_duplex_selector;
    CipUsint count;
    for (count = 0; selected; count++) { /* count # of bits set */
      selected &= selected - 1u;  /* clear the least significant bit set */
    }
    return_value += EncodeData(kCipUsint, &count, pa_acMsg);
  }

  for (unsigned u = 0; u < NELEMENTS(speed_duplex_table); u++) {
    if (g_ethernet_link.interface_caps.speed_duplex_selector & (1u << u)) {
      return_value += EncodeData(
                        kCipUint,
                        &speed_duplex_table[u].interface_speed,
                        pa_acMsg);
      return_value += EncodeData(
                        kCipUsint,
                        &speed_duplex_table[u].interface_duplex_mode,
                        pa_acMsg);
    }
  }
  return return_value;
}

EipStatus GetAttributeSingleEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
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

