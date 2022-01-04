/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPMESSAGEROUTER_H_
#define OPENER_CIPMESSAGEROUTER_H_

#include "typedefs.h"
#include "ciptypes.h"

/** @brief Message Router class code */
static const CipUint kCipMessageRouterClassCode = 0x02U;

/* public functions */

/** @brief Initialize the data structures of the message router
 *  @return kEipStatusOk if class was initialized, otherwise kEipStatusError
 */
EipStatus CipMessageRouterInit(void);

/** @brief Free all data allocated by the classes created in the CIP stack
 */
void DeleteAllClasses(void);

/** @brief Notify the MessageRouter that an explicit message (connected or unconnected)
 *  has been received. This function will be called from the encapsulation layer.
 *  The CPF structure is already parsed an can be accessed via the global variable:
 *  g_stCPFDataItem.
 *  @param data pointer to the data buffer of the message directly at the beginning of the CIP part.
 *  @param data_length number of bytes in the data buffer
 *  @param originator_address The address of the originator as received
 *  @param encapsulation_session The associated encapsulation session of the explicit message
 *  @return  kEipStatusError on fault
 *           kEipStatusOk on success
 */
EipStatus NotifyMessageRouter(EipUint8 *data,
                              int data_length,
                              CipMessageRouterResponse *message_router_response,
                              const struct sockaddr *const originator_address,
                              const CipSessionHandle encapsulation_session);

/*! Register a class at the message router.
 *  In order that the message router can deliver
 *  explicit messages each class has to register.
 *  Will be automatically done when invoking create
 *  createCIPClass.
 *  @param cip_class CIP class to be registered
 *  @return kEipStatusOk on success
 */
EipStatus RegisterCipClass(CipClass *cip_class);

#endif /* OPENER_CIPMESSAGEROUTER_H_ */
