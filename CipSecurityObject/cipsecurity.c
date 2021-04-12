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

#include "cipcommon.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is 3 */
#define CIP_SECURITY_OBJECT_REVISION 3

/* ********************************************************************
 * global public variables
 */
/**< definition of CIP Security object instance 1 data */
CipSecurityObject g_security;

/* ********************************************************************
 * public functions
 */

/** @brief CIP Security Object Reset service
 *
 * Return this CIP Security Object Instance to the
 * Factory Default Configuration State.
 * See Vol.8 Section 5-3.5.1
 */
EipStatus CipSecurityObjectReset(CipInstance *RESTRICT const instance){

/** TODO: call Reset Service of each existing EtherNet/IP Security Object */
   return kEipStatusOk;
}

/** @brief CIP Security Object Begin_Config service
 *
 * Begins a security configuration session.
 * See Vol.8 Section 5-3.7.1
 */
EipStatus CipSecurityObjectBeginConfig(CipInstance *RESTRICT const instance){

   return kEipStatusOk;
}

/** @brief CIP Security Object End_Config service
 *
 * Ends the configuration session.
 * See Vol.8 Section 5-3.7.3
 */
EipStatus CipSecurityObjectEndConfig(CipInstance *RESTRICT const instance){

   return kEipStatusOk;
}

/** @brief CIP Security Object Kick_Timer service
 *
 * Causes the object to reset the configuration session timer.
 * See Vol.8 Section 5-3.7.2
 */
EipStatus CipSecurityObjectKickTimer(CipInstance *RESTRICT const instance){

   return kEipStatusOk;
}

/** @brief CIP Security Object Object_Cleanup service
 *
 * Remove unused object instances related to security configuration
 * See Vol.8 Section 5-3.7.4
 */
EipStatus CipSecurityObjectCleanup(CipClass *RESTRICT const cip_class){

   return kEipStatusOk;
}

void CipSecurityObjectInitializeClassSettings(CipClass *class) {

  CipClass *meta_class = class->class_instance.cip_class;

  InsertAttribute((CipInstance *) class,
                  1,
                  kCipUint,
                  EncodeCipUint,
                  (void *) &class->revision,
                  kGetableSingleAndAll);  /* revision */
  InsertAttribute((CipInstance *) class,
                  2,
                  kCipUint,
                  EncodeCipUint,
                  (void *) &class->number_of_instances,
                  kGetableSingleAndAll); /*  largest instance number */
  InsertAttribute((CipInstance *) class,
                  3,
                  kCipUint,
                  EncodeCipUint,
                  (void *) &class->number_of_instances,
                  kGetableSingle); /* number of instances currently existing*/
  InsertAttribute((CipInstance *) class,
                  4,
                  kCipUint,
                  EncodeCipUint,
                  (void *) &kCipUintZero,
                  kNotSetOrGetable); /* optional attribute list - default = 0 */
  InsertAttribute((CipInstance *) class,
                  5,
                  kCipUint,
                  EncodeCipUint,
                  (void *) &kCipUintZero,
                  kNotSetOrGetable); /* optional service list - default = 0 */
  InsertAttribute((CipInstance *) class,
                  6,
                  kCipUint,
                  EncodeCipUint,
                  (void *) &meta_class->highest_attribute_number,
                  kGetableSingleAndAll); /* max class attribute number*/
  InsertAttribute((CipInstance *) class,
                  7,
                  kCipUint,
                  EncodeCipUint,
                  (void *) &class->highest_attribute_number,
                  kGetableSingleAndAll); /* max instance attribute number*/

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
                kCipSecurityObjectCleanupServiceCode,
                &CipSecurityObjectCleanup,
                "CipSecurityObjectCleanup"
  );
}

EipStatus CipSecurityInit(void) {
  CipClass *cip_security_object_class = NULL;
  CipInstance *cip_security_object_instance;

  cip_security_object_class =
      CreateCipClass(kCipSecurityObjectClassCode,
                     0, /* # class attributes */
                     7, /* # highest class attribute number */
                     3, /* # class services */
                     2, /* # instance attributes */
                     2, /* # highest instance attribute number */
                     6, /* # instance services */
                     1, /* # instances*/
                     "CIP Security Object",
                     CIP_SECURITY_OBJECT_REVISION, /* # class revision */
                     &CipSecurityObjectInitializeClassSettings /* # function pointer for initialization */
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
                  &g_security.state,
                  kGetableSingleAndAll
  );
  InsertAttribute(cip_security_object_instance,
                  2,
                  kCipWord,
                  EncodeCipWord,
                  &g_security.security_profile,
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
                kCipSecurityObjectResetServiceCode,
                &CipSecurityObjectReset,
                "CipSecurityObjectReset"
  );
  InsertService(cip_security_object_class,
                kCipSecurityObjectBeginConfigServiceCode,
                &CipSecurityObjectBeginConfig,
                "CipSecurityObjectBeginConfig"
  );
  InsertService(cip_security_object_class,
                kCipSecurityObjectKickTimerServiceCode,
                &CipSecurityObjectKickTimer,
                "CipSecurityObjectKickTimer"
  );
  InsertService(cip_security_object_class,
                kCipSecurityObjectEndConfigServiceCode,
                &CipSecurityObjectEndConfig,
                "CipSecurityObjectEndConfig"
  );

  return kEipStatusOk;
}
