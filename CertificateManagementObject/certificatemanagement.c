/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the Certificate Management object
 * @author Markus Pešek <markus.pesek@tuwien.ac.at>
 *
 *  Certificate Management object
 *  =============================
 *
 *  This module implements the Certificate Management object.
 *
 *  Implemented Class Attributes
 *  ----------------------------
 *  - Attribute  8: Capability Flags
 *  - Attribute  9: Certificate List
 *  - Attribute 10: Certificate Encoding Flag
 *
 *  Implemented Instance Attributes
 *  -------------------------------
 *  - Attribute  1: Name
 *  - Attribute  2: State
 *  - Attribute  3: Device Certificate
 *  - Attribute  4: CA Certificate
 *  - Attribute  5: Certificate Encoding
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributesAll
 *  - Create (class level)
 *  - Delete
 *  - GetAttributeSingle
 *  - SetAttributeSingle
 *  - Create_CSR
 *  - Verify_Certificate
 */

/* ********************************************************************
 * include files
 */
#include "string.h"
#include <stdio.h>

#include "cipsecurity.h"

#include "cipcommon.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

#include "certificatemanagement.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is 1 */
#define CERTIFICATE_MANAGEMENT_OBJECT_REVISION 1


/** @brief Certificate Management Object Create service
 *
 * The Create service shall be used to create a dynamic instance.
 * See Vol.8 Section 5-5.5.1
 */
EipStatus CertificateManagementObjectCreate(CipInstance *RESTRICT const instance,
                                 CipMessageRouterRequest *const message_router_request,
                                 CipMessageRouterResponse *const message_router_response,
                                 const struct sockaddr *originator_address,
                                 const int encapsulation_session) {
	return kEipStatusOk;
}

/** @brief Certificate Management Object Delete service
 *
 * The Delete service shall be used to delete dynamic instances
 * (static instances shall return status code 0x2D, Instance Not Deletable).
 * See Vol.8 Section 5-5.5.2
 */
EipStatus CertificateManagementObjectDelete(CipInstance *RESTRICT const instance,
                                            CipMessageRouterRequest *const message_router_request,
                                            CipMessageRouterResponse *const message_router_response,
                                            const struct sockaddr *originator_address,
                                            const int encapsulation_session) {
	return kEipStatusOk;
}

/** @brief Certificate Management Object Create CSR service
 *
 * The Create_CSR service creates a Certificate Signing Request,
 * suitable for submission to an enrollment server or certificate authority for signing.
 * See Vol.8 Section 5-5.7.1
 */
EipStatus CertificateManagementObjectCreateCSR(CipInstance *RESTRICT const instance,
                                            CipMessageRouterRequest *const message_router_request,
                                            CipMessageRouterResponse *const message_router_response,
                                            const struct sockaddr *originator_address,
                                            const int encapsulation_session) {
	return kEipStatusOk;
}

/** @brief Certificate Management Object Verify Certificate service
 *
 * The Verify_Certificate shall cause the object to verify the certificate
 * indicated by the service parameter. This service will set the appropriate status value
 * for the Certificate Status field of all certificates involved in the verification.
 * See Vol.8 Section 5-5.7.2
 */
EipStatus CertificateManagementObjectVerifyCertificate(CipInstance *RESTRICT const instance,
                                            CipMessageRouterRequest *const message_router_request,
                                            CipMessageRouterResponse *const message_router_response,
                                            const struct sockaddr *originator_address,
                                            const int encapsulation_session) {
	return kEipStatusOk;
}

void EncodeCertificateManagementObjectCertificate(const void *const data,
                                          ENIPMessage *const outgoing_message) {

}

void DecodeCertificateManagementObjectCertificate(
    Certificate *const data,
    const CipMessageRouterRequest *const message_router_request,
    CipMessageRouterResponse *const message_router_response) {

}

void EncodeCertificateManagementObjectCertificateList(const void *const data,
                                                      ENIPMessage *const outgoing_message) {

}

void CertificateManagementObjectInitializeClassSettings(CertificateManagementObjectClass *class) {

  CipClass *meta_class = class->class_instance.cip_class;

  InsertAttribute((CipInstance *) class,
                  1,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->revision,
                  kGetableSingleAndAll);  /* revision */
  InsertAttribute((CipInstance *) class,
                  2,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->number_of_instances,
                  kGetableSingleAndAll); /*  largest instance number */
  InsertAttribute((CipInstance *) class,
                  3,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->number_of_instances,
                  kGetableSingle); /* number of instances currently existing*/
  InsertAttribute((CipInstance *) class,
                  4,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &kCipUintZero,
                  kNotSetOrGetable); /* optional attribute list - default = 0 */
  InsertAttribute((CipInstance *) class,
                  5,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &kCipUintZero,
                  kNotSetOrGetable); /* optional service list - default = 0 */
  InsertAttribute((CipInstance *) class,
                  6,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &meta_class->highest_attribute_number,
                  kGetableSingleAndAll); /* max class attribute number*/
  InsertAttribute((CipInstance *) class,
                  7,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->highest_attribute_number,
                  kGetableSingleAndAll); /* max instance attribute number*/
  InsertAttribute((CipInstance *) class,
                  8,
                  kCipDword,
                  EncodeCipDword,
                  NULL,
                  (void *) &class->capability_flags,
                  kGetableSingleAndAll); /* Certificate Management capabilities*/
  InsertAttribute((CipInstance *) class,
                  9,
                  kCipAny,
                  EncodeCertificateManagementObjectCertificateList,
                  NULL,
                  (void *) &class->certificate_list,
                  kGetableSingleAndAll); /* List of device certificates*/
  InsertAttribute((CipInstance *) class,
                  10,
                  kCipDword,
                  EncodeCipDword,
                  NULL,
                  (void *) &class->certificate_encodings_flag,
                  kGetableSingleAndAll); /* Certificate encodings supported*/

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
  InsertService(meta_class,
                kCertificateManagementObjectCreateServiceCode,
                &CertificateManagementObjectCreate,
                "CertificateManagementObjectCreate"
  );
}

EipStatus CertificateManagementObjectInit(void) {
  CipClass *certificate_management_object_class = NULL;
  CipInstance *certificate_management_object_instance;

  certificate_management_object_class =
      CreateCipClass(kCertificateManagementObjectClassCode,
                     3, /* # class attributes */
                     10,/* # highest class attribute number */
                     3, /* # class services */
                     5, /* # instance attributes */
                     5, /* # highest instance attribute number */
                     6, /* # instance services */
                     1, /* # instances*/
                     "Certificate Management Object",
                     CERTIFICATE_MANAGEMENT_OBJECT_REVISION, /* # class revision */
                     &CertificateManagementObjectInitializeClassSettings /* # function pointer for initialization */
      );

  if (NULL == certificate_management_object_class) {
    /* Initialization failed */
    return kEipStatusError;
  }

  /* Bind attributes to the instance created above */
  certificate_management_object_instance = GetCipInstance(certificate_management_object_class, 1);

  InsertAttribute(certificate_management_object_instance,
                  1,
                  kCipShortString,
                  EncodeCipShortString,
                  NULL,
                  &g_certificate_management.name,
                  kGetableSingleAndAll
  );
  InsertAttribute(certificate_management_object_instance,
                  2,
                  kCipUsint,
                  EncodeCipUsint,
                  NULL,
                  &g_certificate_management.state,
                  kGetableSingleAndAll
  );
  InsertAttribute(certificate_management_object_instance,
                  3,
                  kCipAny,
                  EncodeCertificateManagementObjectCertificate,
                  DecodeCertificateManagementObjectCertificate,
                  &g_certificate_management.device_certificate,
                  kSetAndGetAble
  );
  InsertAttribute(certificate_management_object_instance,
                  4,
                  kCipAny,
                  EncodeCertificateManagementObjectCertificate,
                  DecodeCertificateManagementObjectCertificate,
                  &g_certificate_management.ca_certificate,
                  kSetAndGetAble
  );
  InsertAttribute(certificate_management_object_instance,
                  5,
                  kCipUsint,
                  EncodeCipUsint,
                  NULL,
                  &g_certificate_management.certificate_encoding,
                  kGetableSingleAndAll
  );

  /* Add services to the instance */
  InsertService(certificate_management_object_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"
  );
  InsertService(certificate_management_object_class,
                kCertificateManagementObjectDeleteServiceCode,
                &CertificateManagementObjectDelete,
                "CertificateManagementObjectDelete"
  );
  InsertService(certificate_management_object_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle"
  );
  InsertService(certificate_management_object_class,
                kSetAttributeSingle,
                &SetAttributeSingle,
                "SetAttributeSingle"
  );
  InsertService(certificate_management_object_class,
                kCertificateManagementObjectCreateCSRServiceCode,
                &CertificateManagementObjectCreateCSR,
                "CertificateManagementObjectCreateCSR"
  );
  InsertService(certificate_management_object_class,
                kCertificateManagementObjectVerifyCertificateServiceCode,
                &CertificateManagementObjectVerifyCertificate,
                "CertificateManagementObjectVerifyCertificate"
  );

  return kEipStatusOk;
}
