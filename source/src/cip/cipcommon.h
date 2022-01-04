/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPCOMMON_H_
#define OPENER_CIPCOMMON_H_

/** @file cipcommon.h
 * Common CIP object interface
 */

#include "typedefs.h"
#include "ciptypes.h"

static const EipUint16 kCipUintZero = 0; /**< Zero value for returning the UINT standard value */

/** @brief Check if requested service present in class/instance and call appropriate service.
 *
 * @param cip_class class receiving the message
 * @param message_router_request request message
 * @param message_router_response reply message
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return
 *     - kEipStatusOkSend ... success
 *     - kEipStatusOk ... no reply to send back
 *     - kEipStatusError ... error
 */
EipStatus NotifyClass(const CipClass *const RESTRICT cip_class,
                      CipMessageRouterRequest *const message_router_request,
                      CipMessageRouterResponse *const message_router_response,
                      const struct sockaddr *originator_address,
                      const CipSessionHandle encapsulation_session);

/** @brief Get largest instance_number present in class instances
 *
 * @param cip_class class to be considered
 * @return largest instance_number in class instances
 */
CipUint GetMaxInstanceNumber(CipClass *RESTRICT const cip_class);                      

void GenerateGetAttributeSingleHeader(
  const CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response);

/** @brief Generic implementation of the GetAttributeSingle CIP service
 *
 * Check from classID which Object requests an attribute, search if object has
 * the appropriate attribute implemented.
 * @param instance pointer to instance.
 * @param message_router_request pointer to request.
 * @param message_router_response pointer to response.
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return status  >0 .. success
 *          -1 .. requested attribute not available
 */
EipStatus GetAttributeSingle(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const
  message_router_request,
  CipMessageRouterResponse *const
  message_router_response,
  const struct sockaddr *originator_address,
  const CipSessionHandle encapsulation_session);

void GenerateSetAttributeSingleHeader(
  const CipMessageRouterRequest *const message_router_request,
  CipMessageRouterResponse *const message_router_response);

/** @brief Generic implementation of the SetAttributeSingle CIP service
 *
 *  Modifies an attribute value if the requested object has
 *  the appropriate attribute implemented and if the attribute is settable.
 *
 * @param instance pointer to instance.
 * @param message_router_request pointer to request.
 * @param message_router_response pointer to response.
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return status  >0 .. success
 *          -1 .. requested attribute not set
 */
EipStatus SetAttributeSingle(
  CipInstance *RESTRICT const instance,
  CipMessageRouterRequest *const
  message_router_request,
  CipMessageRouterResponse *const
  message_router_response,
  const struct sockaddr *originator_address,
  const CipSessionHandle encapsulation_session);

/** @brief Generic implementation of the GetAttributeAll CIP service
 *
 * Copy all attributes from Object into the global message buffer.
 * @param instance pointer to object instance with data.
 * @param message_router_request pointer to MR request.
 * @param message_router_response pointer for MR response.
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return length of data stream >0 .. success
 *              0 .. no reply to send
 */
EipStatus GetAttributeAll(CipInstance *instance,
                          CipMessageRouterRequest *message_router_request,
                          CipMessageRouterResponse *message_router_response,
                          const struct sockaddr *originator_address,
                          const CipSessionHandle encapsulation_session);

/** @brief Generic implementation of the GetAttributeList CIP service
 *
 * Copy the contents of the selected gettable attributes of the specified
 * object class or instance into the global message buffer.
 * @param instance pointer to object instance with data.
 * @param message_router_request pointer to MR request.
 * @param message_router_response pointer for MR response.
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return length of data stream >0 .. success
 *              0 .. no reply to send
 */
EipStatus GetAttributeList(CipInstance *instance,
                          CipMessageRouterRequest *message_router_request,
                          CipMessageRouterResponse *message_router_response,
                          const struct sockaddr *originator_address,
                          const CipSessionHandle encapsulation_session);

/** @brief Generic implementation of the SetAttributeList CIP service
 *
 * Sets the values of selected attributes of the specified object class
 * or instance.
 * @param instance pointer to object instance with data.
 * @param message_router_request pointer to MR request.
 * @param message_router_response pointer to MR response.
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return length of data stream >0 .. success
 *              0 .. no reply to send
 */
EipStatus SetAttributeList(CipInstance *instance,
                          CipMessageRouterRequest *message_router_request,
                          CipMessageRouterResponse *message_router_response,
                          const struct sockaddr *originator_address,
                          const CipSessionHandle encapsulation_session);

/** @brief Decodes padded EPath
 *  @param epath EPath object to the receiving element
 *  @param message pointer to the message to decode
 *  @return Number of decoded bytes
 */
int DecodePaddedEPath(CipEpath *epath,
                      const EipUint8 **message);

/** @brief Generic implementation of the CIP Create service
 *
 *  Creates dynamically allocated object instance within the specified class.
 *
 * @param instance pointer to instance.
 * @param message_router_request pointer to request.
 * @param message_router_response pointer to response.
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return status  >0 .. success
 *          -1 .. requested instance not created
 */
EipStatus CipCreateService(
    CipInstance *RESTRICT const instance,
    CipMessageRouterRequest *const
        message_router_request,
    CipMessageRouterResponse *const
        message_router_response,
    const struct sockaddr *originator_address,
    const CipSessionHandle encapsulation_session);

/** @brief Generic implementation of the CIP Delete service
 *
 *  Deletes dynamically allocated object instance within the specified class
 *  and updates referred class attributes
 *
 * @param instance pointer to instance.
 * @param message_router_request pointer to request.
 * @param message_router_response pointer to response.
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return status  >0 .. success
 *          -1 .. requested instance not deleted
 */
EipStatus CipDeleteService(CipInstance *RESTRICT const instance,
    CipMessageRouterRequest *const
        message_router_request,
    CipMessageRouterResponse *const
        message_router_response,
    const struct sockaddr *originator_address,
    const CipSessionHandle encapsulation_session);

/** @brief Generic implementation of the CIP Reset service
 *
 *  Causes a transition to a default state or mode of
 *  the object instance within the specified class
 *
 *
 * @param instance pointer to instance.
 * @param message_router_request pointer to request.
 * @param message_router_response pointer to response.
 * @param originator_address address struct of the originator as received
 * @param encapsulation_session associated encapsulation session of the explicit message
 * @return status  >0 .. success
 *          -1 .. requested instance not reseted
 */
EipStatus CipResetService(CipInstance *RESTRICT const instance,
    CipMessageRouterRequest *const
        message_router_request,
    CipMessageRouterResponse *const
        message_router_response,
    const struct sockaddr *originator_address,
    const CipSessionHandle encapsulation_session);

#endif /* OPENER_CIPCOMMON_H_ */
