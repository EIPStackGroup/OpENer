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
 *  - Attribute  5: Pre-Shared Keys - at present, a maximum of 1 PSK may be configured
 *  - Attribute  6: Active Device Certificates - at present, a maximum of 1 entry may be configured
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

#include "cipcommon.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

#include "certificatemanagement.h"
#include "cipsecurity.h"
#include "ethernetipsecurity.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is 7 */
#define ETHERNET_IP_SECURITY_OBJECT_REVISION 7

#define ETHERNET_IP_SECURITY_CONFIG_SESSION_DEFAULT_TIMEOUT 10000U // 10 sec

/**
 * declaration of (static) Ethernet/IP Security object class data
 */
EIPSecurityObjectClassAttributes eip_security_class_attr = {
  .number_of_psks_supported = 0,
  .psk_usages_supported = 4  // Bit 2 set
};

/* ********************************************************************
 * global public variables
 */
/**< definition of EtherNet/IP Security object instance 1 data */

CipEpath CMO_Paths[1] = {  // Certificate Management object paths
  {
    2,                                         /* PathSize in 16 Bit chunks */
    kCertificateManagementObjectClassCode,     /* Class Code */
    0x01,                                      /* Instance # */
  }
};

const EIPSecurityObjectPathList active_device_certificates = {
  // at present, a maximum of 1 entry may be configured
  1,
  CMO_Paths,
};

#define number_of_required_cipher_suites 8

EIPSecurityObjectCipherSuiteId const TLS_RSA_WITH_NULL_SHA256 = {
  0x00,
  0x3B
};
EIPSecurityObjectCipherSuiteId const TLS_RSA_WITH_AES_128_CBC_SHA256 = {
  0x00,
  0x3C
};
EIPSecurityObjectCipherSuiteId const TLS_RSA_WITH_AES_256_CBC_SHA256 = {
  0x00,
  0x3D
};
EIPSecurityObjectCipherSuiteId const TLS_ECDHE_ECDSA_WITH_NULL_SHA = {
  0xC0,
  0x06
};
EIPSecurityObjectCipherSuiteId const TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256 = {
  0xC0,
  0x23
};
EIPSecurityObjectCipherSuiteId const TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384 = {
  0xC0,
  0x24
};
EIPSecurityObjectCipherSuiteId const TLS_ECDHE_PSK_WITH_NULL_SHA256 = {
  0xC0,
  0x3A,
};
EIPSecurityObjectCipherSuiteId const TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256 = {
  0xC0,
  0x37
};
EIPSecurityObjectCipherSuiteId const ALLOWED_CIPHER_FREE_ENTRY = {
  0x00,
  0x00
};

const EIPSecurityObjectCipherSuites available_cipher_suites = {
  .number_of_cipher_suites = number_of_required_cipher_suites,
  .cipher_suite_ids = {
    TLS_RSA_WITH_NULL_SHA256,
    TLS_RSA_WITH_AES_128_CBC_SHA256,
    TLS_RSA_WITH_AES_256_CBC_SHA256,
    TLS_ECDHE_ECDSA_WITH_NULL_SHA,
    TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
    TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,
    TLS_ECDHE_PSK_WITH_NULL_SHA256,
    TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256,
  }
};

const EIPSecurityObjectCipherSuites default_allowed_cipher_suites = {
  .number_of_cipher_suites = 2,
  .cipher_suite_ids = {
    TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
    TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,
    ALLOWED_CIPHER_FREE_ENTRY,
    ALLOWED_CIPHER_FREE_ENTRY,
    ALLOWED_CIPHER_FREE_ENTRY,
    ALLOWED_CIPHER_FREE_ENTRY,
    ALLOWED_CIPHER_FREE_ENTRY,
    ALLOWED_CIPHER_FREE_ENTRY
  }
};

//default EIPSecurityObject values
EIPSecurityObject g_eip_security_default = {
  .state = kEIPSecurityObjectStateValueFactoryDefaultConfiguration,   /** Attribute #1 */
  .capability_flags = 0,                            /** Attribute #2 */
  .available_cipher_suites = available_cipher_suites,                  /** Attribute #3 */
  .allowed_cipher_suites = default_allowed_cipher_suites,                         /** Attribute #4 */
  .pre_shared_keys.number_of_pre_shared_keys = 0,  //default = 0  /** Attribute #5 */
  .active_device_certificates = active_device_certificates,   /** Attribute #6 */
  .verify_client_certificate = false,                     /** Attribute #9 */
  .send_certificate_chain = false,                        /** Attribute #10 */
  .check_expiration = 0,                            /** Attribute #11 */
  .pull_model_enabled = true,    // default: true   /** Attribute #13 */
  .pull_model_status = 0x0000,                      /** Attribute #14 */
  .dtls_timeout = 12,    // default: 12 seconds   /** Attribute #15 */
  .udp_only_policy = 0,                             /** Attribute #16 */
  //TODO: add missing default values
  //  .trusted_authorities,                           /** Attribute #7 */
  //  .certificate_revocation_list,                   /** Attribute #8 */
  //  .trusted_identities,                            /** Attribute #12 */
};

// Instance 1 object
EIPSecurityObject g_eip_security = {
  .state = kEIPSecurityObjectStateValueFactoryDefaultConfiguration,   /** Attribute #1 */
  .capability_flags = 0,                            /** Attribute #2 */
  .available_cipher_suites = available_cipher_suites,                  /** Attribute #3 */
  .allowed_cipher_suites = default_allowed_cipher_suites,                         /** Attribute #4 */
  .pre_shared_keys.number_of_pre_shared_keys = 0,  //default = 0  /** Attribute #5 */
  .active_device_certificates = active_device_certificates,   /** Attribute #6 */
  .verify_client_certificate = false,                     /** Attribute #9 */
  .send_certificate_chain = false,                        /** Attribute #10 */
  .check_expiration = 0,                            /** Attribute #11 */
  .pull_model_enabled = true,    // default: true   /** Attribute #13 */
  .pull_model_status = 0x0000,                      /** Attribute #14 */
  .dtls_timeout = 12,    // default: 12 seconds   /** Attribute #15 */
  .udp_only_policy = 0,                             /** Attribute #16 */
  //TODO: add missing default values
  //  .trusted_authorities,                           /** Attribute #7 */
  //  .certificate_revocation_list,                   /** Attribute #8 */
  //  .trusted_identities,                            /** Attribute #12 */
};

MilliSeconds eipSecurityConfigSessionTimeout =
  ETHERNET_IP_SECURITY_CONFIG_SESSION_DEFAULT_TIMEOUT;

/* ********************************************************************
 * public functions
 */

/** @brief EtherNet/IP Security Object Reset settable attributes
 *
 *  Return all settable instance attributes to the
 *  Factory Default Configuration value
 */
void EIPSecurityObjectResetSettableAttributes(CipInstance *instance) {
  if(1 == instance->instance_number) {

    g_eip_security.allowed_cipher_suites =
      g_eip_security_default.allowed_cipher_suites;

    g_eip_security.pre_shared_keys = g_eip_security_default.pre_shared_keys;

    g_eip_security.active_device_certificates =
      g_eip_security_default.active_device_certificates;

    g_eip_security.trusted_authorities =
      g_eip_security_default.trusted_authorities;

    g_eip_security.certificate_revocation_list =
      g_eip_security_default.certificate_revocation_list;

    g_eip_security.verify_client_certificate =
      g_eip_security_default.verify_client_certificate;

    g_eip_security.send_certificate_chain =
      g_eip_security_default.send_certificate_chain;

    g_eip_security.check_expiration = g_eip_security_default.check_expiration;

    g_eip_security.trusted_identities =
      g_eip_security_default.trusted_identities;

    g_eip_security.dtls_timeout = g_eip_security_default.dtls_timeout;

    g_eip_security.udp_only_policy = g_eip_security_default.udp_only_policy;
  }
}

/** EtherNet/IP Security Object PreResetCallback
 *
 *  Used for common Reset service
 *
 *  Return this EtherNet/IP Security Object Instance to the
 *  Factory Default Configuration State.
 *  @See Vol.8, Chapter 5-4.5.1
 */
EipStatus EIPSecurityObjectPreResetCallback(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response) {
  CipAttributeStruct *attribute = GetCipAttribute(instance, 1);
  CipUint *state = attribute->data;

  if (*state == kEIPSecurityObjectStateValueConfigured) {
    CipBool sent_over_TLS = false;  // TODO: check for valid TLS connection
    if (!sent_over_TLS) {
      message_router_response->general_status =
        kCipErrorPrivilegeViolation;    // 0x0F Permission Denied
      return kEipStatusOk;
    }
  }

  CipBool enable_pull_model = true; /* The default value if parameter was omitted. */
  CipUint pull_model_status = 0x0000;

  if (message_router_request->request_data_size == 1) {
    enable_pull_model = GetBoolFromMessage(&message_router_request->data);
  }

  if (enable_pull_model) {
    *state = kEIPSecurityObjectStateValueFactoryDefaultConfiguration;
  } else {
    pull_model_status = 0xFFFF;
    *state = kEIPSecurityObjectStateValuePullModelDisabled;
  }

  attribute = GetCipAttribute(instance, 13);  // attribute #13 pull model enable
  *(CipBool *)attribute->data = enable_pull_model;  // set value

  attribute = GetCipAttribute(instance, 14);  // attribute #14 pull model status
  *(CipUint *)attribute->data = pull_model_status;  // set value

  /* Reset settable attributes of each existing EtherNet/IP Security Object to
   * factory default */
  EIPSecurityObjectResetSettableAttributes(instance);

  message_router_response->general_status = kCipErrorSuccess;
  return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Begin_Config service
 *
 *  Causes the object to transition to the Configuration In Progress state.
 *  @See Vol.8, Chapter 5-4.7.1
 */
EipStatus EIPSecurityObjectBeginConfig(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  message_router_response->general_status = kCipErrorSuccess;
  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service =
    (0x80 | message_router_request->service);

  CipAttributeStruct *attribute = GetCipAttribute(instance, 1);  // attribute #1 state
  CipUsint state = *(CipUsint *)attribute->data;

  if (kEIPSecurityObjectStateValueFactoryDefaultConfiguration != state) {
    message_router_response->general_status = kCipErrorObjectStateConflict;
  } else {
    // TODO: save current instance config before starting new config
    *(CipUsint *)attribute->data =
      kEIPSecurityObjectStateValueConfigurationInProgress;                               // set state
  }

  return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Kick_Timer service
 *
 *  Causes the object to reset the configuration session timer.
 *  @See Vol.8, Chapter 5-4.7.2
 */
EipStatus EIPSecurityObjectKickTimer(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  message_router_response->general_status = kCipErrorObjectStateConflict;
  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service =
    (0x80 | message_router_request->service);

  CipAttributeStruct *attribute = GetCipAttribute(instance, 1);  // attribute #1 - state
  CipUsint state = *(CipUsint *)attribute->data;

  if (kEIPSecurityObjectStateValueConfigurationInProgress == state) {
    // reset configuration session timer
    eipSecurityConfigSessionTimeout =
      ETHERNET_IP_SECURITY_CONFIG_SESSION_DEFAULT_TIMEOUT;
    message_router_response->general_status = kCipErrorSuccess;
  }

  return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Apply_Config service
 *
 *  Applies the configuration and places the object in the Configured state.
 *  @See Vol.8, Chapter 5-4.7.3
 */
EipStatus EIPSecurityObjectApplyConfig(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  message_router_response->general_status = kCipErrorObjectStateConflict;
  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service =
    (0x80 | message_router_request->service);

  CipAttributeStruct *attribute = GetCipAttribute(instance, 1);  // attribute #1 state
  CipUsint state = *(CipUsint *)attribute->data;

  if (kEIPSecurityObjectStateValueConfigurationInProgress == state) {
    /* The default values if parameters were omitted. */
    CipWord apply_behavior_flags = 0;
    CipUint close_delay = 0;

    if (0 < message_router_request->request_data_size) {
      apply_behavior_flags =
        GetWordFromMessage( &(message_router_request->data) );
      close_delay = GetUintFromMessage( &(message_router_request->data) );
    }

    // check apply behavior
    if ( apply_behavior_flags & (1 << 0) ) {  // Bit 0 set
      // TODO: close existing connections once close_delay has elapsed
    }
    if ( apply_behavior_flags & (1 << 1) ) {  // Bit 1 set
      // TODO: run Object_Cleanup service of the CIP Security Object after
      // applying changes
    }

    // TODO: Apply config
    /* device shall begin using the new attribute
     * settings when establishing new (D)TLS sessions. */

    // TODO: change state to configured
    //*(CipUsint*) attribute->data = kEIPSecurityObjectStateValueConfigured;
    message_router_response->general_status = kCipErrorSuccess;
  }

  return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Abort_Config service
 *
 *  Abort the current configuration and discard pending changes.
 *  @See Vol.8, Chapter 5-4.7.4
 */
EipStatus EIPSecurityObjectAbortConfig(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  message_router_response->general_status = kCipErrorObjectStateConflict;
  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service =
    (0x80 | message_router_request->service);

  CipAttributeStruct *attribute = GetCipAttribute(instance, 1);  // attribute #1 state
  CipUsint state = *(CipUsint *)attribute->data;

  if (kEIPSecurityObjectStateValueConfigurationInProgress == state) {
    // TODO: implement service

    // TODO: change back to state before configuration in progress
    *(CipUsint *)attribute->data = kEIPSecurityObjectStateValueConfigured;  // TODO: remove
    message_router_response->general_status = kCipErrorSuccess;
  }

  return kEipStatusOk;
}

/** @brief Checks if config session timed out
 *
 */
void EIPSecuritySessionTimeoutChecker(const MilliSeconds elapsed_time) {
  /* check if EIPSecurity configuration session timed out. */
  if(kEIPSecurityObjectStateValueConfigurationInProgress ==
     g_eip_security.state) {
    if(elapsed_time > eipSecurityConfigSessionTimeout) {
      g_eip_security.state =
        kEIPSecurityObjectStateValueFactoryDefaultConfiguration;
      OPENER_TRACE_INFO(
        "EIPSecuritySessionTimeoutChecker: EIP Security configuration session timed out\n");
      eipSecurityConfigSessionTimeout =
        ETHERNET_IP_SECURITY_CONFIG_SESSION_DEFAULT_TIMEOUT;
    }
    else {
      eipSecurityConfigSessionTimeout -= elapsed_time;
    }
    OPENER_TRACE_INFO("EIPSecurityConfigSession: Time left %" PRIu64 "\n",
                      eipSecurityConfigSessionTimeout);
  }
}

void EncodeEIPSecurityObjectCipherSuiteId(const void *const data,
                                          ENIPMessage *const outgoing_message) {
  EIPSecurityObjectCipherSuiteId *cipher_suite_id =
    (EIPSecurityObjectCipherSuiteId *)data;

  EncodeCipUsint(&(cipher_suite_id->iana_first_byte), outgoing_message);
  EncodeCipUsint(&(cipher_suite_id->iana_second_byte), outgoing_message);
}

int DecodeEIPSecurityObjectCipherSuites(
  EIPSecurityObjectCipherSuites *const data,
  const CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response) {

  int number_of_decoded_bytes = -1;

  CipUsint number_of_cipher_suites =
    GetUsintFromMessage( &(message_router_request->data) );
  number_of_decoded_bytes = sizeof(number_of_cipher_suites);

  if (number_of_cipher_suites > 0) {

    memcpy( data->cipher_suite_ids, message_router_request->data,
            number_of_cipher_suites
            * sizeof(EIPSecurityObjectCipherSuiteId) );

    number_of_decoded_bytes +=
      number_of_cipher_suites * sizeof(EIPSecurityObjectCipherSuiteId);
  }

  data->number_of_cipher_suites = number_of_cipher_suites;

  message_router_response->general_status = kCipErrorSuccess;
  return number_of_decoded_bytes;
}

void EncodeEIPSecurityObjectCipherSuites(const void *const data,
                                         ENIPMessage *const outgoing_message) {
  EIPSecurityObjectCipherSuites *cipher_suites =
    (EIPSecurityObjectCipherSuites *) data;

  EncodeCipUsint(&(cipher_suites->number_of_cipher_suites), outgoing_message);

  for (int i = 0; i<cipher_suites->number_of_cipher_suites; i++) {
    EncodeEIPSecurityObjectCipherSuiteId(&(cipher_suites->cipher_suite_ids[i]),
                                         outgoing_message);
  }
}

void EncodeEIPSecurityObjectPathList(const void *const data,
                                     ENIPMessage *const outgoing_message) {
  EIPSecurityObjectPathList *path_list = (EIPSecurityObjectPathList *) data;

  EncodeCipUsint(&(path_list->number_of_paths), outgoing_message);

  for (int i = 0; i<path_list->number_of_paths; i++) {
    EncodeCipSecurityObjectPath(&(path_list->paths[i]), outgoing_message);
  }
}

int DecodeEIPSecurityObjectPathList(
  EIPSecurityObjectPathList *const path_list,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response) {
  int number_of_decoded_bytes = -1;

  path_list->number_of_paths =
    GetUsintFromMessage(&message_router_request->data);
  number_of_decoded_bytes = 1;

  if (0 != path_list->number_of_paths) {
    for (int i = 0; i < path_list->number_of_paths; i++) {
      number_of_decoded_bytes += DecodeCipSecurityObjectPath(
        &(path_list->paths[i]), message_router_request,
        message_router_response);
    }
  } else {
    message_router_response->general_status = kCipErrorSuccess;
  }

  OPENER_TRACE_INFO("Number_of_decoded bytes: %d\n", number_of_decoded_bytes);
  return number_of_decoded_bytes;
}

/** @brief When accessed via Get_Attributes_All or Get_Attribute_Single, the
 *  Size of PSK element shall be 0, and 0 bytes of PSK value shall be returned.
 *  This ensures that the PSK value cannot be read out of the device,
 *  as it is a confidential piece of information.
 */
void EncodeEIPSecurityObjectPreSharedKeys(const void *const data,
                                          ENIPMessage *const outgoing_message) {
  AddSintToMessage(0, outgoing_message);
}

int DecodeEIPSecurityObjectPreSharedKeys(
  EIPSecurityObjectPreSharedKeys *const pre_shared_keys,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response) {
  int number_of_decoded_bytes = -1;

  CipUsint number_of_psk =
    GetUsintFromMessage( &(message_router_request->data) );

  // At present, a maximum of 1 PSK may be configured
  if (number_of_psk > 1) {
    message_router_response->general_status = kCipErrorInvalidAttributeValue;
    return number_of_decoded_bytes;
  }

  if (number_of_psk == 1) {
    EIPSecurityObjectPreSharedKey *psk_structure =
      CipCalloc( number_of_psk, sizeof(EIPSecurityObjectPreSharedKey) );

    psk_structure->psk_identity_size =
      GetUsintFromMessage( &(message_router_request->data) );

    if (psk_structure->psk_identity_size <= SIZE_MAX_PSK_IDENTITY) {
      CipOctet *psk_identity =
        CipCalloc( psk_structure->psk_identity_size, sizeof(CipOctet) );

      memcpy(psk_identity,
             message_router_request->data,
             psk_structure->psk_identity_size);
      message_router_request->data += psk_structure->psk_identity_size;
      //          for (int i=0; i<psk_structure->psk_identity_size; i++) {
      //            psk_identity[i] =
      //            GetByteFromMessage(&(message_router_request->data));
      //          }

      psk_structure->psk_identity = psk_identity;
      psk_structure->psk_size =
        GetUsintFromMessage( &(message_router_request->data) );

      if (psk_structure->psk_size <= SIZE_MAX_PSK) {
        CipOctet *psk = CipCalloc( psk_structure->psk_size, sizeof(CipOctet) );

        memcpy(psk, message_router_request->data, psk_structure->psk_size);
        //            for (int i=0; i<psk_structure->psk_size; i++) {
        //              psk[i] =
        //              GetByteFromMessage(&(message_router_request->data));
        //            }

        psk_structure->psk = psk;
        // TODO: Cleanup existing PSKs
        pre_shared_keys->pre_shared_keys = psk_structure;
        message_router_response->general_status = kCipErrorSuccess;
      } else {
        if (psk_identity != NULL) {
          CipFree(psk_identity);
          psk_structure->psk_identity = NULL;
        }
        if (psk_structure != NULL) {
          CipFree(psk_structure);
        }
        message_router_response->general_status =
          kCipErrorInvalidAttributeValue;
      }
    } else {
      if (psk_structure != NULL) {
        CipFree(psk_structure);
      }
      message_router_response->general_status = kCipErrorInvalidAttributeValue;
    }
  } else {
    // TODO: Cleanup existing PSKs
    pre_shared_keys->number_of_pre_shared_keys = number_of_psk;  // 0
    pre_shared_keys->pre_shared_keys = NULL;
    message_router_response->general_status = kCipErrorSuccess;
  }

  return number_of_decoded_bytes;
}

int DecodeDTLSTimeout(
  CipUint *const data,
  const CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response) {
  CipInstance *const instance =
    GetCipInstance(GetCipClass(message_router_request->request_path.class_id),
                   message_router_request->request_path.instance_number);

  CipAttributeStruct *attribute = GetCipAttribute(instance, 1);  // attribute #1 state
  CipUsint state = *(CipUsint *)attribute->data;

  if (kEIPSecurityObjectStateValueConfigurationInProgress == state) {
    CipUint dtls_timeout =
      GetUintFromMessage( &(message_router_request->data) );

    if (0 <= dtls_timeout && 3600 >= dtls_timeout) {
      *data = dtls_timeout;
      message_router_response->general_status = kCipErrorSuccess;
      return 2;
    } else {
      message_router_response->general_status = kCipErrorInvalidAttributeValue;
    }
  } else {
    message_router_response->general_status = kCipErrorObjectStateConflict;
  }
  return -1;
}

void EIPSecurityObjectInitializeClassSettings(CipClass *class) {

  CipClass *meta_class = class->class_instance.cip_class;

  InsertAttribute( (CipInstance *) class,
                   1,
                   kCipUint,
                   EncodeCipUint,
                   NULL,
                   (void *) &class->revision,
                   kGetableSingleAndAll ); /* revision */
  InsertAttribute( (CipInstance *) class,
                   2,
                   kCipUint,
                   EncodeCipUint,
                   NULL,
                   (void *) &class->number_of_instances,
                   kGetableSingleAndAll ); /*  largest instance number */
  InsertAttribute( (CipInstance *) class,
                   3,
                   kCipUint,
                   EncodeCipUint,
                   NULL,
                   (void *) &class->number_of_instances,
                   kGetableSingle ); /* number of instances currently existing*/
  InsertAttribute( (CipInstance *) class,
                   4,
                   kCipUint,
                   EncodeCipUint,
                   NULL,
                   (void *) &kCipUintZero,
                   kNotSetOrGetable ); /* optional attribute list - default = 0 */
  InsertAttribute( (CipInstance *) class,
                   5,
                   kCipUint,
                   EncodeCipUint,
                   NULL,
                   (void *) &kCipUintZero,
                   kNotSetOrGetable ); /* optional service list - default = 0 */
  InsertAttribute( (CipInstance *) class,
                   6,
                   kCipUint,
                   EncodeCipUint,
                   NULL,
                   (void *) &meta_class->highest_attribute_number,
                   kGetableSingleAndAll ); /* max class attribute number*/
  InsertAttribute( (CipInstance *) class,
                   7,
                   kCipUint,
                   EncodeCipUint,
                   NULL,
                   (void *) &class->highest_attribute_number,
                   kGetableSingleAndAll ); /* max instance attribute number*/
  InsertAttribute( (CipInstance *) class,
                   8,
                   kCipUint,
                   EncodeCipUint,
                   NULL,
                   (void *) &eip_security_class_attr.number_of_psks_supported,
                   kGetableSingle ); /* number of PSKs supported*/
  InsertAttribute( (CipInstance *) class,
                   9,
                   kCipByte,
                   EncodeCipByte,
                   NULL,
                   (void *) &eip_security_class_attr.psk_usages_supported,
                   kGetableSingle ); /* PSK Usages supported by the device*/

  /* Add class services to the meta class */
  InsertService(meta_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"
                );
  InsertService(meta_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle"
                );
  // add Callback function pointers
  class->PreResetCallback = &EIPSecurityObjectPreResetCallback;
}


EipStatus EIPSecurityInit(void) {
  CipClass *eip_security_object_class = NULL;
  CipInstance *eip_security_object_instance;

  eip_security_object_class = CreateCipClass(
    kEIPSecurityObjectClassCode,
    9,                                /* # class attributes */
    9,                                /* # highest class attribute number */
    2,                                /* # class services */
    16,                               /* # instance attributes */
    16,                               /* # highest instance attribute number */
    8,                                /* # instance services */
    1,                                /* # instances*/
    "EtherNet/IP Security Object",
    ETHERNET_IP_SECURITY_OBJECT_REVISION,       /* # class revision */
    &EIPSecurityObjectInitializeClassSettings   /* # function pointer for
                                                   initialization */
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
                  NULL,
                  &g_eip_security.state,
                  kGetableSingleAndAll
                  );
  InsertAttribute(eip_security_object_instance,
                  2,
                  kCipWord,
                  EncodeCipDword,
                  NULL,
                  &g_eip_security.capability_flags,
                  kGetableSingleAndAll
                  );
  InsertAttribute(eip_security_object_instance,
                  3,
                  kCipAny,
                  EncodeEIPSecurityObjectCipherSuites,
                  NULL,
                  &g_eip_security.available_cipher_suites,
                  kGetableSingleAndAll
                  );
  InsertAttribute(eip_security_object_instance,
                  4,
                  kCipAny,
                  EncodeEIPSecurityObjectCipherSuites,
                  DecodeEIPSecurityObjectCipherSuites,
                  &g_eip_security.allowed_cipher_suites,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  5,
                  kCipAny,
                  EncodeEIPSecurityObjectPreSharedKeys,
                  DecodeEIPSecurityObjectPreSharedKeys,
                  &g_eip_security.pre_shared_keys,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  6,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  DecodeEIPSecurityObjectPathList,
                  &g_eip_security.active_device_certificates,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  7,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  DecodeEIPSecurityObjectPathList,
                  &g_eip_security.trusted_authorities,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  8,
                  kCipEpath,
                  EncodeCipSecurityObjectPath,
                  DecodeCipSecurityObjectPath,
                  &g_eip_security.certificate_revocation_list,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  9,
                  kCipBool,
                  EncodeCipBool,
                  DecodeCipBool,
                  &g_eip_security.verify_client_certificate,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  10,
                  kCipBool,
                  EncodeCipBool,
                  DecodeCipBool,
                  &g_eip_security.send_certificate_chain,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  11,
                  kCipBool,
                  EncodeCipBool,
                  DecodeCipBool,
                  &g_eip_security.check_expiration,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  12,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  DecodeEIPSecurityObjectPathList,
                  &g_eip_security.trusted_identities,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  13,
                  kCipBool,
                  EncodeCipBool,
                  NULL,
                  &g_eip_security.pull_model_enabled,
                  kGetableSingleAndAll
                  );
  InsertAttribute(eip_security_object_instance,
                  14,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  &g_eip_security.pull_model_status,
                  kGetableSingleAndAll
                  );
  InsertAttribute(eip_security_object_instance,
                  15,
                  kCipUint,
                  EncodeCipUint,
                  DecodeDTLSTimeout,
                  &g_eip_security.dtls_timeout,
                  kSetAndGetAble
                  );
  InsertAttribute(eip_security_object_instance,
                  16,
                  kCipUsint,
                  EncodeCipUsint,
                  DecodeCipUsint,
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
                kReset,
                &CipResetService,
                "Reset"
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
                kEIPSecurityObjectServiceCodeBeginConfig,
                &EIPSecurityObjectBeginConfig,
                "EIPSecurityObjectBeginConfig"
                );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectServiceCodeKickTimer,
                &EIPSecurityObjectKickTimer,
                "EIPSecurityObjectKickTimer"
                );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectServiceCodeApplyConfig,
                &EIPSecurityObjectApplyConfig,
                "EIPSecurityObjectApplyConfig"
                );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectServiceCodeAbortConfig,
                &EIPSecurityObjectAbortConfig,
                "EIPSecurityObjectAbortConfig"
                );

  /* Register timeout checker function in opener */
  RegisterTimeoutChecker(EIPSecuritySessionTimeoutChecker);

  return kEipStatusOk;
}
