/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>

#include "appcontype.h"

#include "cipconnectionmanager.h"
#include "opener_api.h"
#include "assert.h"


/** @brief External globals needed from connectionmanager.c */
extern ConnectionObject *g_active_connection_list;

typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  ConnectionObject connection_data; /**< the connection data, only one connection is allowed per O-to-T point*/
} ExclusiveOwnerConnection;

typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  ConnectionObject connection_data[OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH]; /*< the connection data */
} InputOnlyConnection;

typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  ConnectionObject connection_data[OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH]; /**< the connection data */
} ListenOnlyConnection;

ExclusiveOwnerConnection g_exlusive_owner_connections[OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS];

InputOnlyConnection g_input_only_connections[OPENER_CIP_NUM_INPUT_ONLY_CONNS];

ListenOnlyConnection g_listen_only_connections[OPENER_CIP_NUM_LISTEN_ONLY_CONNS];

ConnectionObject *GetExclusiveOwnerConnection(
    ConnectionObject *connection_object, EipUint16 *extended_error);

ConnectionObject *GetInputOnlyConnection(ConnectionObject *connection_object,
                                         EipUint16 *extended_error);

ConnectionObject *GetListenOnlyConnection(ConnectionObject *connection_object,
                                          EipUint16 *extended_error);

void ConfigureExclusiveOwnerConnectionPoint(unsigned int connection_number,
                                            unsigned int output_assembly,
                                            unsigned int input_assembly,
                                            unsigned int config_assembly) {
  if (OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS > connection_number) {
    g_exlusive_owner_connections[connection_number].output_assembly =
        output_assembly;
    g_exlusive_owner_connections[connection_number].input_assembly =
        input_assembly;
    g_exlusive_owner_connections[connection_number].config_assembly =
        config_assembly;
  }
}

void ConfigureInputOnlyConnectionPoint(unsigned int connection_number,
                                       unsigned int output_assembly,
                                       unsigned int input_assembly,
                                       unsigned int config_assembly) {
  if (OPENER_CIP_NUM_INPUT_ONLY_CONNS > connection_number) {
    g_input_only_connections[connection_number].output_assembly =
        output_assembly;
    g_input_only_connections[connection_number].input_assembly = input_assembly;
    g_input_only_connections[connection_number].config_assembly =
        config_assembly;
  }
}

void ConfigureListenOnlyConnectionPoint(unsigned int connection_number,
                                        unsigned int output_assembly,
                                        unsigned int input_assembly,
                                        unsigned int config_assembly) {
  if (OPENER_CIP_NUM_LISTEN_ONLY_CONNS > connection_number) {
    g_listen_only_connections[connection_number].output_assembly =
        output_assembly;
    g_listen_only_connections[connection_number].input_assembly =
        input_assembly;
    g_listen_only_connections[connection_number].config_assembly =
        config_assembly;
  }
}

ConnectionObject *GetIoConnectionForConnectionData(
    ConnectionObject *connection_object, EipUint16 *extended_error) {
  ConnectionObject *io_connection = NULL;
  *extended_error = 0;

  io_connection = GetExclusiveOwnerConnection(connection_object,
                                              extended_error);
  if (NULL == io_connection) {
    if (0 == *extended_error) {
      /* we found no connection and don't have an error so try input only next */
      io_connection = GetInputOnlyConnection(connection_object, extended_error);
      if (NULL == io_connection) {
        if (0 == *extended_error) {
          /* we found no connection and don't have an error so try listen only next */
          io_connection = GetListenOnlyConnection(connection_object,
                                                  extended_error);
          if ((NULL == io_connection) && (0 == *extended_error)) {
            /* no application connection type was found that suits the given data */
            /* TODO check error code VS */
            *extended_error =
                kConnectionManagerStatusCodeInconsistentApplicationPathCombo;
          } else {
            connection_object->instance_type = kConnectionTypeIoListenOnly;
          }
        }
      } else {
        connection_object->instance_type = kConnectionTypeIoInputOnly;
      }
    }
  } else {
    connection_object->instance_type = kConnectionTypeIoExclusiveOwner;
  }

  if (NULL != io_connection) {
    CopyConnectionData(io_connection, connection_object);
  }

  return io_connection;
}

ConnectionObject *GetExclusiveOwnerConnection(
    ConnectionObject *connection_object, EipUint16 *extended_error) {
  ConnectionObject *exclusive_owner_connection = NULL;
  int i;

  for (i = 0; i < OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS; i++) {
    if ((g_exlusive_owner_connections[i].output_assembly
        == connection_object->connection_path.connection_point[0])
        && (g_exlusive_owner_connections[i].input_assembly
            == connection_object->connection_path.connection_point[1])
        && (g_exlusive_owner_connections[i].config_assembly
            == connection_object->connection_path.connection_point[2])) {

      /* check if on other connection point with the same output assembly is currently connected */
      if (NULL
          != GetConnectedOutputAssembly(
              connection_object->connection_path.connection_point[0])) {
        *extended_error = kConnectionManagerStatusCodeErrorOwnershipConflict;
        break;
      }
      exclusive_owner_connection = &(g_exlusive_owner_connections[i]
          .connection_data);
      break;
    }
  }
  return exclusive_owner_connection;
}

ConnectionObject *GetInputOnlyConnection(ConnectionObject *connection_object,
                                         EipUint16 *extended_error) {
  ConnectionObject *input_only_connection = NULL;
  int i, j;

  for (i = 0; i < OPENER_CIP_NUM_INPUT_ONLY_CONNS; i++) {
    if (g_input_only_connections[i].output_assembly
        == connection_object->connection_path.connection_point[0]) { /* we have the same output assembly */
      if (g_input_only_connections[i].input_assembly
          != connection_object->connection_path.connection_point[1]) {
        *extended_error =
            kConnectionManagerStatusCodeInvalidProducingApplicationPath;
        break;
      }
      if (g_input_only_connections[i].config_assembly
          != connection_object->connection_path.connection_point[2]) {
        *extended_error =
            kConnectionManagerStatusCodeInconsistentApplicationPathCombo;
        break;
      }

      for (j = 0; j < OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH; j++) {
        if (kConnectionStateNonExistent
            == g_input_only_connections[i].connection_data[j].state) {
          return &(g_input_only_connections[i].connection_data[j]);
        }
      }
      *extended_error =
          kConnectionManagerStatusCodeTargetObjectOutOfConnections;
      break;
    }
  }
  return input_only_connection;
}

ConnectionObject *GetListenOnlyConnection(ConnectionObject *connection_object,
                                          EipUint16 *extended_error) {
  ConnectionObject *listen_only_connection = NULL;
  int i, j;

  if (kRoutingTypeMulticastConnection
      != (connection_object->t_to_o_network_connection_parameter
          & kRoutingTypeMulticastConnection)) {
    /* a listen only connection has to be a multicast connection. */
    *extended_error =
        kConnectionManagerStatusCodeNonListenOnlyConnectionNotOpened; /* maybe not the best error message however there is no suitable definition in the cip spec */
    return NULL;
  }

  for (i = 0; i < OPENER_CIP_NUM_LISTEN_ONLY_CONNS; i++) {
    if (g_listen_only_connections[i].output_assembly
        == connection_object->connection_path.connection_point[0]) { /* we have the same output assembly */
      if (g_listen_only_connections[i].input_assembly
          != connection_object->connection_path.connection_point[1]) {
        *extended_error =
            kConnectionManagerStatusCodeInvalidProducingApplicationPath;
        break;
      }
      if (g_listen_only_connections[i].config_assembly
          != connection_object->connection_path.connection_point[2]) {
        *extended_error =
            kConnectionManagerStatusCodeInconsistentApplicationPathCombo;
        break;
      }

      if (NULL
          == GetExistingProducerMulticastConnection(
              connection_object->connection_path.connection_point[1])) {
        *extended_error =
            kConnectionManagerStatusCodeNonListenOnlyConnectionNotOpened;
        break;
      }

      for (j = 0; j < OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH; j++) {
        if (kConnectionStateNonExistent
            == g_listen_only_connections[i].connection_data[j].state) {
          return &(g_listen_only_connections[i].connection_data[j]);
        }
      }
      *extended_error =
          kConnectionManagerStatusCodeTargetObjectOutOfConnections;
      break;
    }
  }
  return listen_only_connection;
}

ConnectionObject *GetExistingProducerMulticastConnection(EipUint32 input_point) {
  ConnectionObject *producer_multicast_connection = g_active_connection_list;

  while (NULL != producer_multicast_connection) {
    if ((kConnectionTypeIoExclusiveOwner
        == producer_multicast_connection->instance_type)
        || (kConnectionTypeIoInputOnly
            == producer_multicast_connection->instance_type)) {
      if ((input_point
          == producer_multicast_connection->connection_path.connection_point[1])
          && (kRoutingTypeMulticastConnection
              == (producer_multicast_connection
                  ->t_to_o_network_connection_parameter
                  & kRoutingTypeMulticastConnection))
          && (kEipInvalidSocket
              != producer_multicast_connection->socket[kUdpCommuncationDirectionProducing])) {
        /* we have a connection that produces the same input assembly,
         * is a multicast producer and manages the connection.
         */
        break;
      }
    }
    producer_multicast_connection = producer_multicast_connection
        ->next_connection_object;
  }
  return producer_multicast_connection;
}

ConnectionObject *GetNextNonControlMasterConnection(EipUint32 input_point) {
  ConnectionObject *next_non_control_master_connection =
      g_active_connection_list;

  while (NULL != next_non_control_master_connection) {
    if ((kConnectionTypeIoExclusiveOwner
        == next_non_control_master_connection->instance_type)
        || (kConnectionTypeIoInputOnly
            == next_non_control_master_connection->instance_type)) {
      if ((input_point
          == next_non_control_master_connection->connection_path
              .connection_point[1])
          && (kRoutingTypeMulticastConnection
              == (next_non_control_master_connection
                  ->t_to_o_network_connection_parameter
                  & kRoutingTypeMulticastConnection))
          && (kEipInvalidSocket
              == next_non_control_master_connection->socket[kUdpCommuncationDirectionProducing])) {
        /* we have a connection that produces the same input assembly,
         * is a multicast producer and does not manages the connection.
         */
        break;
      }
    }
    next_non_control_master_connection = next_non_control_master_connection
        ->next_connection_object;
  }
  return next_non_control_master_connection;
}

void CloseAllConnectionsForInputWithSameType(EipUint32 input_point,
                                             ConnectionType instance_type) {
  ConnectionObject *connection = g_active_connection_list;
  ConnectionObject *connection_to_delete;

  while (NULL != connection) {
    if ((instance_type == connection->instance_type)
        && (input_point == connection->connection_path.connection_point[1])) {
      connection_to_delete = connection;
      connection = connection->next_connection_object;
      CheckIoConnectionEvent(
          connection_to_delete->connection_path.connection_point[0],
          connection_to_delete->connection_path.connection_point[1],
          kIoConnectionEventClosed);

      /* FIXME check if this is ok */
      connection_to_delete->connection_close_function(connection_to_delete);
      /*closeConnection(pstToDelete); will remove the connection from the active connection list */
    } else {
      connection = connection->next_connection_object;
    }
  }
}

void CloseAllConnections(void) {
  ConnectionObject *connection = g_active_connection_list;
  while (NULL != connection) {
    assert(connection->connection_close_function != NULL);
    connection->connection_close_function(connection);
    CloseConnection(connection);
    /* Close connection will remove the connection from the list therefore we
     * need to get again the start until there is no connection left
     */
    connection = g_active_connection_list;
  }

}

EipBool8 ConnectionWithSameConfigPointExists(EipUint32 config_point) {
  ConnectionObject *connection = g_active_connection_list;

  while (NULL != connection) {
    if (config_point == connection->connection_path.connection_point[2]) {
      break;
    }
    connection = connection->next_connection_object;
  }
  return (NULL != connection);
}

void InitializeIoConnectionData(void) {
  memset(g_exlusive_owner_connections, 0,
  OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS * sizeof(ExclusiveOwnerConnection));
  memset(g_input_only_connections, 0,
  OPENER_CIP_NUM_INPUT_ONLY_CONNS * sizeof(InputOnlyConnection));
  memset(g_listen_only_connections, 0,
  OPENER_CIP_NUM_LISTEN_ONLY_CONNS * sizeof(ListenOnlyConnection));
}
