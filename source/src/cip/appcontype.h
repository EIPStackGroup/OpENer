/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef APPCONTYPE_H_
#define APPCONTYPE_H_

#include "cipconnectionmanager.h"


void initializeIOConnectionData();

/*! \brief check if for the given connection data received in a forward_open request
 *  a suitable connection is available.
 *
 *  If a suitable connection is found the connection data is transfered the
 *  application connection type is set (i.e., EConnType).
 *  @param pa_pstConnData connection data to be used
 *  @param pa_pnExtendedError if an error occurred this value has the according
 *     error code for the response
 *  @return
 *        - on success: A pointer to the connection object already containing the connection
 *          data given in pa_pstConnData.
 *        - on error: NULL
 */
S_CIP_ConnectionObject *
getIOConnectionForConnectionData(S_CIP_ConnectionObject *pa_pstConnData,
    EIP_UINT16 *pa_pnExtendedError);

/*! \brief Check if there exists already an exclusive owner or listen only connection
 *         which produces the input assembly.
 *
 *  @param pa_unInputPoint the Input point to be produced
 *  @return if a connection could be found a pointer to this connection if not NULL
 */
S_CIP_ConnectionObject *
getExistingProdMulticastConnection(EIP_UINT32 pa_unInputPoint);

/*! \brief check if there exists an producing multicast exclusive owner or
 * listen only connection that should produce the same input but is not in charge
 * of the connection.
 *
 * @param pa_unInputPoint the produced input
 * @return if a connection could be found the pointer to this connection
 *      otherwise NULL.
 */
S_CIP_ConnectionObject *
getNextNonCtrlMasterCon(EIP_UINT32 pa_unInputPoint);

/*! \brief Close all connection producing the same input and have the same type
 * (i.e., listen only or input only).
 *
 * @param pa_unInputPoint  the input point
 * @param pa_eInstanceType the connection application type
 */
void
closeAllConnsForInputWithSameType(EIP_UINT32 pa_unInputPoint,
    EConnType pa_eInstanceType);


/*!\brief close all open connections.
 *
 * For I/O connections the sockets will be freed. The sockets for explicit
 * connections are handled by the encapsulation layer, and freed there.
 */
void closeAllConnections(void);


/*! \brief Check if there is an established connection that uses the same
 * config point.
 */
EIP_BOOL8
connectionWithSameConfigPointExists(EIP_UINT32 pa_unConfigPoint);

#endif /* APPCONTYPE_H_ */
