/*******************************************************************************
 * Copyright (c) 2020, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_CIPSECURITY_H
#define OPENER_CIPSECURITY_H

#include "typedefs.h"
#include "ciptypes.h"

/** @brief CIP Security object class code */
static const CipUint kCipSecurityObjectClassCode = 0x5DU;

/* *****************************************************************************
 * Type declarations
 */

/** @brief CIP Security object specific service codes
 *  @see Volume 8, Chapter 5-3.7 */
typedef enum {
  kCIPSecurityObjectServiceCodeBeginConfig = 0x4B,   /**< CIP Security object Begin_Config service code */
  kCIPSecurityObjectServiceCodeKickTimer = 0x4C,     /**< CIP Security object Kick_Timer service code */
  kCIPSecurityObjectServiceCodeEndConfig = 0x4D,     /**< CIP Security object End_Config service code */
  kCIPSecurityObjectServiceCodeObjectCleanup = 0x4E, /**< CIP Security object Object_Cleanup service code */
} CIPSecurityObjectServiceCode;

/** @brief CIP Security object specific error codes
 *  @see Volume 8, Chapter 5-3.8 */
typedef enum {
  kCipSecurityObjectErrorCodeNoOrphanObjects = 0xD0, /**< No orphan objects for cleanup */
} CipSecurityObjectErrorCode;

/** @brief Valid values for CIP Security Object State (attribute #1)
 *
 *  The State attribute reports the CIP Security Object’s current state.
 *  @see Volume 8, Chapter 5-3.4.1
 */
typedef enum {
  kCipSecurityObjectStateValueFactoryDefaultConfiguration = 0, /**< Factory Default Configuration */
  kCipSecurityObjectStateValueConfigurationInProgress,         /**< ConfigurationIn Progress */
  kCipSecurityObjectStateValueCIPSecurityConfigured,           /**< Configured */
  kCipSecurityObjectStateValueIncompleteConfiguration,         /**< Incomplete Configuration */
} CipSecurityObjectStateValue;

/** @brief Valid values for CIP Security Object Security Profiles (attribute #2)
 *
 *  The Security Profiles attribute represents the Security Profiles
 *  that the device implements.
 *  @see Volume 8, Chapter 5-3.4.2
 */
typedef enum {
  kCipSecurityObjectSecurityProfileValueEtherNetIpIntegrityProfile = 0x01U,       /**< The device supports the EtherNet/IP Integrity Profile (Obsoleted) */
  kCipSecurityObjectSecurityProfileValueEtherNetIpConfidentialityProfile = 0x02U, /**< The device supports the EtherNet/IP Confidentiality Profile */
  kCipSecurityObjectSecurityProfileValueCipAuthorizationProfile = 0x04U,          /**< The device supports the CIP Authorization Profile */
  kCipSecurityObjectSecurityProfileValueCipUserAuthenticationProfile = 0x08U,     /**< The device supports the CIP User Authentication Profile */
} CipSecurityObjectSecurityProfileValue;

/** @brief Type declaration for the CIP Security object
 *
 *  This is the type declaration for the CIP Security object.
 *  @see Volume 8, Chapter 5-3.4
 */
typedef struct {
  CipUsint state;                       /** Attribute #1 */
  CipWord security_profiles;            /** Attribute #2 */
  CipWord security_profiles_configured; /** Attribute #3 */
} CipSecurityObject;


/* ********************************************************************
 * global public variables
 */
extern CipSecurityObject g_security;  /**< declaration of CIP Security object instance 1 data */

/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the CIP Security object
 *
 *  @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipSecurityInit(void);

/** @brief Produce the data according to CIP encoding onto the message buffer.
 *
 *  This function may be used in own services for sending object path data back to the
 *  requester (e.g., getAttributeSingle).
 *  @param epath pointer to the object EPATH to encode
 *  @param outgoing_message pointer to the message to be sent
 */
void EncodeCipSecurityObjectPath(const CipEpath *const epath,
		ENIPMessage *const outgoing_message);

/** @brief Retrieve the given object instance EPATH according to
 *  CIP encoding from the message buffer.
 *
 *  This function may be used for writing various object paths
 *  from the request message (e.g., setAttributeSingle).
 *  @param epath pointer to EPATH of the object instance to be written
 *  @param message_router_request pointer to the request where the data should be taken from
 *  @param message_router_response pointer to the response where status should be set
 *  @return length of taken bytes\n
 *          -1 ... error
 */
int DecodeCipSecurityObjectPath(CipEpath *const epath,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response);

#endif  // OPENER_CIPSECURITY_H
