/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#ifndef APPCONTYPE_H_
#define APPCONTYPE_H_

#include "cipconnectionmanager.h"

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

#endif /* APPCONTYPE_H_ */
