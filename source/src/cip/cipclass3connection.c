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
int EstablishClass3Connection(ConnectionObject *connection_object,
                              EipUint16 *extended_error) {
  int eip_status = kEipStatusOk;
  EipUint32 produced_connection_id_buffer;

  /*TODO add check for transport type trigger */
  /* if (0x03 == (g_stDummyConnectionObject.TransportTypeClassTrigger & 0x03)) */

  ConnectionObject *explicit_connection = GetFreeExplicitConnection();

  if (NULL == explicit_connection) {
    eip_status = kCipErrorConnectionFailure;
    *extended_error =
        kConnectionManagerStatusCodeErrorNoMoreConnectionsAvailable;
  } else {
    CopyConnectionData(explicit_connection, connection_object);

    produced_connection_id_buffer = explicit_connection->produced_connection_id;
    GeneralConnectionConfiguration(explicit_connection);
    explicit_connection->produced_connection_id = produced_connection_id_buffer;
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

ConnectionObject *GetFreeExplicitConnection(void) {
  int i;
  for (i = 0; i < OPENER_CIP_NUM_EXPLICIT_CONNS; i++) {
    if (g_explicit_connections[i].state == kConnectionStateNonExistent)
      return &(g_explicit_connections[i]);
  }
  return NULL;
}

void InitializeClass3ConnectionData(void) {
  memset(g_explicit_connections, 0,
  OPENER_CIP_NUM_EXPLICIT_CONNS * sizeof(ConnectionObject));
}
