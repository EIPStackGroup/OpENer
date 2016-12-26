/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>

#include "cipclass3connection.h"

ConnectionObject *GetFreeExplicitConnection(void);

/**** Global variables ****/

/** @brief Array of the available explicit connections */
ConnectionObject g_explicit_connections[OPENER_CIP_NUM_EXPLICIT_CONNS];

/**** Implementation ****/
EipStatus EstablishClass3Connection(
  ConnectionObject *RESTRICT const connection_object,
  EipUint16 *const extended_error) {
  EipStatus eip_status = kEipStatusOk;

  /*TODO add check for transport type trigger */
  /* if (0x03 == (g_stDummyConnectionObject.TransportTypeClassTrigger & 0x03)) */

  ConnectionObject *explicit_connection = GetFreeExplicitConnection();

  if (NULL == explicit_connection) {
    eip_status = kCipErrorConnectionFailure;
    *extended_error =
      kConnectionManagerExtendedStatusCodeErrorNoMoreConnectionsAvailable;
  } else {
    CopyConnectionData(explicit_connection, connection_object);

    EipUint32 produced_connection_id_buffer =
      explicit_connection->cip_produced_connection_id;
    GeneralConnectionConfiguration(explicit_connection);
    explicit_connection->cip_produced_connection_id =
      produced_connection_id_buffer;
    explicit_connection->instance_type = kConnectionTypeExplicit;
    explicit_connection->socket[0] = explicit_connection->socket[1] =
                                       kEipInvalidSocket;
    /* set the connection call backs */
    explicit_connection->connection_close_function =
      RemoveFromActiveConnections;
    /* explicit connection have to be closed on time out*/
    explicit_connection->connection_timeout_function =
      RemoveFromActiveConnections;

    AddNewActiveConnection(explicit_connection);
  }
  return eip_status;
}

/** @brief Searches and returns a free explicit connection slot
 *
 * @return Free explicit connection slot, or NULL if no slot is free
 */
ConnectionObject *GetFreeExplicitConnection(void) {
  for (int i = 0; i < OPENER_CIP_NUM_EXPLICIT_CONNS; i++) {
    if (g_explicit_connections[i].state == kConnectionStateNonExistent) {
      return &(g_explicit_connections[i]);
    }
  }
  return NULL;
}

void InitializeClass3ConnectionData(void) {
  memset( g_explicit_connections, 0,
          OPENER_CIP_NUM_EXPLICIT_CONNS * sizeof(ConnectionObject) );
}
