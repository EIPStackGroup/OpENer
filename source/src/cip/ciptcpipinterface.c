/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <string.h>

#include "ciptcpipinterface.h"

#include "opener_user_conf.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "cipethernetlink.h"
#include "opener_api.h"
#include "trace.h"
#include "cipassembly.h"

CipDword tcp_status_ = 0x1; /**< #1  TCP status with 1 we indicate that we got a valid configuration from DHCP or BOOTP */
CipDword configuration_capability_ = 0x04; /**< #2  This is a default value meaning that it is a DHCP client see 5-3.2.2.2 EIP specification*/
CipDword configuration_control_ = 0x02; /**< #3  This is a TCP/IP object attribute. 0x02 means that the device shall obtain its interface configuration values via DHCP. */
CipEpath physical_link_object_ = /**< #4 */
{ 2, /**< EIP_UINT16 (UINT) PathSize in 16 Bit chunks*/
  CIP_ETHERNETLINK_CLASS_CODE, /**< EIP_UINT16 ClassID*/
  1, /**< EIP_UINT16 InstanceNr*/
  0 /**< EIP_UINT16 AttributNr (not used as this is the EPATH the EthernetLink object)*/
};

CipTcpIpNetworkInterfaceConfiguration interface_configuration_ = /**< #5 IP, network mask, gateway, name server 1 & 2, domain name*/
{ 0, /* default IP address */
  0, /* NetworkMask */
  0, /* Gateway */
  0, /* NameServer */
  0, /* NameServer2 */
  { /* DomainName */
    0, NULL,
  } };

CipString hostname_ = /**< #6 Hostname*/
{ 0, NULL };
/** @brief #8 the time to live value to be used for multi-cast connections
 *
 * Currently we implement it non set-able and with the default value of 1.
 */
EipUint8 g_time_to_live_value = 1;

/** @brief #9 The multicast configuration for this device
 *
 * Currently we implement it non set-able and use the default
 * allocation algorithm
 */
MulticastAddressConfiguration g_multicast_configuration = { 0, /* us the default allocation algorithm */
                                                            0, /* reserved */
                                                            1, /* we currently use only one multicast address */
                                                            0 /* the multicast address will be allocated on ip address configuration */
};

/** @brief #13 Number of seconds of inactivity before TCP connection is closed
 *
 * Currently we implemented with the default value of 120 seconds.
 */
CipUint g_encapsulation_inactivity_timeout = 120;

/************** Functions ****************************************/
EipStatus GetAttributeSingleTcpIpInterface(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session);

EipStatus GetAttributeAllTcpIpInterface(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session);

EipStatus SetAttributeSingleTcp(
  CipInstance *instance,
  CipMessageRouterRequest *message_router_request,
  CipMessageRouterResponse *message_router_response,
  struct sockaddr *originator_address,
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

  if ( ( tcp_ip_class = CreateCipClass(kCipTcpIpInterfaceClassCode, 0, /* # class attributes*/
                                       7, /* # highest class attribute number*/
                                       2, /* # class services*/
                                       9, /* # instance attributes*/
                                       13, /* # highest instance attribute number*/
                                       3, /* # instance services*/
                                       1, /* # instances*/
                                       "TCP/IP interface", 4, /* # class revision*/
                                       NULL /* # function pointer for initialization*/
                                       ) ) == 0 ) {
    return kEipStatusError;
  }

  CipInstance *instance = GetCipInstance(tcp_ip_class, 1); /* bind attributes to the instance #1 that was created above*/

  InsertAttribute(instance, 1, kCipDword, (void *) &tcp_status_,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 2, kCipDword, (void *) &configuration_capability_,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 3, kCipDword, (void *) &configuration_control_,
                  kSetAndGetAble);
  InsertAttribute(instance, 4, kCipEpath, &physical_link_object_,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 5, kCipUdintUdintUdintUdintUdintString,
                  &interface_configuration_, kGetableSingleAndAll);
  InsertAttribute(instance, 6, kCipString, (void *) &hostname_,
                  kGetableSingleAndAll);

  InsertAttribute(instance, 8, kCipUsint, (void *) &g_time_to_live_value,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 9, kCipAny, (void *) &g_multicast_configuration,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 13, kCipUint,
                  (void *) &g_encapsulation_inactivity_timeout, kSetAndGetAble);

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
  if (NULL != hostname_.string) {
    CipFree(hostname_.string);
    hostname_.string = NULL;
  }

  if (NULL != interface_configuration_.domain_name.string) {
    CipFree(interface_configuration_.domain_name.string);
    interface_configuration_.domain_name.string = NULL;
  }
}

EipStatus GetAttributeSingleTcpIpInterface(
  CipInstance *const RESTRICT instance,
  CipMessageRouterRequest *RESTRICT const message_router_request,
  CipMessageRouterResponse *RESTRICT const message_router_response,
  struct sockaddr *originator_address,
  const int encapsulation_session) {

  EipStatus status = kEipStatusOkSend;
  EipByte *message = message_router_response->data;

  message_router_response->data_length = 0;
  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;

  EipUint16 attribute_number = message_router_request->request_path
                               .attribute_number;
  uint8_t get_bit_mask = 0;

  message_router_response->general_status = kCipErrorAttributeNotSupported;

  if (9 == message_router_request->request_path.attribute_number) { /* attribute 9 can not be easily handled with the default mechanism therefore we will do it by hand */
    if (kGetAttributeAll == message_router_request->service) {
      get_bit_mask = (instance->cip_class->get_all_bit_mask[CalculateIndex(
                                                              attribute_number)]);
      message_router_response->general_status = kCipErrorSuccess;
    } else {
      get_bit_mask = (instance->cip_class->get_single_bit_mask[CalculateIndex(
                                                                 attribute_number)
                      ]);
    }

    if ( 0 == ( get_bit_mask & ( 1 << (attribute_number % 8) ) ) ) {
      return kEipStatusOkSend;
    }
    message_router_response->general_status = kCipErrorSuccess;

    message_router_response->data_length += EncodeData(
      kCipUsint, &(g_multicast_configuration.alloc_control), &message);
    message_router_response->data_length += EncodeData(
      kCipUsint, &(g_multicast_configuration.reserved_shall_be_zero),
      &message);
    message_router_response->data_length += EncodeData(
      kCipUint,
      &(g_multicast_configuration.number_of_allocated_multicast_addresses),
      &message);

    EipUint32 multicast_address = ntohl(
      g_multicast_configuration.starting_multicast_address);

    message_router_response->data_length += EncodeData(kCipUdint,
                                                       &multicast_address,
                                                       &message);
  } else {
    CipAttributeStruct *attribute = GetCipAttribute(instance, attribute_number);

    if ( (NULL != attribute) && ( NULL != attribute->data) ) {

      OPENER_TRACE_INFO("getAttribute %d\n",
                        message_router_request->request_path.attribute_number); /* create a reply message containing the data*/

      if (kGetAttributeAll == message_router_request->service) {
        get_bit_mask = (instance->cip_class->get_all_bit_mask[CalculateIndex(
                                                                attribute_number)
                        ]);
        message_router_response->general_status = kCipErrorSuccess;
      } else {
        get_bit_mask = (instance->cip_class->get_single_bit_mask[CalculateIndex(
                                                                   attribute_number)
                        ]);
      }

      if ( 0 == ( get_bit_mask & ( 1 << ( (attribute_number) % 8 ) ) ) ) {
        return kEipStatusOkSend;
      }

      /*TODO think if it is better to put this code in an own
       * getAssemblyAttributeSingle functions which will call get attribute
       * single.
       */

      if (attribute->type == kCipByteArray
          && instance->cip_class->class_id == kCipAssemblyClassCode) {
        /* we are getting a byte array of a assembly object, kick out to the app callback */
        OPENER_TRACE_INFO(" -> getAttributeSingle CIP_BYTE_ARRAY\r\n");
        BeforeAssemblyDataSend(instance);
      }

      message_router_response->data_length = EncodeData(attribute->type,
                                                        attribute->data,
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
  struct sockaddr *originator_address,
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
  OPENER_ASSERT(NULL != attribute);
  CipUint *data = (CipUint *) attribute->data;
  EipUint16 encapsulation_inactivity_timeout = *data;
  return encapsulation_inactivity_timeout;
}

