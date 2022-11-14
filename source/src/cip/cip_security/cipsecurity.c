/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the CIP Security object
 * @author Markus Pešek <markus.pesek@tuwien.ac.at>
 *
 *  CIP Security object
 *  ===================
 *
 *  This module implements the CIP Security object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  1: State
 *  - Attribute  2: Security Profiles
 *  - Attribute  3: Security Profiles Configured
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributesAll
 *  - Reset
 *  - GetAttributeSingle
 *  - Begin_Config
 *  - Kick_Timer
 *  - End_Config
 *  - Object_Cleanup (class level)
 */

/* ********************************************************************
 * include files
 */
#include "string.h"
#include <stdio.h>

#include "cipsecurity.h"
#include "ethernetipsecurity.h"

#include "cipcommon.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"
#include "cipepath.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is 3 */
#define CIP_SECURITY_OBJECT_REVISION 3

#define CIP_SECURITY_CONFIG_SESSION_DEFAULT_TIMEOUT 10000U // 10 sec

/* ********************************************************************
 * global public variables
 */
/**< definition of CIP Security object instance 1 data */

CipSecurityObject g_security = {
  .state = kCipSecurityObjectStateValueFactoryDefaultConfiguration,
  .security_profiles =
    kCipSecurityObjectSecurityProfileValueEtherNetIpConfidentialityProfile,
  .security_profiles_configured =
    kCipSecurityObjectSecurityProfileValueEtherNetIpConfidentialityProfile,
};

MilliSeconds cipSecurityConfigSessionTimeout =
  CIP_SECURITY_CONFIG_SESSION_DEFAULT_TIMEOUT;

/* ********************************************************************
 * public functions
 */

/** @brief Cip Security Object PreResetCallback
 *
 *  Used for common Reset service
 *
 *  Return this CIP Security Object Instance to the
 *  Factory Default Configuration State.
 *  @See Vol.8, Chapter 5-3.5.1
 */
EipStatus CipSecurityObjectPreResetCallback(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response) {

  if (message_router_request->request_data_size > 0) {
    message_router_response->general_status = kCipErrorTooMuchData;
  } else {
    g_security.state = kCipSecurityObjectStateValueFactoryDefaultConfiguration;
    OPENER_TRACE_INFO("Reset attribute 1 (state) of instance %d\n",
                      instance->instance_number);

    /*perform a reset on each Ethernet/IP Security Object instances present*/
    CipInstance *eip_security_instance =
      GetCipClass(kEIPSecurityObjectClassCode)->instances;

    while (NULL != eip_security_instance) {
      EIPSecurityObjectPreResetCallback(eip_security_instance,
                                        message_router_request,
                                        message_router_response);
      eip_security_instance = eip_security_instance->next;
    }
    message_router_response->general_status = kCipErrorSuccess;
  }
  return kEipStatusOk;
}

/** @brief CIP Security Object Begin_Config service
 *
 *  Begins a security configuration session.
 *  @See Vol.8, Chapter 5-3.7.1
 */
EipStatus CipSecurityObjectBeginConfig(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  message_router_response->general_status = kCipErrorPrivilegeViolation;
  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service =
    (0x80 | message_router_request->service);

//  CipAttributeStruct *attribute = GetCipAttribute(instance, 1);
//  attribute #1 state CipUsint state = *(CipUsint*) attribute->data; //TODO: check
  CipUsint state = g_security.state;

  if (kCipSecurityObjectStateValueConfigurationInProgress == state) {
    message_router_response->general_status = kCipErrorObjectStateConflict;
  } else {
    if (kCipSecurityObjectStateValueCIPSecurityConfigured == state) {
      // TODO: check if command is sent over valid TLS connection, else:
      message_router_response->general_status = kCipErrorPrivilegeViolation;
    } else {
      // TODO: check if other configuration in progress

      // *(CipUsint*) attribute->data = kCipSecurityObjectStateValueConfigurationInProgress; //set state  TODO: check

      g_security.state = kCipSecurityObjectStateValueConfigurationInProgress;

      message_router_response->general_status = kCipErrorSuccess;
    }
  }

  return kEipStatusOk;
}

/** @brief CIP Security Object End_Config service
 *
 *  Ends the configuration session.
 *  @See Vol.8, Chapter 5-3.7.3
 */
EipStatus CipSecurityObjectEndConfig(
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

//  CipAttributeStruct *attribute = GetCipAttribute(instance, 1); // attribute #1 state
//  CipUsint state = *(CipUsint*) attribute->data; //TODO: check
  CipUsint state = g_security.state;

  if (kCipSecurityObjectStateValueConfigurationInProgress == state) {
    message_router_response->general_status = kCipErrorSuccess;
//    *(CipUsint *)attribute->data = kCipSecurityObjectStateValueCIPSecurityConfigured;  // set state
    g_security.state = kCipSecurityObjectStateValueCIPSecurityConfigured;
  }

  return kEipStatusOk;
}

/** @brief CIP Security Object Kick_Timer service
 *
 *  Causes the object to reset the configuration session timer.
 *  @See Vol.8, Chapter 5-3.7.2
 */
EipStatus CipSecurityObjectKickTimer(
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

  CipUsint state = g_security.state;

  if (kCipSecurityObjectStateValueConfigurationInProgress == state) {
    // reset configuration session timer
    cipSecurityConfigSessionTimeout =
      CIP_SECURITY_CONFIG_SESSION_DEFAULT_TIMEOUT;
    message_router_response->general_status = kCipErrorSuccess;
  }

  return kEipStatusOk;
}

/** @brief CIP Security Object Object_Cleanup service
 *
 *  Remove unused object instances related to security configuration
 *  @See Vol.8, Chapter 5-3.7.4
 */
EipStatus CipSecurityObjectCleanup(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response,
  const struct sockaddr *originator_address,
  const int encapsulation_session) {
  message_router_response->general_status =
    kCipSecurityObjectErrorCodeNoOrphanObjects;
  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service =
    (0x80 | message_router_request->service);

  // TODO: implement service

  return kEipStatusOk;
}

/** @brief Checks if config session timed out
 *
 */
void CipSecuritySessionTimeoutChecker(const MilliSeconds elapsed_time) {
  /* check if CIPSecurity configuration session timed out. */
  if(kCipSecurityObjectStateValueConfigurationInProgress == g_security.state) {
    if(elapsed_time > cipSecurityConfigSessionTimeout) {
      g_security.state = kCipSecurityObjectStateValueIncompleteConfiguration;
      OPENER_TRACE_INFO(
        "CipSecuritySessionTimeoutCheck: CIP Security configuration session timed out\n");
      cipSecurityConfigSessionTimeout =
        CIP_SECURITY_CONFIG_SESSION_DEFAULT_TIMEOUT;
    }
    else {
      cipSecurityConfigSessionTimeout -= elapsed_time;
    }
    OPENER_TRACE_INFO("CipSecurityConfigSession: Time left %" PRIu64 "\n",
                      cipSecurityConfigSessionTimeout);
  }
}

void EncodeCipSecurityObjectPath(const CipEpath *const epath,
                                 ENIPMessage *const outgoing_message) {
  AddSintToMessage(epath->path_size, outgoing_message);
  if (0 != epath->path_size) {
    EncodeEPath( (CipEpath *)epath, outgoing_message );
  }
}

int DecodeCipSecurityObjectPath(
  CipEpath *const epath,
  CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response) {
  const EipUint8 *message_runner = (message_router_request->data);

  /* get data from message */
  EipUint8 path_size = GetUsintFromMessage(&message_runner);
  EipUint16 class_id = 0;
  EipUint16 instance_number = 0;
  EipUint16 attribute_number = 0;

  int number_of_decoded_bytes = 0;

  while ( number_of_decoded_bytes < (path_size * 2) ) {
    switch (*message_runner) {
      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_CLASS_ID +
        LOGICAL_SEGMENT_FORMAT_EIGHT_BIT:
        message_runner++;
        class_id = GetUsintFromMessage(&message_runner);
        number_of_decoded_bytes += 2;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_CLASS_ID +
        LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT:
        message_runner += 2;
        class_id = GetUintFromMessage(&message_runner);
        number_of_decoded_bytes += 4;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_INSTANCE_ID +
        LOGICAL_SEGMENT_FORMAT_EIGHT_BIT:
        message_runner++;
        instance_number = GetUsintFromMessage(&message_runner);
        number_of_decoded_bytes += 2;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_INSTANCE_ID +
        LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT:
        message_runner += 2;
        instance_number = GetUintFromMessage(&message_runner);
        number_of_decoded_bytes += 4;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID +
        LOGICAL_SEGMENT_FORMAT_EIGHT_BIT:
        message_runner++;
        attribute_number = GetUsintFromMessage(&message_runner);
        number_of_decoded_bytes += 2;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID +
        LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT:
        message_runner += 2;
        attribute_number = GetUintFromMessage(&message_runner);
        number_of_decoded_bytes += 4;
        break;

      default:
        OPENER_TRACE_ERR("ERROR wrong path requested\n");
        message_router_response->general_status = kCipErrorPathSegmentError;
        return kEipStatusError;
    }

  }  // end while

  /* copy epath to attribute structure */
  epath->path_size = path_size;
  epath->class_id = class_id;
  epath->instance_number = instance_number;
  epath->attribute_number = attribute_number;

  OPENER_ASSERT(path_size * 2 == number_of_decoded_bytes); /* path size is in 16 bit chunks according to the specification */

  message_router_request->data = message_runner;  // update message-pointer

  message_router_response->general_status = kCipErrorSuccess;
  return number_of_decoded_bytes += 1;  // + 1 byte for path size
}

void CipSecurityObjectInitializeClassSettings(CipClass *class) {

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
                   (void *) &class->max_instance,
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
                kCIPSecurityObjectServiceCodeObjectCleanup,
                &CipSecurityObjectCleanup,
                "CipSecurityObjectCleanup"
                );
  // add Callback function pointers
  class->PreResetCallback = &CipSecurityObjectPreResetCallback;
}

EipStatus CipSecurityInit(void) {
  CipClass *cip_security_object_class = NULL;
  CipInstance *cip_security_object_instance;

  cip_security_object_class =
    CreateCipClass(kCipSecurityObjectClassCode,
                   0,   /* # class attributes */
                   7,   /* # highest class attribute number */
                   3,   /* # class services */
                   3,   /* # instance attributes */
                   3,   /* # highest instance attribute number */
                   6,   /* # instance services */
                   1,   /* # instances*/
                   "CIP Security Object",
                   CIP_SECURITY_OBJECT_REVISION,   /* # class revision */
                   &CipSecurityObjectInitializeClassSettings   /* # function pointer for initialization */
                   );

  if (NULL == cip_security_object_class) {
    /* Initialization failed */
    return kEipStatusError;
  }

  /* Bind attributes to the instance */
  cip_security_object_instance = GetCipInstance(cip_security_object_class, 1);

  InsertAttribute(cip_security_object_instance,
                  1,
                  kCipUsint,
                  EncodeCipUsint,
                  NULL,
                  &g_security.state,
                  kGetableSingleAndAll
                  );
  InsertAttribute(cip_security_object_instance,
                  2,
                  kCipWord,
                  EncodeCipWord,
                  NULL,
                  &g_security.security_profiles,
                  kGetableSingleAndAll
                  );
  InsertAttribute(cip_security_object_instance,
                  3,
                  kCipWord,
                  EncodeCipWord,
                  NULL,
                  &g_security.security_profiles_configured,
                  kGetableSingleAndAll
                  );

  /* Add services to the instance */
  InsertService(cip_security_object_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle"
                );
  InsertService(cip_security_object_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"
                );
  InsertService(cip_security_object_class,
                kReset,
                &CipResetService,
                "Reset"
                );
  InsertService(cip_security_object_class,
                kCIPSecurityObjectServiceCodeBeginConfig,
                &CipSecurityObjectBeginConfig,
                "CipSecurityObjectBeginConfig"
                );
  InsertService(cip_security_object_class,
                kCIPSecurityObjectServiceCodeKickTimer,
                &CipSecurityObjectKickTimer,
                "CipSecurityObjectKickTimer"
                );
  InsertService(cip_security_object_class,
                kCIPSecurityObjectServiceCodeEndConfig,
                &CipSecurityObjectEndConfig,
                "CipSecurityObjectEndConfig"
                );

  /* Register timeout checker function in opener */
  RegisterTimeoutChecker(CipSecuritySessionTimeoutChecker);

  return kEipStatusOk;
}
