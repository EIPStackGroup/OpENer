/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_APPCONTYPE_H_
#define OPENER_APPCONTYPE_H_

#include "cipconnectionmanager.h"

void InitializeIoConnectionData(void);

/** @brief check if for the given connection data received in a forward_open request
 *  a suitable connection is available.
 *
 *  If a suitable connection is found the connection data is transfered the
 *  application connection type is set (i.e., EConnType).
 *  @param connection_object connection data to be used
 *  @param extended_error pointer to the extended_error variable, if an error occurred this value has the according
 *     error code for the response
 *  @return
 *        - on success: A pointer to the connection object already containing the connection
 *          data given in connection_object.
 *        - on error: NULL
 */
CipConnectionObject *GetIoConnectionForConnectionData(
  CipConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error);

/** @brief Check if there exists already an exclusive owner or listen only connection
 *         which produces the input assembly.
 *
 *  @param  multicast_only  Look only for multi-cast connections
 *  @param  input_point     the Input point to be produced
 *  @return   a pointer to the found connection; NULL if nothing found
 */
CipConnectionObject *GetExistingProducerIoConnection(
  const bool multicast_only,
  const EipUint32 input_point);

/** @brief check if there exists an producing multicast exclusive owner or
 * listen only connection that should produce the same input but is not in charge
 * of the connection.
 *
 * @param input_point the produced input
 * @return if a connection could be found the pointer to this connection
 *      otherwise NULL.
 */
CipConnectionObject *GetNextNonControlMasterConnection(
  const EipUint32 input_point);

/** @brief Close all connection producing the same input and have the same type
 * (i.e., listen only or input only).
 *
 * @param input_point the input point
 * @param instance_type the connection application type
 */
void CloseAllConnectionsForInputWithSameType(const EipUint32 input_point,
                                             const ConnectionObjectInstanceType instance_type);

/**@ brief close all open connections.
 *
 * For I/O connections the sockets will be freed. The sockets for explicit
 * connections are handled by the encapsulation layer, and freed there.
 */
void CloseAllConnections(void);

/** @brief Check if there is an established connection that uses the same
 * config point.
 *
 * @param config_point The configuration point
 * @return true if connection was found, otherwise false
 */
bool ConnectionWithSameConfigPointExists(const EipUint32 config_point);

#endif /* OPENER_APPCONTYPE_H_ */
