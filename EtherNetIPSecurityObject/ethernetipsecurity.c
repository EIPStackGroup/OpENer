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
define ETHERNET_IP_SECURITY_OBJECT_REVISION 5

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

}

/** @brief EtherNet/IP Security Object Begin_Config service
 *
 * Causes the object to transition to the Configuration In Progress state.
 * See Vol.8 Section 5-4.7.1
 */
EipStatus EIPSecurityObjectBeginConfig(CipInstance *RESTRICT const instance){


}

/** @brief EtherNet/IP Security Object Kick_Timer service
 *
 * Causes the object to reset the configuration session timer.
 * See Vol.8 Section 5-4.7.2
 */
EipStatus EIPSecurityObjectKickTimer(CipInstance *RESTRICT const instance){


}

/** @brief EtherNet/IP Security Object Apply_Config service
 *
 * Applies the configuration and places the object in the Configured state.
 * See Vol.8 Section 5-4.7.3
 */
EipStatus EIPSecurityObjectApplyConfig(CipInstance *RESTRICT const instance){


}

/** @brief EtherNet/IP Security Object Abort_Config service
 *
 * Abort the current configuration and discard pending changes.
 * See Vol.8 Section 5-4.7.4
 */
EipStatus EIPSecurityObjectAbortConfig(CipInstance *RESTRICT const instance){


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
  EncodeCipEpath(&(path->path), outgoing_message);

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


EipStatus EipSecurityInit(void) {
  CipClass *eip_security_object_class = NULL;
  CipInstance *eip_security_object_instance;

  cip_security_object_class =
      CreateCipClass(kEIPSecurityObjectClassCode,
                     0, /* # class attributes */
                     7, /* # highest class attribute number */
                     2, /* # class services */
                     16, /* # instance attributes */
                     16, /* # highest instance attribute number */
                     8, /* # instance services */
                     1, /* # instances*/
                     "EtherNet/IP Security Object",
                     ETHERNET_IIP_SECURITY_OBJECT_REVISION, /* # class revision */
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
                &SetAttributeSingle,
                "SetAttributeSingle"
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