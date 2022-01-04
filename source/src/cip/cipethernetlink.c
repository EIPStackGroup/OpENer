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
 *  - GetAttributesAll
 *  - GetAttributeSingle
 *  - GetAndClearAttribute  (0 != OPENER_ETHLINK_CNTRS_ENABLE)
 *        This service should only implemented for the attributes 4, 5, 12,
 *        13 and 15.
 *  - SetAttributeSingle (0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE)
 *  		This service should only be implemented if attribute 6 is enabled.
 *
 */

#include "cipethernetlink.h"

#include <string.h>

#include "cipcommon.h"
#include "opener_api.h"
#include "trace.h"
#include "opener_user_conf.h"
#include "endianconv.h"

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

#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
  0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE
  #define IFACE_CTRL_ACCESS_MODE  (kSetAndGetAble | kNvDataFunc)
#else
  #define IFACE_CTRL_ACCESS_MODE  kGetableAll
#endif
/** @brief Type definition of one entry in the speed / duplex array
 */
typedef struct speed_duplex_array_entry {
  CipUint interface_speed;  /**< the interface speed in Mbit/s */
  CipUsint interface_duplex_mode;  /**< the interface's duplex mode: 0 = half duplex, 1 = full duplex, 2-255 = reserved */
} CipEthernetLinkSpeedDuplexArrayEntry;


/* forward declaration of functions to encode certain attribute objects */
static void EncodeCipEthernetLinkInterfaceCounters(const void *const data,
                                                   ENIPMessage *const outgoing_message);

static void EncodeCipEthernetLinkMediaCounters(const void *const data,
                                               ENIPMessage *const outgoing_message);

static void EncodeCipEthernetLinkInterfaceControl(const void *const data,
                                                  ENIPMessage *const outgoing_message);

static void EncodeCipEthernetLinkInterfaceCaps(const void *const data,
                                               ENIPMessage *const outgoing_message);

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
/* forward declaration for the GetAndClear service handler function */
EipStatus GetAndClearEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const CipSessionHandle encapsulation_session);
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */

#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
  0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE

/** @brief Modify the attribute values for Attribute6: Interface Control
 *
 * @param CipEthernetLinkInterfaceControl pointer to attribute data.
 * @param message_router_request pointer to request.
 * @param message_router_response pointer to response.
 * @return length of taken bytes
 *          -1 .. error
 */
int DecodeCipEthernetLinkInterfaceControl(
		CipEthernetLinkInterfaceControl *const data,
		const CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response);
#endif


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
    .length = sizeof IFACE_LABEL - 1,
    .string = (EipByte *)IFACE_LABEL
  },
#if OPENER_ETHLINK_INSTANCE_CNT > 1
  {
    .length = sizeof IFACE_LABEL_1 - 1,
    .string = (EipByte *)IFACE_LABEL_1
  },
#endif
#if OPENER_ETHLINK_INSTANCE_CNT > 2
  {
    .length = sizeof IFACE_LABEL_2 - 1,
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

#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
  0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE
#else
/* Constant dummy data for attribute #6 */
static CipEthernetLinkInterfaceControl s_interface_control =
{
  .control_bits = 0,
  .forced_interface_speed = 0,
};
#endif

/** @brief Definition of the Ethernet Link object instance(s) */
CipEthernetLinkObject g_ethernet_link[OPENER_ETHLINK_INSTANCE_CNT];

EipStatus CipEthernetLinkInit(void) {
  CipClass *ethernet_link_class = CreateCipClass(kCipEthernetLinkClassCode,
                                                 0,
                                                 /* # class attributes*/
                                                 7,
                                                 /* # highest class attribute number*/
                                                 2,
                                                 /* # class services*/
                                                 11,
                                                 /* # instance attributes*/
                                                 11,
                                                 /* # highest instance attribute number*/
                                                 /* # instance services follow */
                                                 2 + OPENER_ETHLINK_CNTRS_ENABLE + OPENER_ETHLINK_IFACE_CTRL_ENABLE,
                                                 OPENER_ETHLINK_INSTANCE_CNT,
                                                 /* # instances*/
                                                 "Ethernet Link",
                                                 /* # class name */
                                                 4,
                                                 /* # class revision*/
                                                 NULL); /* # function pointer for initialization*/

  /* set attributes to initial values */
  for (size_t idx = 0; idx < OPENER_ETHLINK_INSTANCE_CNT; ++idx) {
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
#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
    0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE
    g_ethernet_link[idx].interface_control.control_bits =
      kEthLinkIfCntrlAutonegotiate;
    g_ethernet_link[idx].interface_control.forced_interface_speed = 0U;
#endif
  }

  if (ethernet_link_class != NULL) {
    /* add services to the class */
    InsertService(ethernet_link_class, kGetAttributeSingle,
                  &GetAttributeSingle,
                  "GetAttributeSingle");
    InsertService(ethernet_link_class, kGetAttributeAll, &GetAttributeAll,
                  "GetAttributeAll");

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
    InsertService(ethernet_link_class, kEthLinkGetAndClear,
                  &GetAndClearEthernetLink, "GetAndClear");
#endif
#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
    0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE
    InsertService(ethernet_link_class, kSetAttributeSingle,
    				&SetAttributeSingle,
                      "SetAttributeSingle");
#endif

    /* bind attributes to the instance */
    for (CipInstanceNum idx = 0; idx < OPENER_ETHLINK_INSTANCE_CNT; ++idx) {
      CipInstance *ethernet_link_instance =
        GetCipInstance( ethernet_link_class, (CipInstanceNum)(idx + 1) );

      InsertAttribute(ethernet_link_instance,
                      1,
                      kCipUdint,
                      EncodeCipUdint,
                      NULL,
                      &g_ethernet_link[idx].interface_speed,
                      kGetableSingleAndAll);
      InsertAttribute(ethernet_link_instance,
                      2,
                      kCipDword,
                      EncodeCipDword,
                      NULL,
                      &g_ethernet_link[idx].interface_flags,
                      kGetableSingleAndAll);
      InsertAttribute(ethernet_link_instance,
                      3,
                      kCip6Usint,
                      EncodeCipEthernetLinkPhyisicalAddress,
                      NULL,
                      &g_ethernet_link[idx].physical_address,
                      kGetableSingleAndAll);
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
      InsertAttribute(ethernet_link_instance,
                      4,
                      kCipUsint,
                      EncodeCipUsint,
                      NULL,
                      &g_ethernet_link[idx].interface_cntrs,
                      kGetableSingleAndAll);
      InsertAttribute(ethernet_link_instance,
                      5,
                      kCipUsint,
                      EncodeCipUsint,
                      NULL,
                      &g_ethernet_link[idx].media_cntrs,
                      kGetableSingleAndAll);
#else
      InsertAttribute(ethernet_link_instance,
                      4,
                      kCipAny,
                      EncodeCipEthernetLinkInterfaceCounters,
                      NULL,
                      &dummy_attribute_udint,
                      kGetableAllDummy);
      InsertAttribute(ethernet_link_instance,
                      5,
                      kCipAny,
                      EncodeCipEthernetLinkMediaCounters,
                      NULL,
                      &dummy_attribute_udint,
                      kGetableAllDummy);
#endif  /* ... && 0 != OPENER_ETHLINK_CNTRS_ENABLE */
#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
      0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE
      if (2 == idx) {
        /* Interface control of internal switch port is never settable. */
        InsertAttribute(ethernet_link_instance,
                        6,
                        kCipAny,
                        EncodeCipEthernetLinkInterfaceControl,
                        DecodeCipEthernetLinkInterfaceControl,
                        &g_ethernet_link[idx].interface_control,
                        IFACE_CTRL_ACCESS_MODE & ~kSetable);
      } else {
        InsertAttribute(ethernet_link_instance,
                        6,
                        kCipAny,
                        EncodeCipEthernetLinkInterfaceControl,
                        DecodeCipEthernetLinkInterfaceControl,
                        &g_ethernet_link[idx].interface_control,
                        IFACE_CTRL_ACCESS_MODE);
      }
#else
      InsertAttribute(ethernet_link_instance,
                      6,
                      kCipAny,
                      EncodeCipEthernetLinkInterfaceControl,
                      NULL,
                      &s_interface_control,
                      kGetableAll);
#endif
      InsertAttribute(ethernet_link_instance,
                      7,
                      kCipUsint,
                      EncodeCipUsint,
                      NULL,
                      &g_ethernet_link[idx].interface_type,
                      kGetableSingleAndAll);
      InsertAttribute(ethernet_link_instance,
                      8,
                      kCipUsint,
                      EncodeCipUsint,
                      NULL,
                      &dummy_attribute_usint,
                      kGetableAllDummy);
      InsertAttribute(ethernet_link_instance,
                      9,
                      kCipUsint,
                      EncodeCipUsint,
                      NULL,
                      &dummy_attribute_usint, kGetableAllDummy);
      InsertAttribute(ethernet_link_instance,
                      10,
                      kCipShortString,
                      EncodeCipShortString,
                      NULL,
                      &g_ethernet_link[idx].interface_label,
                      IFACE_LABEL_ACCESS_MODE);
      InsertAttribute(ethernet_link_instance,
                      11,
                      kCipAny,
                      EncodeCipEthernetLinkInterfaceCaps,
                      NULL,
                      &g_ethernet_link[idx].interface_caps,
                      kGetableSingleAndAll);
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

static void EncodeCipEthernetLinkInterfaceCounters(const void *const data,
                                                   ENIPMessage *const outgoing_message)
{
  /* Suppress unused parameter compiler warning. */
  (void)data;

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
  for (size_t i = 0; i < 11; i++) {
    /* Encode real values using the access through the array of the
     *  interface_cntrs union. */
    EncodeData(kCipUdint,
               g_ethernet_link[instance_id - 1].interface_cntrs.cntr32 + i,
               message_router_response);
  }
#else
  /* Encode the default counter value of 0 */
  FillNextNMessageOctetsWithValueAndMoveToNextPosition(0,
                                                       11 * sizeof(CipUdint),
                                                       outgoing_message);
#endif
}

static void EncodeCipEthernetLinkMediaCounters(const void *const data,
                                               ENIPMessage *const outgoing_message)
{
  /* Suppress unused parameter compiler warning. */
  (void)data;

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
  for (size_t i = 0; i < 12; i++) {
    /* Encode real values using the access through the array of the
     *  media_cntrs union. */
    EncodeData(kCipUdint,
               g_ethernet_link[instance_id - 1].media_cntrs.cntr32 + i,
               message_router_response);
  }
#else
  /* Encode the default counter value of 0 */
  FillNextNMessageOctetsWithValueAndMoveToNextPosition(0,
                                                       12 * sizeof(CipUdint),
                                                       outgoing_message);
#endif
}

static void EncodeCipEthernetLinkInterfaceControl(const void *const data,
                                                  ENIPMessage *const outgoing_message)
{
#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
  0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE
  const CipEthernetLinkInterfaceControl *const interface_control =
    data;
#else
  /* Suppress unused parameter compiler warning. */
  (void)data;

  CipEthernetLinkInterfaceControl *interface_control = &s_interface_control;
#endif
  EncodeCipWord(&interface_control->control_bits, outgoing_message);
  EncodeCipUint(&interface_control->forced_interface_speed, outgoing_message);
}

#define NELEMENTS(x)  ( (sizeof(x) / sizeof(x[0]) ) )
static void EncodeCipEthernetLinkInterfaceCaps(const void *const data,
                                               ENIPMessage *const outgoing_message)
{
  const CipEthernetLinkMetaInterfaceCapability *const interface_caps = data;
  EncodeCipDword(&interface_caps->capability_bits, outgoing_message);
  uint16_t selected = interface_caps->speed_duplex_selector;
  CipUsint count = 0;
  while(selected) { /* count # of bits set */
    selected &= selected - 1U;        /* clear the least significant bit set */
    count++;
  }
  EncodeCipUsint(&count, outgoing_message);

  for (size_t i = 0; i < NELEMENTS(speed_duplex_table); i++) {
    if (interface_caps->speed_duplex_selector &
        (1U << i) ) {
      EncodeCipUint(&speed_duplex_table[i].interface_speed, outgoing_message);
      EncodeCipUsint(&speed_duplex_table[i].interface_duplex_mode,
                     outgoing_message);
    }
  }
}

#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
EipStatus GetAndClearEthernetLink(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const CipSessionHandle encapsulation_session) {

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

#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
  0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE
static bool IsIfaceControlAllowed(CipInstanceNum instance_id,
                                  CipEthernetLinkInterfaceControl const *iface_cntrl)
{
  const CipUsint duplex_mode =
    (iface_cntrl->control_bits & kEthLinkIfCntrlForceDuplexFD) ? 1 : 0;
  for (size_t i = 0; i < NELEMENTS(speed_duplex_table); i++) {
    if (g_ethernet_link[instance_id - 1].interface_caps.speed_duplex_selector &
        (1U << i) ) {
      if (duplex_mode == speed_duplex_table[i].interface_duplex_mode &&
          iface_cntrl->forced_interface_speed ==
          speed_duplex_table[i].interface_speed) {
        return true;
      }
    }
  }
  return false;
}

int DecodeCipEthernetLinkInterfaceControl(
		CipEthernetLinkInterfaceControl *const data,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	CipInstance *const instance = GetCipInstance(
				GetCipClass(message_router_request->request_path.class_id),
				message_router_request->request_path.instance_number);

	int number_of_decoded_bytes = -1;

	CipEthernetLinkInterfaceControl if_cntrl;

	DecodeCipWord(&if_cntrl.control_bits, message_router_request,
			message_router_response);
	DecodeCipUint(&if_cntrl.forced_interface_speed, message_router_request,
			message_router_response);

	if (if_cntrl.control_bits > kEthLinkIfCntrlMaxValid) {
		message_router_response->general_status =
				kCipErrorInvalidAttributeValue;
		return number_of_decoded_bytes;

	} else {
		if ((0 != (if_cntrl.control_bits & kEthLinkIfCntrlAutonegotiate))
				&& ((0 != (if_cntrl.control_bits & kEthLinkIfCntrlForceDuplexFD))
						|| (0 != if_cntrl.forced_interface_speed))) {
			message_router_response->general_status =
					kCipErrorObjectStateConflict;
			return number_of_decoded_bytes;
		} else {
			if (0 == (if_cntrl.control_bits & kEthLinkIfCntrlAutonegotiate)) {
				/* Need to check if a supported mode is forced. */
				if (!IsIfaceControlAllowed(instance->instance_number,
						&if_cntrl)) {
					message_router_response->general_status =
							kCipErrorInvalidAttributeValue;
					return number_of_decoded_bytes;
				}
			}
			*data = if_cntrl; //write data to attribute
			message_router_response->general_status = kCipErrorSuccess;
			number_of_decoded_bytes = 4;
		}
	}
	return number_of_decoded_bytes;
}

 #endif
