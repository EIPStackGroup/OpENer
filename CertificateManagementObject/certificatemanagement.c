/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the Certificate Management object
 * @author Markus Pešek <markus.pesek@tuwien.ac.at>
 * @author Michael Satovich <michael.satovich@tuwien.ac.at>
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
#include <string.h>

#include "cipcommon.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

#include "certificatemanagement.h"
#include "CipSecurityObject/cipsecurity.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is 1 */
#define CERTIFICATE_MANAGEMENT_OBJECT_REVISION 1
#define DEFAULT_DEVICE_CERTIFICATE_INSTANCE_NUMBER 1

/**
 * declaration of (static) Certificate Management object instance 1 data
 */
CertificateManagementObjectClassAttributes cmo_class_attr = {
    .capability_flags = kCertificateManagementObjectCapabilityFlagPushModel,
    .certificate_list_dummy = 0,
    .certificate_encodings_flag =
        kCertificateManagementObjectCertificateEncodingFlagPEM |
        kCertificateManagementObjectCertificateEncodingFlagPKCS7,
};

const char instance_1_name[] = "Default Device Certificate";
const EipUint8 instance_1_length = 26; // excluding trailing \0

const CipShortString default_name = {
    .length = instance_1_length,
    .string = (EipByte *)(&instance_1_name),
};

const Certificate default_device_certificate = {
    .certificate_status = kCertificateManagementObjectCertificateStateValueVerified
    // TODO: add path
};

const Certificate default_ca_certificate = {
    .certificate_status = kCertificateManagementObjectCertificateStateValueVerified
    // TODO: add path
};

CertificateManagementObject g_certificate_management = {
    .name = default_name,                                    /*Attribute 1*/
    .state = kCertificateManagementObjectStateValueVerified,                              /*Attribute 2*/
    .device_certificate = default_device_certificate,        /*Attribute 3*/
    .ca_certificate = default_ca_certificate,                /*Attribute 4*/
    .certificate_encoding = kCertificateManagementObjectCertificateEncodingPEM, /*Attribute 5*/
};

/** @brief Produce the data according to CIP encoding onto the message buffer.
 *
 * This function may be used in own services for sending object data back to the
 * requester (e.g., getAttributeSingle).
 *  @param certificate pointer to the certificate object to encode
 *  @param outgoing_message pointer to the message to be sent
 */
void EncodeCertificateManagementObjectCertificate(
    const Certificate *const certificate, ENIPMessage *const outgoing_message) {
  AddSintToMessage(certificate->certificate_status, outgoing_message);
  EncodeCipSecurityObjectPath(&(certificate->path), outgoing_message);
}

/** @brief Retrieve the given object instance EPATH according to
 * CIP encoding from the message buffer.
 *
 * This function may be used for writing certificate object data
 * received from the request message (e.g., setAttributeSingle).
 *  @param certificate pointer to certificate object to be written.
 *  @param message_router_request pointer to the request where the data should be taken from
 *  @param message_router_response pointer to the response where status should be set
 *  @return length of taken bytes
 *          -1 .. error
 */
int DecodeCertificateManagementObjectCertificate(
    Certificate *const certificate,
    CipMessageRouterRequest *const message_router_request,
    CipMessageRouterResponse *const message_router_response) {
  int number_of_decoded_bytes = -1;

  certificate->certificate_status = GetUsintFromMessage(&message_router_request->data);
  number_of_decoded_bytes = 1;

  // write EPATH to the file object instance
  number_of_decoded_bytes += DecodeCipSecurityObjectPath(
      &(certificate->path), message_router_request, message_router_response);

  OPENER_TRACE_INFO("Number_of_decoded bytes: %d\n", number_of_decoded_bytes);
  return number_of_decoded_bytes;
}

/** @brief Produce the data according to CIP encoding onto the message buffer.
 *
 * This function may be used in own services for sending object data back to the
 * requester (e.g., getAttributeSingle).
 *  @param data pointer to the certificate list to encode
 *  @param outgoing_message pointer to the message to be sent
 */
void EncodeCertificateManagementObjectCertificateList(
    const void *const data, ENIPMessage *const outgoing_message) {
  CipClass *class = GetCipClass(kCertificateManagementObjectClassCode);

  CipUsint number_of_instances = class->number_of_instances;
  EncodeCipUsint(&number_of_instances, outgoing_message);

  for (CipInstance *instance = class->instances; instance; instance = instance->next) {
    CipAttributeStruct *instance_name = GetCipAttribute(instance, 1);
    EncodeCipShortString((CipShortString *)instance_name->data, outgoing_message);

    CipEpath path = {
        .path_size = 2,
        .class_id = kCertificateManagementObjectClassCode,
        .instance_number = instance->instance_number,
    };
    EncodeCipUsint(&path.path_size, outgoing_message);
    EncodeEPath(&path, outgoing_message);
  }
}

/** @brief Bind attribute values to a certificate management object instance
 *
 *  @param instance Pointer to the object where attributes should be written
 *  @param name Name of the certificate management instance
 *  @param state Object instance state
 *  @param device_certificate X.509 device certificate
 *  @param ca_certificate X.509 certificate of the authority for the device certificate
 *  @param certificate_encoding Encoding method of the certificate
 */
void CertificateManagementObjectBindAttributes(CipInstance *instance,
                                               CipShortString *name,
                                               CipUsint *state,
                                               Certificate *device_certificate,
                                               Certificate *ca_certificate,
                                               CipUsint *certificate_encoding) {

    InsertAttribute(instance,
                    1,
                    kCipShortString,
                    EncodeCipShortString,
                    NULL,
                    name,
                    kGetableSingleAndAll
    );
    InsertAttribute(instance,
                    2,
                    kCipUsint,
                    EncodeCipUsint,
                    NULL,
                    state,
                    kGetableSingleAndAll
    );
    InsertAttribute(instance,
                    3,
                    kCipAny,
                    EncodeCertificateManagementObjectCertificate,
                    DecodeCertificateManagementObjectCertificate,
                    device_certificate,
                    kSetAndGetAble
    );
    InsertAttribute(instance,
                    4,
                    kCipAny,
                    EncodeCertificateManagementObjectCertificate,
                    DecodeCertificateManagementObjectCertificate,
                    ca_certificate,
                    kSetAndGetAble
    );
    InsertAttribute(instance,
                    5,
                    kCipUsint,
                    EncodeCipUsint,
                    NULL,
                    certificate_encoding,
                    kGetableSingleAndAll
    );
}

/** @brief Certificate Management Object PreCreateCallback
 *
 *  Used for common Create service before new instance is created
 *  @See Vol.8, Chapter 5-5.5.1
 */
EipStatus CertificateManagementObjectPreCreateCallback(
    CipInstance *RESTRICT const instance,
    CipMessageRouterRequest *const message_router_request,
    CipMessageRouterResponse *const message_router_response
) {

  if (message_router_request->request_data_size > 0) {
    return kEipStatusOk;
  } else {
    message_router_response->general_status = kCipErrorNotEnoughData;
    return kEipStatusError;
  }
}

/** @brief Certificate Management Object PostCreateCallback
 *
 *  Used for common Create service after new instance is created
 *  @See Vol.8, Chapter 5-5.5.1
 */
EipStatus CertificateManagementObjectPostCreateCallback(
    CipInstance *RESTRICT const new_instance,
    CipMessageRouterRequest *const message_router_request,
    CipMessageRouterResponse *const message_router_response
) {

  CipShortString *name = (CipShortString *)CipCalloc(1, sizeof(CipShortString));
  name->length = GetUsintFromMessage(&message_router_request->data);
  name->string = (CipByte *)CipCalloc(name->length, sizeof(CipByte));
  memcpy(name->string, message_router_request->data, name->length);

  CipUsint *state = (CipUsint *)CipCalloc(1, sizeof(CipUsint));
  *state = kCertificateManagementObjectStateValueCreated;

  Certificate *device_certificate = (Certificate *)CipCalloc(1, sizeof(Certificate));
  Certificate *ca_certificate = (Certificate *)CipCalloc(1, sizeof(Certificate));
  CipUsint *certificate_encoding = (CipUsint *)CipCalloc(1, sizeof(CipUsint));

  CertificateManagementObjectBindAttributes(
      new_instance,
      name, state, device_certificate, ca_certificate, certificate_encoding);

  AddIntToMessage(new_instance->instance_number, &(message_router_response->message));
  return kEipStatusOk;
}

/** @brief Certificate Management Object PreDeleteCallback
 *
 *  Used for common Delete service before instance is deleted
 *  @See Vol.8, Chapter 5-5.5.2
 */
EipStatus CertificateManagementObjectPreDeleteCallback(
    CipInstance *RESTRICT const instance,
    CipMessageRouterRequest *const message_router_request,
    CipMessageRouterResponse *const message_router_response
) {
  if (DEFAULT_DEVICE_CERTIFICATE_INSTANCE_NUMBER ==
      instance->instance_number) {  // static instance 1 should not be deleted
    message_router_response->general_status = kCipErrorInstanceNotDeletable;
    return kEipStatusError;
  }
  return kEipStatusOk;
}

/** @brief Certificate Management Object Create CSR service
 *
 *  The Create_CSR service creates a Certificate Signing Request,
 *  suitable for submission to an enrollment server or certificate authority for signing.
 *  @See Vol.8, Chapter 5-5.7.1
 */
EipStatus CertificateManagementObjectCreateCSR(
    CipInstance *RESTRICT const instance,
    CipMessageRouterRequest *const message_router_request,
    CipMessageRouterResponse *const message_router_response,
    const struct sockaddr *originator_address,
    const int encapsulation_session) {
  // TODO: implement service

  return kEipStatusOk;
}

/** @brief Certificate Management Object Verify Certificate service
 *
 *  The Verify_Certificate shall cause the object to verify the certificate
 *  indicated by the service parameter. This service will set the appropriate status value
 *  for the Certificate Status field of all certificates involved in the verification.
 *  @See Vol.8, Chapter 5-5.7.2
 */
EipStatus CertificateManagementObjectVerifyCertificate(
    CipInstance *RESTRICT const instance,
    CipMessageRouterRequest *const message_router_request,
    CipMessageRouterResponse *const message_router_response,
    const struct sockaddr *originator_address,
    const int encapsulation_session) {
  // TODO: implement service

  return kEipStatusOk;
}

void CertificateManagementObjectInitializeClassSettings(CipClass *class) {

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
                  (void *) &class->max_instance,
                  kGetableSingleAndAll); /*  largest instance number */
  InsertAttribute((CipInstance *) class,
                  3,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->number_of_instances,
                  kGetableSingleAndAll); /* number of instances currently existing*/
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
                  (void *) &cmo_class_attr.capability_flags,
                  kGetableSingleAndAll); /* Certificate Management capabilities*/
  InsertAttribute((CipInstance *) class,
                  9,
                  kCipAny,
                  EncodeCertificateManagementObjectCertificateList,
                  NULL,
                  (void *) &cmo_class_attr.certificate_list_dummy,
                  kGetableSingleAndAll); /* List of device certificates*/
  InsertAttribute((CipInstance *) class,
                  10,
                  kCipDword,
                  EncodeCipDword,
                  NULL,
                  (void *) &cmo_class_attr.certificate_encodings_flag,
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
                kCreate,
                &Create,
                "Create"
  );
  // add Callback function pointers
  class->PreCreateCallback = &CertificateManagementObjectPreCreateCallback;
  class->PostCreateCallback = &CertificateManagementObjectPostCreateCallback;
  class->PreDeleteCallback = &CertificateManagementObjectPreDeleteCallback;
}

EipStatus CertificateManagementObjectInit(void) {
  CipClass *certificate_management_object_class = NULL;
  CipInstance *certificate_management_object_instance;

  certificate_management_object_class = CreateCipClass(
      kCertificateManagementObjectClassCode,
      3,  /* # class attributes */
      10, /* # highest class attribute number */
      3,  /* # class services */
      5,  /* # instance attributes */
      5,  /* # highest instance attribute number */
      6,  /* # instance services */
      1,  /* # instances */
      "Certificate Management Object",
      CERTIFICATE_MANAGEMENT_OBJECT_REVISION,             /* # class revision */
      &CertificateManagementObjectInitializeClassSettings /* # function pointer for initialization */
  );

  if (NULL == certificate_management_object_class) {
    /* Initialization failed */
    return kEipStatusError;
  }

  certificate_management_object_instance =
      GetCipInstance(certificate_management_object_class, 1);

  /* Bind attributes to the static instance number 1 (default certificates)*/
  CertificateManagementObjectBindAttributes(
      certificate_management_object_instance,
      &g_certificate_management.name,
      &g_certificate_management.state,
      &g_certificate_management.device_certificate,
      &g_certificate_management.ca_certificate,
      &g_certificate_management.certificate_encoding);

  /* Add services to the instance */
  InsertService(certificate_management_object_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"
  );
  InsertService(certificate_management_object_class,
                kDelete,
                &Delete,
                "Delete"
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
                kCertificateManagementObjectServiceCodeCreateCSR,
                &CertificateManagementObjectCreateCSR,
                "CertificateManagementObjectCreateCSR"
  );
  InsertService(certificate_management_object_class,
                kCertificateManagementObjectServiceCodeVerifyCertificate,
                &CertificateManagementObjectVerifyCertificate,
                "CertificateManagementObjectVerifyCertificate"
  );

  return kEipStatusOk;
}
