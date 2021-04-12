/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the EtherNet/IP Security object
 * @author Markus Pešek <markus.pesek@tuwien.ac.at>
 *
 *  EtherNet/IP Security object
 *  ===================
 *
 *  This module implements the EtherNet/IP Security object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  1: State
 *  - Attribute  2: Capability Flags
 *  - Attribute  3: Available Cipher Suites
 *  - Attribute  4: Allowed Cipher Suites
 *  - Attribute  5: Pre-Shared Keys
 *  - Attribute  6: Active Device Certificates
 *  - Attribute  7: Trusted Authorities
 *  - Attribute  8: Certificate Revocation List
 *  - Attribute  9: Verify Client Certificate
 *  - Attribute  10: Send Certificate Chain
 *  - Attribute  11: Check Expiration
 *  - Attribute  12: Trusted Identities
 *  - Attribute  13: Pull Model Enable
 *  - Attribute  14: Pull Model Status
 *  - Attribute  15: DTLS Timeout
 *  - Attribute  16: UDP-only Policy
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributesAll
 *  - Reset
 *  - GetAttributeSingle
 *  - SetAttributeSingle
 *  - Begin_Config
 *  - Kick_Timer
 *  - Apply_Config
 *  - Abort_Config
 */

/* ********************************************************************
 * include files
 */
#include "string.h"
#include <stdio.h>

#include "ethernetipsecurity.h"

#include "cipcommon.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is 5 */
#define ETHERNET_IP_SECURITY_OBJECT_REVISION 5

/* ********************************************************************
 * global public variables
 */
/**< definition of EtherNet/IP Security object instance 1 data */
EIPSecurityObject g_eip_security;

/* ********************************************************************
 * public functions
 */

/** @brief EtherNet/IP Security Object Reset service
 *
 * Return this EtherNet/IP Security Object Instance to the
 * Factory Default Configuration State.
 * See Vol.8 Section 5-4.5.1
 */
EipStatus EIPSecurityObjectReset(CipInstance *RESTRICT const instance){

   return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Begin_Config service
 *
 * Causes the object to transition to the Configuration In Progress state.
 * See Vol.8 Section 5-4.7.1
 */
EipStatus EIPSecurityObjectBeginConfig(CipInstance *RESTRICT const instance){

   return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Kick_Timer service
 *
 * Causes the object to reset the configuration session timer.
 * See Vol.8 Section 5-4.7.2
 */
EipStatus EIPSecurityObjectKickTimer(CipInstance *RESTRICT const instance){

   return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Apply_Config service
 *
 * Applies the configuration and places the object in the Configured state.
 * See Vol.8 Section 5-4.7.3
 */
EipStatus EIPSecurityObjectApplyConfig(CipInstance *RESTRICT const instance){

   return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Abort_Config service
 *
 * Abort the current configuration and discard pending changes.
 * See Vol.8 Section 5-4.7.4
 */
EipStatus EIPSecurityObjectAbortConfig(CipInstance *RESTRICT const instance){

   return kEipStatusOk;
}

void FinalizeMessage(CipUsint general_status,
                     CipMessageRouterRequest *message_router_request,
                     CipMessageRouterResponse *message_router_response) {
  message_router_response->general_status = general_status;
  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service = (0x80 | message_router_request->service);
}

EipStatus SetAttributeSingleEIPSecurityObject(
    CipInstance *instance,
    CipMessageRouterRequest *message_router_request,
    CipMessageRouterResponse *message_router_response,
    const struct sockaddr *originator_address,
    const int encapsulation_session) {

  EipUint16 attribute_number = message_router_request->request_path.attribute_number;
  CipAttributeStruct *attribute = GetCipAttribute(instance, attribute_number);

  /* we don't have this attribute */
  if (NULL == attribute) {
    FinalizeMessage(kCipErrorAttributeNotSupported,
                    message_router_request,
                    message_router_response);
    return kEipStatusOkSend;
  }

  uint8_t set_bit_mask = (instance->cip_class->set_bit_mask[
      CalculateIndex(attribute_number)
  ]);
  if (set_bit_mask & (1 << ((attribute_number) % 8))) {
    FinalizeMessage(kCipErrorAttributeNotSetable,
                    message_router_request,
                    message_router_response);
    return kEipStatusOkSend;
  }

  if (attribute->data == NULL) {
    FinalizeMessage(kCipErrorNotEnoughData,
                    message_router_request,
                    message_router_response);
    return kEipStatusOkSend;
  }

  // Execute PreSetCallback iff available
  if ((attribute->attribute_flags & kPreSetFunc) && instance->cip_class->PreSetCallback ) {
    instance->cip_class->PreSetCallback(instance, attribute, message_router_request->service);
  }

  CipDword *data = (CipDword *) attribute->data;

  OPENER_TRACE_INFO(" setAttribute %d\n", attribute_number);

  switch (attribute_number) {
    case 4: { /** Attribute 4: Allowed Cipher Suites **/
      CipUsint number_of_cipher_suites = GetUsintFromMessage(
          &(message_router_request->data));

      EIPSecurityObjectCipherSuites *allowed_cipher_suites = data;

      //TODO: Cleanup old CipherSuiteIds

      allowed_cipher_suites->number_of_cipher_suites = number_of_cipher_suites;
      if (number_of_cipher_suites>0) {
        EIPSecurityObjectCipherSuiteId *cipher_suite_ids =
            CipCalloc(number_of_cipher_suites, sizeof(EIPSecurityObjectCipherSuiteId));

        memcpy(cipher_suite_ids,
               message_router_request->data,
               number_of_cipher_suites * sizeof(EIPSecurityObjectCipherSuiteId));
//        for (size_t i = 0; i < number_of_cipher_suites; i++) {
//          cipher_suite_ids[i].iana_first_byte = GetUsintFromMessage(
//              &(message_router_request->data));
//          cipher_suite_ids[i].iana_second_byte = GetUsintFromMessage(
//              &(message_router_request->data));
//        }

        allowed_cipher_suites->cipher_suite_ids = cipher_suite_ids;
      } else {
        allowed_cipher_suites->cipher_suite_ids = NULL;
      }
      message_router_response->general_status = kCipErrorSuccess;
    }
      break;

    case 5: { /** Attribute 5: Pre-Shared Keys **/
      CipUsint number_of_psk = GetUsintFromMessage(&(message_router_request->data));
      EIPSecurityObjectPreSharedKeys *pre_shared_keys = data;

      // At present, a maximum of 1 PSK may be configured
      if (number_of_psk > 1) {
        message_router_response->general_status = kCipErrorInvalidAttributeValue;
        break;
      }

      if (number_of_psk == 1) {
        EIPSecurityObjectPreSharedKey *psk_structure =
            CipCalloc(number_of_psk, sizeof(EIPSecurityObjectPreSharedKey));

        psk_structure->psk_identity_size = GetUsintFromMessage(&(message_router_request->data));

        if (psk_structure->psk_identity_size <= SIZE_MAX_PSK_IDENTITY){
          CipOctet *psk_identity = CipCalloc(psk_structure->psk_identity_size, sizeof(CipOctet));

          memcpy(psk_identity,
                 message_router_request->data,
                 psk_structure->psk_identity_size);
          message_router_request->data += psk_structure->psk_identity_size;
//          for (int i=0; i<psk_structure->psk_identity_size; i++) {
//            psk_identity[i] = GetByteFromMessage(&(message_router_request->data));
//          }

          psk_structure->psk_identity = psk_identity;
          psk_structure->psk_size = GetUsintFromMessage(&(message_router_request->data));

          if(psk_structure->psk_size <= SIZE_MAX_PSK) {
            CipOctet *psk = CipCalloc(psk_structure->psk_size, sizeof(CipOctet));

            memcpy(psk, message_router_request->data, psk_structure->psk_size);
//            for (int i=0; i<psk_structure->psk_size; i++) {
//              psk[i] = GetByteFromMessage(&(message_router_request->data));
//            }

            psk_structure->psk = psk;
            //TODO: Cleanup existing PSKs
            pre_shared_keys->pre_shared_keys = psk_structure;
            message_router_response->general_status = kCipErrorSuccess;
          } else {
            if (psk_identity != NULL){
              CipFree(psk_identity);
              psk_structure->psk_identity = NULL;
            }
            if (psk_structure != NULL) {
              CipFree(psk_structure);
            }
            message_router_response->general_status = kCipErrorInvalidAttributeValue;
          }
        } else {
          if (psk_structure != NULL) {
            CipFree(psk_structure);
          }
          message_router_response->general_status = kCipErrorInvalidAttributeValue;
        }
      } else {
        //TODO: Cleanup existing PSKs
        pre_shared_keys->number_of_pre_shared_keys = number_of_psk; //0
        pre_shared_keys->pre_shared_keys=NULL;
        message_router_response->general_status = kCipErrorSuccess;
      }
    }
      break;

    default:
      message_router_response->general_status = kCipErrorAttributeNotSetable;
      break;
  } //end of switch

  message_router_response->size_of_additional_status = 0;
  message_router_response->reply_service = (0x80 | message_router_request->service);

  return kEipStatusOkSend;
}

void EncodeEIPSecurityObjectCipherSuiteId(const void *const data,
                                          ENIPMessage *const outgoing_message)
{
  EIPSecurityObjectCipherSuiteId *cipher_suite_id =
      (EIPSecurityObjectCipherSuiteId *) data;

  EncodeCipUsint(&(cipher_suite_id->iana_first_byte), outgoing_message);
  EncodeCipUsint(&(cipher_suite_id->iana_second_byte), outgoing_message);
}

void EncodeEIPSecurityObjectCipherSuites(const void *const data,
                                           ENIPMessage *const outgoing_message)
{
  EIPSecurityObjectCipherSuites *cipher_suites =
      (EIPSecurityObjectCipherSuites *) data;

  EncodeCipUsint(&(cipher_suites->number_of_cipher_suites), outgoing_message);

  for (int i=0; i<cipher_suites->number_of_cipher_suites; i++) {
    EncodeEIPSecurityObjectCipherSuiteId(&(cipher_suites->cipher_suite_ids[i]),
                                         outgoing_message);
  }
}

void EncodeEIPSecurityObjectPath(const void *const data,
                                       ENIPMessage *const outgoing_message)
{
  EIPSecurityObjectPath *path = (EIPSecurityObjectPath *) data;

  EncodeCipUsint(&(path->path_size), outgoing_message);
  EncodeCipEPath(&(path->path), outgoing_message);

}

void EncodeEIPSecurityObjectPathList(const void *const data,
                                       ENIPMessage *const outgoing_message)
{
  EIPSecurityObjectPathList *path_list = (EIPSecurityObjectPathList *) data;

  EncodeCipUsint(&(path_list->number_of_paths), outgoing_message);

  for (int i=0; i<path_list->number_of_paths; i++) {
    EncodeEIPSecurityObjectPath(&(path_list->paths[i]), outgoing_message);
  }
}

/**
 * When accessed via Get_Attributes_All or Get_Attribute_Single, the Size of
 * PSK element shall be 0, and 0 bytes of PSK value shall be returned.
 * This ensures that the PSK value cannot be read out of the device,
 * as it is a confidential piece of information.
 */
void EncodeEIPSecurityObjectPreSharedKeys(const void *const data,
                                         ENIPMessage *const outgoing_message) {
  AddSintToMessage(0, outgoing_message);
}


EipStatus EIPSecurityInit(void) {
  CipClass *eip_security_object_class = NULL;
  CipInstance *eip_security_object_instance;

  eip_security_object_class =
      CreateCipClass(kEIPSecurityObjectClassCode,
                     0, /* # class attributes */
                     7, /* # highest class attribute number */
                     2, /* # class services */
                     16, /* # instance attributes */
                     16, /* # highest instance attribute number */
                     8, /* # instance services */
                     1, /* # instances*/
                     "EtherNet/IP Security Object",
                     ETHERNET_IP_SECURITY_OBJECT_REVISION, /* # class revision */
                     NULL /* # function pointer for initialization */
      );

  if (NULL == eip_security_object_class) {
    /* Initialization failed */
    return kEipStatusError;
  }

  /* Bind attributes to the instance created above */
  eip_security_object_instance = GetCipInstance(eip_security_object_class, 1);

  InsertAttribute(eip_security_object_instance,
                  1,
                  kCipUsint,
                  EncodeCipUsint,
                  &g_eip_security.state,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  2,
                  kCipWord,
                  EncodeCipDword,
                  &g_eip_security.capability_flags,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  3,
                  kCipAny,
                  EncodeEIPSecurityObjectCipherSuites,
                  &g_eip_security.available_cipher_suites,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  4,
                  kCipAny,
                  EncodeEIPSecurityObjectCipherSuites,
                  &g_eip_security.allowed_cipher_suites,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  5,
                  kCipAny,
                  EncodeEIPSecurityObjectPreSharedKeys,
                  &g_eip_security.pre_shared_keys,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  6,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  &g_eip_security.active_device_certificates,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  7,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  &g_eip_security.trusted_authorities,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  8,
                  kCipAny,
                  EncodeEIPSecurityObjectPath,
                  &g_eip_security.certificate_revocation_list,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  9,
                  kCipBool,
                  EncodeCipBool,
                  &g_eip_security.verify_client_certificate,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  10,
                  kCipBool,
                  EncodeCipBool,
                  &g_eip_security.send_certificate_chain,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  11,
                  kCipBool,
                  EncodeCipBool,
                  &g_eip_security.check_expiration,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  12,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  &g_eip_security.trusted_identities,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  13,
                  kCipBool,
                  EncodeCipBool,
                  &g_eip_security.pull_model_enabled,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  14,
                  kCipUint,
                  EncodeCipUint,
                  &g_eip_security.pull_model_status,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  15,
                  kCipUint,
                  EncodeCipUint,
                  &g_eip_security.dtls_timeout,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  16,
                  kCipUsint,
                  EncodeCipUsint,
                  &g_eip_security.udp_only_policy,
                  kSetAndGetAble
  );

  /* Add services to the instance */
  InsertService(eip_security_object_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectResetServiceCode,
                &EIPSecurityObjectReset,
                "EIPSecurityObjectReset"
  );
  InsertService(eip_security_object_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle"
  );
  InsertService(eip_security_object_class,
                kSetAttributeSingle,
                &SetAttributeSingleEIPSecurityObject,
                "SetAttributeSingleEIPSecurityObject"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectBeginConfigServiceCode,
                &EIPSecurityObjectBeginConfig,
                "EIPSecurityObjectBeginConfig"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectKickTimerServiceCode,
                &EIPSecurityObjectKickTimer,
                "EIPSecurityObjectKickTimer"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectApplyConfigServiceCode,
                &EIPSecurityObjectApplyConfig,
                "EIPSecurityObjectApplyConfig"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectAbortConfigServiceCode,
                &EIPSecurityObjectAbortConfig,
                "EIPSecurityObjectAbortConfig"
  );

  return kEipStatusOk;
}
