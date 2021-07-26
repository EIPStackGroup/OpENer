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

/** @brief Certificate Management object Create service code */
static const CipUint kCertificateManagementObjectCreateServiceCode = 0x08U;
/** @brief Certificate Management object Delete service code */
static const CipUint kCertificateManagementObjectDeleteServiceCode = 0x09U;
/** @brief Certificate Management object Create CSR service code */
static const CipUint kCertificateManagementObjectCreateCSRServiceCode = 0x4BU;
/** @brief Certificate Management object Verify Certificate service code */
static const CipUint kCertificateManagementObjectVerifyCertificateServiceCode = 0x4CU;

/* *****************************************************************************
 * Type declarations
 */
/** @brief Valid values for Certificate Management Object Capability Flags (class attribute #8)
 *  The Capability Flags attribute contains an indication of whether the device supports
 *  capabilities related to certificate enrollment.
 *  see Volume 8, Chapter 5-5.3.1
 */
typedef enum certificate_management_object_capability_flags {
  /** Push */
  kCertificateManagementObjectCapabilityFlagsPushModel = 1,
  /** Pull */
  kCertificateManagementObjectCapabilityFlagsPullModel = 2,
} CertificateManagementObjectCapabilityFlags;

/** @brief Valid values for Certificate Management Object Certificate Encoding Flags (class attribute #10)
 *  The Certificate Encodings Flag attribute contains an indication
 *  of what certificate encodings the device supports.
 *  see Volume 8, Chapter 5-5.3.3
 */
typedef enum certificate_management_object_certificate_encoding_flags {
  /** PEM Encoding */
  kCertificateManagementObjectCertificateEncodingFlagsPEM = 1,
  /** PKCS#7 Encoding */
  kCertificateManagementObjectCertificateEncodingFlagsPKCS7 = 2,
} CertificateManagementObjectCertificateEncodingFlags;

/** @brief Valid values for Certificate Management Object State (attribute #2)
 *  The State attribute reports the Certificate Management Object instance’s current state.
 *  see Volume 8, Chapter 5-5.4.2
 */
typedef enum certificate_management_object_state_values {
  /** Non existent */
  kCertificateManagementObjectStateValueNonExistent = 0,
  /** Created */
  kCertificateManagementObjectStateValueCreated,
  /** Configuring */
  kCertificateManagementObjectStateValueConfiguring,
  /** Verified */
  kCertificateManagementObjectStateValueVerified,
  /** Invalid */
  kCertificateManagementObjectStateValueInvalid,
} CertificateManagementObjectStateValues;

/** @brief Valid values for Certificate Management Object Certificate State (attribute #3 & #4)
 *  The Certificate State reports the Certificate Management Object Device or CA certificate's current state.
 *  see Volume 8, Chapter 5-5.4.3
 */
typedef enum certificate_management_object_certificate_state_values {
  /** Not Verified */
  kCertificateManagementObjectCertificateStateValueNotVerified = 0,
  /** Verified */
  kCertificateManagementObjectCertificateStateValueVerified,
  /** Invalid */
  kCertificateManagementObjectCertificateStateValueInvalid,
} CertificateManagementObjectCertificateStateValues;

/** @brief Valid values for Certificate Management Object Certificate Encoding (attribute #5)
 *  The Certificate Encoding contains a value which represents how the certificates
 *  corresponding to this Certificate Management Object Instance are encoded.
 *  see Volume 8, Chapter 5-5.4.5
 */
typedef enum certificate_management_object_certificate_encoding {
  /** PEM Encoding */
  kCertificateManagementObjectCertificateEncodingPEM = 0,
  /** PKCS#7 Encoding */
  kCertificateManagementObjectCertificateEncodingPKCS7,
} CertificateManagementObjectCertificateEncoding;

/** @brief Type declaration for a X.509 certificate
 *
 */
typedef struct {
  CipUsint certificate_status;
  CipEpath path;
} Certificate;

/** @brief Type declaration for a certificate list
 * The Certificate List attribute contains a list of created certificate instances
 * see Volume 8, Chapter 5-5.3.2
 */
typedef struct {
  CipUsint number_of_certificates;
  CipShortString certificate_name;
  Certificate *certificate_list;
} CertificateList;


/** @brief Type declaration for the Certificate Management object
 *
 * This is the type declaration for the Certificate Management object.
 */
typedef struct {
  CipShortString name;             /** Attribute #1 */
  CipUsint state;                  /** Attribute #2 */
  Certificate device_certificate;  /** Attribute #3 */
  Certificate ca_certificate;      /** Attribute #4 */
  CipUsint certificate_encoding;   /** Attribute #5 */
} CertificateManagementObject;

/** @brief Type definition of CipClass that is a subclass of CipInstance */
typedef struct cmo_class {
  CipInstance class_instance;   /**< This is the instance that contains the
                                   class attributes of this class. */
  /* the rest of these are specific to the Class class only. */
  CipUdint class_code;   /**< class code */
  EipUint16 revision;   /**< class revision*/
  EipUint16 number_of_instances;   /**< number of instances in the class (not
                                      including instance 0) */
  EipUint16 number_of_attributes;   /**< number of attributes of each instance */
  EipUint16 highest_attribute_number;   /**< highest defined attribute number
                                           (attribute numbers are not necessarily
                                           consecutive) */
  uint8_t *get_single_bit_mask;   /**< bit mask for GetAttributeSingle */
  uint8_t *set_bit_mask;   /**< bit mask for SetAttributeSingle */
  uint8_t *get_all_bit_mask;   /**< bit mask for GetAttributeAll */

  EipUint16 number_of_services;   /**< number of services supported */
  CipInstance *instances;   /**< pointer to the list of instances */
  struct cip_service_struct *services;   /**< pointer to the array of services */
  char *class_name;   /**< class name */
  /** Is called in GetAttributeSingle* before the response is assembled from
   * the object's attributes */
  CipGetSetCallback PreGetCallback;
  /** Is called in GetAttributeSingle* after the response has been sent. */
  CipGetSetCallback PostGetCallback;
  /** Is called in SetAttributeSingle* before the received data is moved
   * to the object's attributes */
  CipGetSetCallback PreSetCallback;
  /** Is called in SetAttributeSingle* after the received data was set
   * in the object's attributes. */
  CipGetSetCallback PostSetCallback;

  /**Class specific attributes*/

  /**The Capability Flags attribute contains an indication of whether the
   * device supports capabilities related to certificate enrollment*/
  CipDword capability_flags;
  /**The Certificate List attribute contains a list of created certificate instances */
  CipDword certificate_list;
  /**The Certificate Encodings Flag attribute contains an indication of what
   * certificate encodings the device supports*/
  CipDword certificate_encodings_flag;
} CertificateManagementObjectClass;

/* ********************************************************************
 * global public variables
 */
/**
 * declaration of Certificate Management object instance 1 data
 */
CertificateManagementObject g_certificate_management;

/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the Certificate Management object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CertificateManagementObjectInit(void);

#endif  // OPENER_CERTIFICATEMANAGEMENT_H
