/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include "ciptcpipinterface.h"

#include <string.h>

#include "opener_user_conf.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "cipethernetlink.h"
#include "opener_api.h"
#include "trace.h"
#include "cipassembly.h"


/** definition of TCP/IP object instance 1 data */
CipTcpIpObject g_tcpip = {
  0x01, /* attribute #1 TCP status with 1 we indicate that we got a valid configuration from DHCP or BOOTP */
  0x04, /* attribute #2 config_capability: This is a default value meaning that it is a DHCP client see 5-3.2.2.2 EIP specification. */
  0x02, /* attribute #3 config_control: 0x02 means that the device shall obtain its interface configuration values via DHCP. */
  {     /* attribute #4 physical link object */
    2,  /* PathSize in 16 Bit chunks */
    CIP_ETHERNETLINK_CLASS_CODE, /* Class Code */
    1,  /* Instance # */
    0   /* Attribute # (not used as this is the EPATH to the EthernetLink object)*/
  },
  { /* attribute #5 interface_configuration */
    0, /* IP address */
    0, /* NetworkMask */
    0, /* Gateway */
    0, /* NameServer */
    0, /* NameServer2 */
    { /* DomainName */
      0, NULL,
    }
  },
  { /* attribute #6 hostname */
    0,
    NULL
  },
  1,  /* attribute #8 mcast TTL value */
  {   /* attribute #9 multicast configuration */
    0,  /* use the default allocation algorithm */
    0,  /* reserved */
    1,  /* we currently use only one multicast address */
    0   /* the multicast address will be allocated on IP address configuration */
  },
  120 /* attribute #13 encapsulation_inactivity_timeout, use a default value of 120 */
};



/************** Functions ****************************************/
EipStatus GetAttributeSingleTcpIpInterface(
  CipInstance *const RESTRICT instance,
  CipMessageRouterRequest *const RESTRICT message_router_request,
  CipMessageRouterResponse *const RESTRICT message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session);

EipStatus GetAttributeAllTcpIpInterface(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session);

EipStatus SetAttributeSingleTcp(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  CipAttributeStruct *attribute = GetCipAttribute(
    instance, message_router_request->request_path.attribute_number);
  (void) instance; /*Suppress compiler warning */
  EipUint16 attribute_number = message_router_request->request_path
                               .attribute_number;

  if (NULL != attribute) {
    uint8_t set_bit_mask = (instance->cip_class->set_bit_mask[CalculateIndex(
                                                                attribute_number)
                            ]);
    if ( set_bit_mask & ( 1 << ( (attribute_number) % 8 ) ) ) {
      switch (attribute_number) {
        case 3: {
          CipUint configuration_control_recieved = GetDintFromMessage(
            &(message_router_request->data) );
          if ( (configuration_control_recieved >= 0x03)
               && (configuration_control_recieved <= 0x0F) ) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;

          } else {

            OPENER_TRACE_INFO(" setAttribute %d\n", attribute_number);

            if (attribute->data != NULL) {
              CipDword *data = (CipDword *) attribute->data;
              *(data) = configuration_control_recieved;
              message_router_response->general_status = kCipErrorSuccess;
            } else {
              message_router_response->general_status = kCipErrorNotEnoughData;
            }
          }
        }
        break;

        case 13: {

          CipUint inactivity_timeout_received = GetIntFromMessage(
            &(message_router_request->data) );

          if (inactivity_timeout_received > 3600) {
            message_router_response->general_status =
              kCipErrorInvalidAttributeValue;
          } else {

            OPENER_TRACE_INFO("setAttribute %d\n", attribute_number);

            if (attribute->data != NULL) {

              CipUint *data = (CipUint *) attribute->data;
              *(data) = inactivity_timeout_received;
              message_router_response->general_status = kCipErrorSuccess;
            } else {
              message_router_response->general_status = kCipErrorNotEnoughData;
            }
          }
        }
        break;

        default:
          message_router_response->general_status =
            kCipErrorAttributeNotSetable;
          break;
      }
    } else {
      message_router_response->general_status = kCipErrorAttributeNotSetable;
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

EipStatus CipTcpIpInterfaceInit() {
  CipClass *tcp_ip_class = NULL;

  if ( ( tcp_ip_class = CreateCipClass(kCipTcpIpInterfaceClassCode, /* class code */
                                       0, /* # class attributes */
                                       7, /* # highest class attribute number */
                                       2, /* # class services */
                                       9, /* # instance attributes */
                                       13, /* # highest instance attribute number */
                                       3, /* # instance services */
                                       1, /* # instances */
                                       "TCP/IP interface", 4, /* # class revision */
                                       NULL /* # function pointer for initialization */
                                       ) ) == 0 ) {
    return kEipStatusError;
  }

  CipInstance *instance = GetCipInstance(tcp_ip_class, 1); /* bind attributes to the instance #1 that was created above */

  InsertAttribute(instance, 1, kCipDword, (void *) &g_tcpip.status,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 2, kCipDword, (void *) &g_tcpip.config_capability,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 3, kCipDword, (void *) &g_tcpip.config_control,
                  kSetAndGetAble);
  InsertAttribute(instance, 4, kCipEpath, &g_tcpip.physical_link_object,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 5, kCipUdintUdintUdintUdintUdintString,
                  &g_tcpip.interface_configuration, kGetableSingleAndAll);
  InsertAttribute(instance, 6, kCipString, (void *) &g_tcpip.hostname,
                  kGetableSingleAndAll);

  InsertAttribute(instance, 8, kCipUsint, (void *) &g_tcpip.mcast_ttl_value,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 9, kCipAny, (void *) &g_tcpip.mcast_config,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 13, kCipUint,
                  (void *) &g_tcpip.encapsulation_inactivity_timeout, kSetAndGetAble);

  InsertService(tcp_ip_class, kGetAttributeSingle,
                &GetAttributeSingleTcpIpInterface,
                "GetAttributeSingleTCPIPInterface");

  InsertService(tcp_ip_class, kGetAttributeAll, &GetAttributeAllTcpIpInterface,
                "GetAttributeAllTCPIPInterface");

  InsertService(tcp_ip_class, kSetAttributeSingle, &SetAttributeSingleTcp,
                "SetAttributeSingle");
  return kEipStatusOk;
}

void ShutdownTcpIpInterface(void) {
  /*Only free the resources if they are initialized */
  if (NULL != g_tcpip.hostname.string) {
    CipFree(g_tcpip.hostname.string);
    g_tcpip.hostname.string = NULL;
  }

  if (NULL != g_tcpip.interface_configuration.domain_name.string) {
    CipFree(g_tcpip.interface_configuration.domain_name.string);
    g_tcpip.interface_configuration.domain_name.string = NULL;
  }
}

EipStatus GetAttributeSingleTcpIpInterface(
  CipInstance *const RESTRICT instance,
  CipMessageRouterRequest *const RESTRICT message_router_request,
  CipMessageRouterResponse *const RESTRICT message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {

  EipUint16 attribute_number = message_router_request->request_path
                               .attribute_number;

  /* Use common handler for all attributes except attribute 9. */
  if (9 != attribute_number) {
    return GetAttributeSingle(instance,
                              message_router_request,
                              message_router_response,
                              originator_address,
                              encapsulation_session);
  }

  { /* attribute 9 can not be easily handled with the default mechanism therefore we will do it by hand */
    EipByte *message = message_router_response->data;

    message_router_response->data_length = 0;
    message_router_response->reply_service = (0x80
                                              | message_router_request->service);
    message_router_response->size_of_additional_status = 0;
    message_router_response->general_status = kCipErrorAttributeNotSupported;

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
                        message_router_request->request_path.attribute_number);

      /* create a reply message containing the data*/
      message_router_response->data_length += EncodeData(
        kCipUsint, &(g_tcpip.mcast_config.alloc_control), &message);
      message_router_response->data_length += EncodeData(
        kCipUsint, &(g_tcpip.mcast_config.reserved_shall_be_zero),
        &message);
      message_router_response->data_length += EncodeData(
        kCipUint,
        &(g_tcpip.mcast_config.number_of_allocated_multicast_addresses),
        &message);

      EipUint32 multicast_address = ntohl(
        g_tcpip.mcast_config.starting_multicast_address);

      message_router_response->data_length += EncodeData(kCipUdint,
                                                         &multicast_address,
                                                         &message);
      message_router_response->general_status = kCipErrorSuccess;
    }
  }

  return kEipStatusOkSend;
}

EipStatus GetAttributeAllTcpIpInterface(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {

  EipUint8 *response = message_router_response->data; /* pointer into the reply */
  CipAttributeStruct *attribute = instance->attributes;

  for (int j = 0; j < instance->cip_class->number_of_attributes; j++) /* for each instance attribute of this class */
  {
    int attribute_number = attribute->attribute_number;
    if (attribute_number < 32) /* only return attributes that are flagged as being part of GetAttributeALl */
    {
      message_router_request->request_path.attribute_number = attribute_number;

      if (8 == attribute_number) { /* insert 6 zeros for the required empty safety network number according to Table 5-3.10 */
        memset(message_router_response->data, 0, 6);
        message_router_response->data += 6;
      }

      if ( kEipStatusOkSend
           != GetAttributeSingleTcpIpInterface(instance, message_router_request,
                                               message_router_response,
                                               originator_address,
                                               encapsulation_session) ) {
        message_router_response->data = response;
        return kEipStatusError;
      }
      message_router_response->data += message_router_response->data_length;

      if (9 == attribute_number) {
        /* returning default value for unimplemented attributes 10,11 and 12 */

        /* attribute 10, type: BOOL, default value: 0 */
        message_router_response->data += 6;
        *(message_router_response->data) = 0;
        message_router_response->data += 1;

        /* attribute 11, type: STRUCT OF USINT, ARRAY of 6 USINTs, ARRAY of 28 USINTs default value: 0 */
        memset(message_router_response->data, 0, 29);
        message_router_response->data += 29;

        /* attribute 12, type: BOOL default value: 0 */
        *(message_router_response->data) = 0;
        message_router_response->data += 1;
      }

    }
    attribute++;
  }
  message_router_response->data_length = message_router_response->data
                                         - response;
  message_router_response->data = response;

  return kEipStatusOkSend;
}

EipUint16 GetEncapsulationInactivityTimeout(CipInstance *instance) {
  CipAttributeStruct *attribute = GetCipAttribute(instance, 13);
  OPENER_ASSERT(NULL != attribute)
  CipUint * data = (CipUint *) attribute->data;
  EipUint16 encapsulation_inactivity_timeout = *data;
  return encapsulation_inactivity_timeout;
}

