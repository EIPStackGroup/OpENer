/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef CIPCLASS3CONNECTION_H_
#define CIPCLASS3CONNECTION_H_


#include <opener_api.h>

/** \brief Check if Class3 connection is available and if yes setup all data.
 *
 * This function can be called after all data has been parsed from the forward open request
 * @param pa_pstConnObj pointer to the connection object structure holding the parsed data from the forward open request
 * @param pa_pnExtendedError the extended error code in case an error happened
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
int
establishClass3Connection(struct CIP_ConnectionObject *pa_pstConnObj, EIP_UINT16 *pa_pnExtendedError);

void initializeClass3ConnectionData();

#endif /* CIPCLASS3CONNECTION_H_ */
