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
 * @param pa_pstConnObj pointer to the connection object structure holding the parsed data from the forward open request
 * @param pa_pnExtendedError the extended error code in case an error happened
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
int EstablishClass3Connection(ConnectionObject *connection_object,
                              EipUint16 *extended_error);

void InitializeClass3ConnectionData(void);

#endif /* OPENER_CIPCLASS3CONNECTION_H_ */
