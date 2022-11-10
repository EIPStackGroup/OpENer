/*******************************************************************************
 * Copyright (c) 2020, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_CERTIFICATEMANAGEMENT_H
#define OPENER_CERTIFICATEMANAGEMENT_H

#include "typedefs.h"
#include "ciptypes.h"

/** @brief Certificate Management object class code */
static const CipUint kCertificateManagementObjectClassCode = 0x5FU;

/** @brief Certificate management object specific service codes
 *  @see Volume 8, Chapter 5-5.7 */
typedef enum {
  kCertificateManagementObjectServiceCodeCreateCSR = 0x4B,         /**< Certificate Management object Create CSR service code */
  kCertificateManagementObjectServiceCodeVerifyCertificate = 0x4C, /**< Certificate Management object Verify Certificate service code */
} CertificateManagementObjectServiceCode;

/** @brief Certificate management object specific error codes
 *  @see Volume 8, Chapter 5-5.8.1 */
typedef enum {
  kCertificateManagementObjectErrorCodeVerificationFailed = 0xD0, /**< Verification of the requested certificate failed */
} CertificateManagementObjectErrorCode;

/* *****************************************************************************
 * Type declarations
 */
/** @brief Valid values for Certificate Management Object Capability Flags (class attribute #8)
 *
 *  The Capability Flags attribute contains an indication of whether the device supports
 *  capabilities related to certificate enrollment.
 *  @see Volume 8, Chapter 5-5.3.1
 */
typedef enum {
  kCertificateManagementObjectCapabilityFlagPushModel = 1, /**< Push model for certificate configuration */
  kCertificateManagementObjectCapabilityFlagPullModel = 2, /**< Pull model for certificate configuration */
} CertificateManagementObjectCapabilityFlag;

/** @brief Valid values for Certificate Management Object Certificate Encoding Flags (class attribute #10)
 *
 *  The Certificate Encodings Flag attribute contains an indication
 *  of what certificate encodings the device supports.
 *
 *  WARNING: This is not equal to the certificate encoding values (Volume 8, Chapter 5-5.4.5)!
 *  @see Volume 8, Chapter 5-5.3.3
 */
typedef enum {
  kCertificateManagementObjectCertificateEncodingFlagPEM = 1,   /**< device supports PEM Encoding */
  kCertificateManagementObjectCertificateEncodingFlagPKCS7 = 2, /**< device supports PKCS#7 Encoding */
} CertificateManagementObjectCertificateEncodingFlag;

/** @brief Valid values for Certificate Management Object State (attribute #2)
 *
 *  The State attribute reports the Certificate Management Object instance’s current state.
 *  @see Volume 8, Chapter 5-5.4.2
 */
typedef enum {
  kCertificateManagementObjectStateValueNonExistent = 0, /**< The instance doesn't exist */
  kCertificateManagementObjectStateValueCreated,         /**< The instance has been created, dynamically or static */
  kCertificateManagementObjectStateValueConfiguring,     /**< One of the certificates are being configured */
  kCertificateManagementObjectStateValueVerified,        /**< Both certificates, Device Certificate and CA Certificate, have successfully been verified */
  kCertificateManagementObjectStateValueInvalid,         /**< At least one of the certificates is invalid */
} CertificateManagementObjectStateValue;

/** @brief Valid values for Certificate Management Object Certificate State (attribute #3 & #4)
 *
 *  The Certificate State reports the Certificate Management Object Device or CA certificate's current state.
 *  @see Volume 8, Chapter 5-5.4.3
 */
typedef enum {
  kCertificateManagementObjectCertificateStateValueNotVerified = 0, /**< Not Verified */
  kCertificateManagementObjectCertificateStateValueVerified,        /**< Verified */
  kCertificateManagementObjectCertificateStateValueInvalid,         /**< Invalid */
} CertificateManagementObjectCertificateStateValue;

/** @brief Valid values for Certificate Management Object Certificate Encoding (attribute #5)
 *
 *  The Certificate Encoding contains a value which represents how the certificates
 *  corresponding to this Certificate Management Object Instance are encoded.
 *  @see Volume 8, Chapter 5-5.4.5
 */
typedef enum {
  kCertificateManagementObjectCertificateEncodingPEM = 0, /**< PEM Encoding */
  kCertificateManagementObjectCertificateEncodingPKCS7,   /**< PKCS#7 Encoding */
} CertificateManagementObjectCertificateEncoding;

/** @brief Type declaration for a X.509 certificate
 *
 */
typedef struct {
  CipUsint certificate_status;
  CipEpath path;
} Certificate;

/** @brief Type declaration for the Certificate Management object
 *
 *  This is the type declaration for the Certificate Management object.
 *  @see Volume 8, Chapter 5-5.4
 */
typedef struct {
  CipShortString name;             /** Attribute #1 */
  CipUsint state;                  /** Attribute #2 */
  Certificate device_certificate;  /** Attribute #3 */
  Certificate ca_certificate;      /** Attribute #4 */
  CipUsint certificate_encoding;   /** Attribute #5 */
} CertificateManagementObject;

/** @brief Certificate Management Object specific class attributes
 *  @see Volume 8, Chapter 5-5.3 */
typedef struct cmo_class_attributes {
  CipDword capability_flags;           /** Class Attribute #8 */
  CipBool certificate_list_dummy;      /** Class Attribute #9 */
  CipDword certificate_encodings_flag; /** Class Attribute #10 */
} CertificateManagementObjectClassAttributes;

/** @brief Certificate Management Object additional instance data */
typedef struct certificate_management_object_values {
  CipServiceFunction delete_instance_data;
} CertificateManagementObjectValues;

/* ********************************************************************
 * global public variables
 */

CertificateManagementObject g_certificate_management; /**< declaration of Certificate Management object instance 1 data */

/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the Certificate Management object
 *
 *  @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CertificateManagementObjectInit(void);

#endif  // OPENER_CERTIFICATEMANAGEMENT_H
