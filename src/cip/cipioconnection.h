/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef CIPIOCONNECTION_H_
#define CIPIOCONNECTION_H_

#include <opener_api.h>

/** \brief Setup all data in order to establish an IO connection
 *
 * This function can be called after all data has been parsed from the forward open request
 * @param pa_pstConnObjData pointer to the connection object structure holding the parsed data from the forward open request
 * @param pa_pnExtendedError the extended error code in case an error happened
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
int
establishIOConnction(struct CIP_ConnectionObject *pa_pstConnObjData,
    EIP_UINT16 *pa_pnExtendedError);

/** \brief Take the data given in the connection object structure and open the necessary communication channels
 *
 * This function will use the g_stCPFDataItem!
 * @param pa_pstIOConnObj pointer to the connection object data
 * @return general status on the open process
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
int
openCommunicationChannels(struct CIP_ConnectionObject *pa_pstIOConnObj);


/*! \brief close the communication channels of the given connection and remove it
 * from the active connections list.
 *
 * @param pa_pstIOConnObj pointer to the connection object data
 */
void
closeCommChannelsAndRemoveFromActiveConnsList(
    struct CIP_ConnectionObject *pa_pstConnObjData);

extern EIP_UINT8 *g_pnConfigDataBuffer;
extern unsigned int g_unConfigDataLen;

#endif /* CIPIOCONNECTION_H_ */
