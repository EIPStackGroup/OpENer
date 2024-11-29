/*******************************************************************************
 * Copyright (c) 2024, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "ciplldpmanagement.h"

static CipLldpManagementObjectValues g_lldp_management_object_instance_values;

void CipLldpManagementEncodeLldpEnable(const void *const data,
                                       ENIPMessage *const outgoing_message) {
  //TODO: Add Code to encode LLDP enable to a message
}

void CipStringIDecodeFromMessage(CipLldpManagementEncodeLldpEnable *data_to,
                                 CipMessageRouterRequest *const
                                 message_router_request) {
  // TODO: Add code to decode the LLDP Enable attribute to the message

}

EipStatus kCipLldpManagementInit(void) {
  CipClass *lldp_management_class = NULL;

  if ( ( lldp_management_class = CreateCipClass(
           kCipLldpManagementClassCode, 7, /* # class attributes */
           7,                    /* # highest class attribute number */
           2,                    /* # class services */
           5,                    /* # instance attributes */
           5,                    /* # highest instance attribute number */
           3,                    /* # instance services */
           1,                    /* # instances */
           "LLDP Management", 1, /* # class revision */
           NULL                  /* # function pointer for initialization */
           ) ) == 0 ) {
    return kEipStatusError;
  }

  CipInstance *instance = GetCipInstance(lldp_management_class, 1);   /* bind attributes to the instance #1 that was created above */

  InsertAttribute(instance, 1, kCipAny, CipLldpManagementEncodeLldpEnable,
                  CipLldpManagementDecodeLldpEnable,
                  &g_lldp_management_object_instance_values.lldp_enable,
                  kSetAndGetAble);
  InsertAttribute(instance, 2, kCipUint, EncodeCipUint, DecodeCipUint,
                  &g_lldp_management_object_instance_values.msg_tx_interval,
                  kSetAndGetAble);
  InsertAttribute(instance, 3, kCipUint, EncodeCipUsint, DecodeCipUsint,
                  &g_lldp_management_object_instance_values.msg_tx_hold,
                  kSetAndGetAble);
  InsertAttribute(instance, 4, kCipWord, EncodeCipWord, DecodeCipWord,
                  &g_lldp_management_object_instance_values.lldp_datastore,
                  kSetAndGetAble);
  InsertAttribute(instance, 5, kCipUdint, EncodeCipUdint, DecodeCipUdint,
                  &g_lldp_management_object_instance_values.last_change,
                  kSetAndGetAble);
}