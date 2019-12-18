/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 *  @brief Implement the CIP Ethernet Link Object
 *
 *  CIP Ethernet Link Object
 *  ========================
 *
 *  Implemented Attributes
 *  ----------------------
 *  Conditional attributes are indented and marked with the condition it
 *  depends on like "(0 != OPENER_ETHLINK_CNTRS_ENABLE)"
 *
 *  - Attribute  1: Interface Speed
 *  - Attribute  2: Interface Flags
 *  - Attribute  3: Physical Address (Ethernet MAC)
 *      - Attribute  4: Interface Counters (32-bit) (0 != OPENER_ETHLINK_CNTRS_ENABLE)
 *      - Attribute  5: Media Counters (32-bit)     (0 != OPENER_ETHLINK_CNTRS_ENABLE)
 *      - Attribute  6: Interface Control           (0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE)
 *  - Attribute  7: Interface Type
 *      See Vol. 2 Section 6-3.4 regarding an example for a device with internal
 *      switch port where the implementation of this attribute is recommended.
 *  - Attribute 10: Interface Label (0 != OPENER_ETHLINK_LABEL_ENABLE)
 *      If the define OPENER_ETHLINK_LABEL_ENABLE is set then this attribute
 *      has a string content ("PORT 1" by default on instance 1) otherwise it
 *      is empty.
 *  - Attribute 11: Interface Capabilities
 *
 *  Implemented Services
 *  --------------------
 *  Conditional services are indented and marked with the condition it
 *  depends on like "(0 != OPENER_ETHLINK_CNTRS_ENABLE)"
 *
 *  - GetAttributeAll
 *  - GetAttributeSingle
 *    - GetAndClearAttribute  (0 != OPENER_ETHLINK_CNTRS_ENABLE)
 *        This service should only implemented for the attributes 4, 5, 12,
 *        13 and 15.
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

#if OPENER_ETHLINK_INSTANCE_CNT > 1
  /* If we have more than 1 Ethernet Link instance then the interface label
   * attribute is mandatory. We need then OPENER_ETHLINK_LABEL_ENABLE. */
  #define OPENER_ETHLINK_LABEL_ENABLE  1
#endif

#ifndef OPENER_ETHLINK_LABEL_ENABLE
  #define OPENER_ETHLINK_LABEL_ENABLE  0
#endif

#if defined(OPENER_ETHLINK_LABEL_ENABLE) && 0 != OPENER_ETHLINK_LABEL_ENABLE
  #define IFACE_LABEL_ACCESS_MODE kGetableSingleAndAll
  #define IFACE_LABEL             "PORT 1"
  #define IFACE_LABEL_1           "PORT 2"
  #define IFACE_LABEL_2           "PORT internal"
#else
  #define IFACE_LABEL_ACCESS_MODE kGetableAll
#endif

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
static int EncodeInterfaceCounters(CipUdint instance_id, CipOctet **message);

static int EncodeMediaCounters(CipUdint instance_id, CipOctet **message);

static int EncodeInterfaceControl(CipOctet **message);

static int EncodeInterfaceCapability(CipUdint instance_id, CipOctet **message);


/* forward declaration for the GetAttributeSingle service handler function */
EipStatus GetAttributeSingleEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session);

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
/* forward declaration for the GetAndClear service handler function */
EipStatus GetAndClearEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session);
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */


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

#if defined(OPENER_ETHLINK_LABEL_ENABLE) && 0 != OPENER_ETHLINK_LABEL_ENABLE
static const CipShortString iface_label_table[OPENER_ETHLINK_INSTANCE_CNT] =
{
  {
    .length = sizeof IFACE_LABEL -1,
    .string = (EipByte *)IFACE_LABEL
  },
#if OPENER_ETHLINK_INSTANCE_CNT > 1
  {
    .length = sizeof IFACE_LABEL_1 -1,
    .string = (EipByte *)IFACE_LABEL_1
  },
#endif
#if OPENER_ETHLINK_INSTANCE_CNT > 2
  {
    .length = sizeof IFACE_LABEL_2 -1,
    .string = (EipByte *)IFACE_LABEL_2
  },
#endif
};
#endif /* defined(OPENER_ETHLINK_LABEL_ENABLE) && 0 != OPENER_ETHLINK_LABEL_ENABLE */

/* Two dummy variables to provide fill data for the GetAttributeAll service. */
static CipUsint dummy_attribute_usint = 0;
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
#else
static CipUdint dummy_attribute_udint = 0;
#endif

/* Constant dummy data for attribute #6 */
static CipEthernetLinkInterfaceControl interface_control =
{
  .control_bits = 0,
  .forced_interface_speed = 0,
};

/** @brief Definition of the Ethernet Link object instance(s) */
CipEthernetLinkObject g_ethernet_link[OPENER_ETHLINK_INSTANCE_CNT];

EipStatus CipEthernetLinkInit(void) {
  CipClass *ethernet_link_class = CreateCipClass(kCipEthernetLinkClassCode,
                                                 0, /* # class attributes*/
                                                 7, /* # highest class attribute number*/
                                                 2, /* # class services*/
                                                 11, /* # instance attributes*/
                                                 11, /* # highest instance attribute number*/
                                                 2+OPENER_ETHLINK_CNTRS_ENABLE, /* # instance services*/
                                                 OPENER_ETHLINK_INSTANCE_CNT, /* # instances*/
                                                 "Ethernet Link", /* # class name */
                                                 4, /* # class revision*/
                                                 NULL); /* # function pointer for initialization*/

  /* set attributes to initial values */
  for (size_t idx = 0;  idx < OPENER_ETHLINK_INSTANCE_CNT; ++idx) {
    g_ethernet_link[idx].interface_speed = 100;
    /* successful speed and duplex neg, full duplex active link.
     * TODO: in future it should be checked if link is active */
    g_ethernet_link[idx].interface_flags = 0xF;

    g_ethernet_link[idx].interface_type = kEthLinkIfTypeTwistedPair;
    if (2 == idx) {
      g_ethernet_link[idx].interface_type = kEthLinkIfTypeInternal;
    }
#if defined(OPENER_ETHLINK_LABEL_ENABLE) && 0 != OPENER_ETHLINK_LABEL_ENABLE
    g_ethernet_link[idx].interface_label = iface_label_table[idx];
#endif
    g_ethernet_link[idx].interface_caps.capability_bits = kEthLinkCapAutoNeg;
    g_ethernet_link[idx].interface_caps.speed_duplex_selector =
      kEthLinkSpeedDpx_100_FD;
  }

  if (ethernet_link_class != NULL) {
    /* add services to the class */
    InsertService(ethernet_link_class, kGetAttributeSingle,
                  &GetAttributeSingleEthernetLink, "GetAttributeSingle");
    InsertService(ethernet_link_class, kGetAttributeAll, &GetAttributeAll,
                  "GetAttributeAll");
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
    InsertService(ethernet_link_class, kEthLinkGetAndClear,
                  &GetAndClearEthernetLink, "GetAndClear");
#endif

    /* bind attributes to the instance */
    for (size_t idx = 0; idx < OPENER_ETHLINK_INSTANCE_CNT; ++idx) {
      CipInstance *ethernet_link_instance =
        GetCipInstance(ethernet_link_class, idx+1);

      InsertAttribute(ethernet_link_instance, 1, kCipUdint,
                      &g_ethernet_link[idx].interface_speed, kGetableSingleAndAll);
      InsertAttribute(ethernet_link_instance, 2, kCipDword,
                      &g_ethernet_link[idx].interface_flags, kGetableSingleAndAll);
      InsertAttribute(ethernet_link_instance, 3, kCip6Usint,
                      &g_ethernet_link[idx].physical_address, kGetableSingleAndAll);
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
      InsertAttribute(ethernet_link_instance, 4, kCipUsint,
                      &g_ethernet_link[idx].interface_cntrs, kGetableSingleAndAll);
      InsertAttribute(ethernet_link_instance, 5, kCipUsint,
                      &g_ethernet_link[idx].media_cntrs, kGetableSingleAndAll);
#else
      InsertAttribute(ethernet_link_instance, 4, kCipUsint,
                      &dummy_attribute_udint, kGetableAll);
      InsertAttribute(ethernet_link_instance, 5, kCipUsint,
                      &dummy_attribute_udint, kGetableAll);
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */
      InsertAttribute(ethernet_link_instance, 6, kCipUdint, &interface_control,
                      kGetableAll);
      InsertAttribute(ethernet_link_instance, 7, kCipUsint,
                      &g_ethernet_link[idx].interface_type, kGetableSingleAndAll);
      InsertAttribute(ethernet_link_instance, 8, kCipUsint,
                      &dummy_attribute_usint, kGetableAll);
      InsertAttribute(ethernet_link_instance, 9, kCipUsint,
                      &dummy_attribute_usint, kGetableAll);
      InsertAttribute(ethernet_link_instance, 10, kCipShortString,
                      &g_ethernet_link[idx].interface_label,
                      IFACE_LABEL_ACCESS_MODE);
      InsertAttribute(ethernet_link_instance, 11, kCipAny,
                      &g_ethernet_link[idx].interface_caps, kGetableSingleAndAll);
    }
  } else {
    return kEipStatusError;
  }

  return kEipStatusOk;
}

void CipEthernetLinkSetMac(EipUint8 *p_physical_address) {
  for (size_t idx = 0; idx < OPENER_ETHLINK_INSTANCE_CNT; ++idx) {
    memcpy(g_ethernet_link[idx].physical_address,
           p_physical_address,
           sizeof(g_ethernet_link[0].physical_address)
           );
  }
  return;
}

static int EncodeInterfaceCounters(CipUdint instance_id, CipOctet **message) {
  int encoded_len = 0;
  for (size_t i = 0; i < 11; i++) {
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
    /* Encode real values using the access through the array of the
     *  interface_cntrs union. */
    encoded_len += EncodeData(
                    kCipUdint,
                    g_ethernet_link[instance_id-1].interface_cntrs.cntr32 +i,
                    message);
#else
    /* Encode the default counter value of 0 */
    encoded_len += EncodeData(kCipUdint, &dummy_attribute_udint, message);
#endif
  }
  return encoded_len;
}

static int EncodeMediaCounters(CipUdint instance_id, CipOctet **message) {
  int encoded_len = 0;
  for (size_t i = 0; i < 12; i++) {
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
    /* Encode real values using the access through the array of the
     *  media_cntrs union. */
    encoded_len += EncodeData(
                    kCipUdint,
                    g_ethernet_link[instance_id-1].media_cntrs.cntr32 +i,
                    message);
#else
    /* Encode the default counter value of 0 */
    encoded_len += EncodeData(kCipUdint, &dummy_attribute_udint, message);
#endif
  }
  return encoded_len;
}

static int EncodeInterfaceControl(CipOctet **message) {
  int encoded_len = 0;
  encoded_len += EncodeData(kCipWord, &interface_control.control_bits,
                            message);
  encoded_len += EncodeData(kCipUint,
                            &interface_control.forced_interface_speed,
                            message);
  return encoded_len;
}

#define NELEMENTS(x)  ((sizeof(x)/sizeof(x[0])))
static int EncodeInterfaceCapability(CipUdint instance_id, CipOctet **message)
{
  int encoded_len = 0;
  encoded_len += EncodeData(
                    kCipDword,
                    &g_ethernet_link[instance_id - 1].interface_caps.capability_bits,
                    message);
  uint16_t selected = g_ethernet_link[instance_id - 1].interface_caps.speed_duplex_selector;
  CipUsint count;
  for (count = 0; selected; count++) { /* count # of bits set */
	  selected &= selected - 1u;  /* clear the least significant bit set */
  }
  encoded_len += EncodeData(kCipUsint, &count, message);

  for (size_t i = 0; i < NELEMENTS(speed_duplex_table); i++) {
    if (g_ethernet_link[instance_id - 1].interface_caps.speed_duplex_selector &
        (1u << i)) {
      encoded_len += EncodeData(
                        kCipUint,
                        &speed_duplex_table[i].interface_speed,
                        message);
      encoded_len += EncodeData(
                        kCipUsint,
                        &speed_duplex_table[i].interface_duplex_mode,
                        message);
    }
  }
  return encoded_len;
}

EipStatus GetAttributeSingleEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {

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
    /* Mask for filtering get-ability */
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
    OPENER_TRACE_INFO ("Service %" PRIu8 ", get_bit_mask=%02" PRIX8 "\n",
                       message_router_request->service, get_bit_mask);
    if ( 0 != ( get_bit_mask & ( 1 << (attribute_number % 8) ) ) ) {
      OPENER_TRACE_INFO("getAttribute %d\n", attribute_number);

      /* create a reply message containing the data*/
      bool use_common_handler;
      switch (attribute_number) {
      case 4: /* fall through */
      case 5: /* fall through */
      case 6: /* fall through */
      case 11: /* fall through */
        use_common_handler = false;
        break;
      default:
        use_common_handler = true;
        break;
      }

      /* Call the PreGetCallback if enabled for this attribute and the common handler is not used. */
      if (!use_common_handler) {
        if (attribute->attribute_flags & kPreGetFunc && NULL != instance->cip_class->PreGetCallback) {
          instance->cip_class->PreGetCallback(instance, attribute, message_router_request->service);
        }
      }

      switch (attribute_number) {
        case 4:
          message_router_response->data_length = EncodeInterfaceCounters(
            instance->instance_number,
            &message);
          message_router_response->general_status = kCipErrorSuccess;
          break;
        case 5:
          message_router_response->data_length = EncodeMediaCounters(
            instance->instance_number,
            &message);
          message_router_response->general_status = kCipErrorSuccess;
          break;
        case 6:
          message_router_response->data_length = EncodeInterfaceControl(
            &message);
          message_router_response->general_status = kCipErrorSuccess;
          break;
        case 11:
          message_router_response->data_length = EncodeInterfaceCapability(
            instance->instance_number,
            &message);
          message_router_response->general_status = kCipErrorSuccess;
          break;

        default:
          GetAttributeSingle(instance, message_router_request,
                             message_router_response,
                             originator_address,
                             encapsulation_session);
      }

      /* Call the PostGetCallback if enabled for this attribute and the common handler is not used. */
      if (!use_common_handler) {
        if (attribute->attribute_flags & kPostGetFunc && NULL != instance->cip_class->PostGetCallback) {
          instance->cip_class->PostGetCallback(instance, attribute, message_router_request->service);
        }
      }

    }
  }

  return kEipStatusOkSend;
}

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
EipStatus GetAndClearEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {

  CipAttributeStruct *attribute = GetCipAttribute(
    instance, message_router_request->request_path.attribute_number);

  message_router_response->data_length = 0;
  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->general_status = kCipErrorAttributeNotSupported;
  message_router_response->size_of_additional_status = 0;

  EipUint16 attribute_number = message_router_request->request_path
                               .attribute_number;

  if ( (NULL != attribute) && (NULL != attribute->data) ) {
    OPENER_TRACE_INFO("GetAndClear attribute %" PRIu16 "\n", attribute_number);

    /* The PreGetCallback is not executed here.
     * The GetAttributeSingle{EthernetLink}() will do it on our behalf. */
    switch (attribute_number) {
      case 4: /* fall through */
      case 5:
        GetAttributeSingleEthernetLink(
                           instance, message_router_request,
                           message_router_response,
                           originator_address,
                           encapsulation_session);
        break;
      default:
        message_router_response->general_status =
          kCipErrorServiceNotSupportedForSpecifiedPath;
        break;
    }

    /* Clearing the counters must be done in the PostGetCallback because it
     *  is often hardware specific and can't be handled in generic code. */
    /* The PostGetCallback is not executed here. The
     *  GetAttributeSingle{EthernetLink}() should have done it on our
     *  behalf already.
     */
  }

  return kEipStatusOkSend;
}
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */
