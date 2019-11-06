/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the DLR object
 * @author Stefan Maetje <stefan.maetje@esd.eu>
 *
 *  CIP DLR object
 *  ==============
 *
 *  This module implements the DLR object for a non supervisor and non gateway
 *  device.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  1: Network Topology
 *  - Attribute  2: Network Status
 *  - Attribute 10: Active Supervisor Address
 *  - Attribute 12: Capability Flags
 *
 *  Non-implemented attributes
 *  --------------------------
 *  The attributes 3, 4, 5, 6, 7, 8, 9 and 11 are only required for devices
 *  that are capable of functioning as a ring supervisor. These attributes
 *  shall not be implemented by non-supervisor devices.
 *
 *  The attributes 13, 14, 15 and 16 are only required for devices that are
 *  capable of functioning as a redundant gateway. These attributes shall
 *  not be implemented by non-redundant gateway devices.
 *
 *  None of the attributes 17, 18 and 19 is required and implemented.
 *  Because of this the Object Revision stays on level 3 (see @ref
 *  DLR_CLASS_REVISION).
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributesAll
 *  - GetAttributeSingle
 */
/* ********************************************************************
 * include files
 */
#include "cipdlr.h"

#include <string.h>

#include "cipcommon.h"
#include "opener_api.h"
#include "trace.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is still 3 because the attributes
 *  mandatory for revision 4 are NOT implemented. */
#define DLR_CLASS_REVISION  3

/* ********************************************************************
 * Type declarations
 */

/* ********************************************************************
 * module local variables
 */
/* Define variables with default values to be used for the
 *  GetAttributeAll response for not implemented attributes. */
static const CipUsint s_0xFF_default = 0xFFU;
static const CipUint s_0xFFFF_default = 0xFFFFU;

static const CipUsint s_0x00_default = 0x00U;
static const CipUint s_0x0000_default = 0x0000U;
static const CipUdint s_0x00000000_default = 0x00000000U;

static const CipNodeAddress s_zero_node = {
  .device_ip = 0,
  .device_mac = {
    0, 0, 0, 0, 0, 0,
  }
};

/* ********************************************************************
 * global public variables
 */
CipDlrObject g_dlr;  /**< definition of DLR object instance 1 data */


/* ********************************************************************
 * local functions
 */
static int EncodeNodeAddress(CipNodeAddress *node_address, CipOctet **message) {
  int encoded_len = 0;
  encoded_len += EncodeData(kCipUdint, &node_address->device_ip,
                            message);
  encoded_len += EncodeData(kCip6Usint,
                            &node_address->device_mac,
                            message);
  return encoded_len;
}

static EipStatus GetAttributeSingleDlr(
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
      /* create a reply message containing the data*/
      bool use_common_handler;
      switch (attribute_number) {
      case 4: /* fall through */
      case 6: /* fall through */
      case 7: /* fall through */
      case 10:
        use_common_handler = false;
        OPENER_TRACE_INFO("getAttribute %d\n", attribute_number);
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
        case 4: {
          /* This attribute is not implemented and only reached by GetAttributesAll. */
          const size_t kRingSupStructSize = 12u;
          memset(message_router_response->data, 0, kRingSupStructSize);
          message_router_response->data += kRingSupStructSize;
          message_router_response->general_status = kCipErrorSuccess;
          break;
        }
        case 6: /* fall through */
        case 7: /* fall through */
        case 10:
          message_router_response->data_length = EncodeNodeAddress(
            (CipNodeAddress *)attribute->data,
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


/* ********************************************************************
 * public functions
 */
EipStatus CipDlrInit(void) {
  CipClass *dlr_class = NULL;

  dlr_class = CreateCipClass(kCipDlrClassCode,
                             0, /* # class attributes */
                             7, /* # highest class attribute number */
                             2, /* # class services */
                             11,/* # instance attributes */
                             12,/* # of highest instance attribute */
                             2, /* # instance services */
                             1, /* # instances */
                             "DLR", /* object class name */
                             DLR_CLASS_REVISION,  /* # class revision */
                             NULL /* function pointer for initialization */
                             );

  if (NULL == dlr_class) {
    return kEipStatusError;
  }

  /* Add services to the class */
  InsertService(dlr_class, kGetAttributeSingle,
                GetAttributeSingleDlr, "GetAttributeSingleDlr");
  InsertService(dlr_class, kGetAttributeAll,
                GetAttributeAll, "GetAttributeAll");

  /* Bind attributes to the instance */
  CipInstance *dlr_instance = GetCipInstance(dlr_class, 1u);

  InsertAttribute(dlr_instance,  1, kCipUsint, &g_dlr.network_topology,
                  kGetableSingleAndAll);
  InsertAttribute(dlr_instance,  2, kCipUsint, &g_dlr.network_status,
                  kGetableSingleAndAll);
  InsertAttribute(dlr_instance,  3, kCipUsint, (void *)&s_0xFF_default,
                  kGetableAll);
  InsertAttribute(dlr_instance,  4, kCipAny, (void *)&s_0x00000000_default,
                  kGetableAll);
  InsertAttribute(dlr_instance,  5, kCipUint, (void *)&s_0x0000_default,
                  kGetableAll);
  InsertAttribute(dlr_instance,  6, kCipAny, (void *)&s_zero_node,
                  kGetableAll);
  InsertAttribute(dlr_instance,  7, kCipAny, (void *)&s_zero_node,
                  kGetableAll);
  InsertAttribute(dlr_instance,  8, kCipUint, (void *)&s_0xFFFF_default,
                  kGetableAll);
  /* Attribute #9 is not implemented and also NOT part of the GetAttributesAll
   *  response. Therefore it is not added here! */
  InsertAttribute(dlr_instance, 10, kCipAny, &g_dlr.active_supervisor_address,
                  kGetableSingleAndAll);
  InsertAttribute(dlr_instance, 11, kCipUsint, (void *)&s_0x00_default,
                  kGetableAll);
  InsertAttribute(dlr_instance, 12, kCipDword, &g_dlr.capability_flags,
                  kGetableSingleAndAll);

  /* Set attributes to initial values */
  /* Assume beacon based DLR device. Also all Revision 3 and higher devices
   *  are required to support the Flush_Tables and Learning_Update frames
   *  (see Vol. 2 Section 5-6.2 Revision History of the DLR object).*/
  g_dlr.capability_flags = (kDlrCapBeaconBased | kDlrCapFlushTableFrame);

  return kEipStatusOk;
}
