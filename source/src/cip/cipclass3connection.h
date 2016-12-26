/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_CIPCLASS3CONNECTION_H_
#define OPENER_CIPCLASS3CONNECTION_H_

/** @file cipclass3connection.h
 *  @brief CIP Class 3 connection
 */

#include "opener_api.h"
#include "cipconnectionmanager.h"

/** @brief Check if Class3 connection is available and if yes setup all data.
 *
 * This function can be called after all data has been parsed from the forward open request
 * @param connection_object pointer to the connection object structure holding the parsed data from the forward open request
 * @param extended_error the extended error code in case an error happened
 * @return general status on the establishment
 *    - kEipStatusOk ... on success
 *    - On an error the general status code to be put into the response
 */
EipStatus EstablishClass3Connection(
  ConnectionObject *RESTRICT const connection_object,
  EipUint16 *const extended_error);

/** @brief Initializes the explicit connections mechanism
 *
 *  Prepares the available explicit connection slots for use at the start of the OpENer
 */
void InitializeClass3ConnectionData(void);

#endif /* OPENER_CIPCLASS3CONNECTION_H_ */
