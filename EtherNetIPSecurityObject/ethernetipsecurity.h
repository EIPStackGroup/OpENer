/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_ETHERNETIPSECURITY_H
#define OPENER_ETHERNETIPSECURITY_H

#include "ciptypes.h"
#include "typedefs.h"

/** @brief EtherNet/IP Security object class code */
static const CipUint kEIPSecurityObjectClassCode = 0x5EU;

/** @brief Maximum length of PSK Identity in octets
 * see Volume 8, Chapter 5-4.4.5
 */
static const CipUsint SIZE_MAX_PSK_IDENTITY = 128;

/** @brief Maximum length of PSK in octets. See Volume 8, Chapter 5-4.4.5 */
static const CipUsint SIZE_MAX_PSK = 64;

/* *****************************************************************************
 * Type declarations
 */
/** @brief EtherNet/IP Security object specific service codes
 * see Volume 8, Chapter 5-4.7 */
typedef enum {
  /** @brief EtherNet/IP Security object Begin_Config service code */
  kBeginConfig = 0x4B,
  /** @brief EtherNet/IP Security object Kick_Timer service code */
  kKickTimer = 0x4C,
  /** @brief EtherNet/IP Security object Apply_Config service code */
  kApplyConfig = 0x4D,
  /** @brief EtherNet/IP Security object Abort_Config service code */
  kAbortConfig = 0x4E,
} EIPSecurityObjectServiceCode;

/** @brief Valid values for EtherNet/IP Security Object State (attribute #1)
 *  The State attribute reports the EtherNet/IP Security Object’s current state.
 *  see Volume 8, Chapter 5-4.4.1
 */
typedef enum {
  /** Factory Default Configuration */
  kEIPFactoryDefaultConfiguration = 0,
  /** Configuration In Progress */
  kEIPConfigurationInProgress,
  /** Configured */
  kEIPConfigured,
  /** Pull Model In Progress */
  kPullModelInProgress,
  /** Pull Model Completed */
  kPullModelCompleted,
  /** Pull Model Disabled */
  kPullModelDisabled,
} EIPSecurityObjectStateValue;

/** @brief Provide bit masks for the Capability Flags attribute (#2)
 *  The Capability Flags attribute contains an indication of whether the device
 *  supports optional capabilities related to TLS and DTLS.
 *  see Volume 8, Chapter 5-4.4.2
 */
typedef enum {
  /** Indicates whether or not the device supports secure renegotiation via the
      Renegotiation Indication Extension defined in RFC 5746. */
  kSecureRenegotiation = 0x01,
} EIPSecurityObjectCapabilityFlag;

/** @brief Extended status error codes for Apply_Config service
 *  see Volume 8, Chapter 5-4.8.1
 */
typedef enum {
  /**< Only PSK cipher suited allowed but no Pre-Shared Keys configured */
  kNoPSKConfigured = 0x0001,
  /**< Only certificate suites allowed, but no certificates configured */
  kNoCertificatesConfigured = 0x0002,
  /**< Certificate suites allowed but none consistent with the device private key type */
  kCertificateSuitesNoneConsistent = 0x0003,
  /**< No Active Cipher Suites configured */
  kNoActiveCipherSuitesConfigured = 0x0004
} EIPSecurityObjectApplyConfigErrorCodeExtended;

/** @brief Type declaration for a single IANA Cipher Suite ID
 *  see Volume 8, Chapter 5-4.4.3
 */
typedef struct {
  CipUsint iana_first_byte;
  CipUsint iana_second_byte;
} EIPSecurityObjectCipherSuiteId;

/** @brief Type declaration for a list of Cipher Suites
 *
 */
typedef struct {
  CipUsint number_of_cipher_suites;
  EIPSecurityObjectCipherSuiteId *cipher_suite_ids;
} EIPSecurityObjectCipherSuites;

/** @brief Type declaration for a pre-shared key
 *  see Volume 8, Chapter 5-4.4.5
 */
typedef struct {
  CipUsint psk_identity_size;
  CipOctet *psk_identity;
  CipUsint psk_size;
  CipOctet *psk;
} EIPSecurityObjectPreSharedKey;

/** @brief Type declaration for pre-shared keys to use with PSK cipher suites
 *  see Volume 8, Chapter 5-4.4.5
 */
typedef struct {
  CipUsint number_of_pre_shared_keys;
  EIPSecurityObjectPreSharedKey *pre_shared_keys;
} EIPSecurityObjectPreSharedKeys;

/** @brief Type declaration for a list of paths to
 *  Certificate Management Object instances or a File Object instances
 */
typedef struct {
  CipUsint number_of_paths;
  CipEpath *paths;
} EIPSecurityObjectPathList;

/** @brief Type declaration for the EtherNet/IP Security object
 *
 *  This is the type declaration for the EtherNet/IP Security object.
 */
typedef struct {
  CipUsint state;                                        /** Attribute #1 */
  CipDword capability_flags;                             /** Attribute #2 */
  EIPSecurityObjectCipherSuites available_cipher_suites; /** Attribute #3 */
  EIPSecurityObjectCipherSuites allowed_cipher_suites;   /** Attribute #4 */
  EIPSecurityObjectPreSharedKeys pre_shared_keys;        /** Attribute #5 */
  EIPSecurityObjectPathList active_device_certificates;  /** Attribute #6 */
  EIPSecurityObjectPathList trusted_authorities;         /** Attribute #7 */
  CipEpath certificate_revocation_list;                  /** Attribute #8 */
  CipBool verify_client_certificate;                     /** Attribute #9 */
  CipBool send_certificate_chain;                        /** Attribute #10 */
  CipBool check_expiration;                              /** Attribute #11 */
  EIPSecurityObjectPathList trusted_identities;          /** Attribute #12 */
  CipBool pull_model_enabled;                            /** Attribute #13 */
  CipUint pull_model_status;                             /** Attribute #14 */
  CipUint dtls_timeout;                                  /** Attribute #15 */
  CipUsint udp_only_policy;                              /** Attribute #16 */
} EIPSecurityObject;

/* ********************************************************************
 * global public variables
 */
/**
 * declaration of EtherNet/IP Security object instance 1 data
 */
extern EIPSecurityObject g_eip_security;

/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the EtherNet/IP Security object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus EIPSecurityInit(void);

/** @brief EtherNet/IP Security Object Reset settable attributes
 *
 * Return all settable instance attributes to the
 * Factory Default Configuration value
 */
void EIPSecurityObjectResetSettableAttributes(CipInstance *instance);

#endif  // OPENER_ETHERNETIPSECURITY_H
