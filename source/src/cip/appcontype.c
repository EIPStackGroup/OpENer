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
#include "trace.h"


/** @brief External globals needed from connectionmanager.c */
extern ConnectionObject *g_active_connection_list;

/** @brief Exclusive Owner connection data */
typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  ConnectionObject connection_data; /**< the connection data, only one connection is allowed per O-to-T point*/
} ExclusiveOwnerConnection;

/** @brief Input Only connection data */
typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  ConnectionObject connection_data[OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH]; /*< the connection data */
} InputOnlyConnection;

/** @brief Listen Only connection data */
typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  ConnectionObject connection_data[OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH
  ];                                                                               /**< the connection data */
} ListenOnlyConnection;

ExclusiveOwnerConnection g_exlusive_owner_connections[
  OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS];                                                     /**< Exclusive Owner connections */

InputOnlyConnection g_input_only_connections[OPENER_CIP_NUM_INPUT_ONLY_CONNS]; /**< Input Only connections */

ListenOnlyConnection g_listen_only_connections[OPENER_CIP_NUM_LISTEN_ONLY_CONNS]; /**< Listen Only connections */

/** @brief Takes an ConnectionObject and searches and returns an Exclusive Owner Connection based on the ConnectionObject,
 * if there is non it returns NULL
 *
 * @param connection_object Connection Object which will be searched for in the Exclusive Owner Connections
 * @param extended_error Pointer to the extended error variable, will be written if an error occurs
 * @return The corresponding Exclusive Owner Connection or NULL if there is non
 */
ConnectionObject *GetExclusiveOwnerConnection(
  const ConnectionObject *RESTRICT connection_object,
  EipUint16 *const extended_error);

/** @brief Takes an ConnectionObject and searches and returns an Input Only Connection based on the ConnectionObject,
 * if there is non it returns NULL
 *
 * @param connection_object Connection Object which will be searched for in the Input Only Connections
 * @param extended_error Pointer to the extended error variable, will be written if an error occurs
 * @return The corresponding Exclusive Owner Connection or NULL if there is non
 */
ConnectionObject *GetInputOnlyConnection(
  const ConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error);

/** @brief Takes an ConnectionObject and searches and returns an Listen Only Connection based on the ConnectionObject,
 * if there is non it returns NULL
 *
 * @param connection_object Connection Object which will be searched for in the Listen Only Connections
 * @param extended_error Pointer to the extended error variable, will be written if an error occurs
 * @return The corresponding Exclusive Owner Connection or NULL if there is non
 */
ConnectionObject *GetListenOnlyConnection(
  const ConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error);

void ConfigureExclusiveOwnerConnectionPoint(
  const unsigned int connection_number,
  const unsigned int output_assembly,
  const unsigned int input_assembly,
  const unsigned int config_assembly) {
  if (OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS > connection_number) {
    g_exlusive_owner_connections[connection_number].output_assembly =
      output_assembly;
    g_exlusive_owner_connections[connection_number].input_assembly =
      input_assembly;
    g_exlusive_owner_connections[connection_number].config_assembly =
      config_assembly;
  }
}

void ConfigureInputOnlyConnectionPoint(const unsigned int connection_number,
                                       const unsigned int output_assembly,
                                       const unsigned int input_assembly,
                                       const unsigned int config_assembly) {
  if (OPENER_CIP_NUM_INPUT_ONLY_CONNS > connection_number) {
    g_input_only_connections[connection_number].output_assembly =
      output_assembly;
    g_input_only_connections[connection_number].input_assembly = input_assembly;
    g_input_only_connections[connection_number].config_assembly =
      config_assembly;
  }
}

void ConfigureListenOnlyConnectionPoint(const unsigned int connection_number,
                                        const unsigned int output_assembly,
                                        const unsigned int input_assembly,
                                        const unsigned int config_assembly) {
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
  ConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error) {

  *extended_error = 0;

  ConnectionObject *io_connection = GetExclusiveOwnerConnection(
    connection_object,
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
          if ( (NULL == io_connection) && (0 == *extended_error) ) {
            /* no application connection type was found that suits the given data */
            /* TODO check error code VS */
            *extended_error =
              kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
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
  const ConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error) {
  ConnectionObject *exclusive_owner_connection = NULL;

  for (int i = 0; i < OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS; i++) {
    if ( (g_exlusive_owner_connections[i].output_assembly
          == connection_object->connection_path.connection_point[0])
         && (g_exlusive_owner_connections[i].input_assembly
             == connection_object->connection_path.connection_point[1])
         && (g_exlusive_owner_connections[i].config_assembly
             == connection_object->connection_path.connection_point[2]) ) {

      /* check if on other connection point with the same output assembly is currently connected */
      if ( NULL
           != GetConnectedOutputAssembly(
             connection_object->connection_path.connection_point[0]) ) {
        *extended_error =
          kConnectionManagerExtendedStatusCodeErrorOwnershipConflict;
        OPENER_TRACE_INFO("Hit an Ownership conflict in appcontype.c");
        break;
      }
      exclusive_owner_connection = &(g_exlusive_owner_connections[i]
                                     .connection_data);
      break;
    }
  }
  return exclusive_owner_connection;
}

ConnectionObject *GetInputOnlyConnection(
  const ConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error) {
  ConnectionObject *input_only_connection = NULL; //TODO: This variable has no use

  for (int i = 0; i < OPENER_CIP_NUM_INPUT_ONLY_CONNS; i++) {
    if (g_input_only_connections[i].output_assembly
        == connection_object->connection_path.connection_point[0]) { /* we have the same output assembly */
      if (g_input_only_connections[i].input_assembly
          != connection_object->connection_path.connection_point[1]) {
        *extended_error =
          kConnectionManagerExtendedStatusCodeInvalidProducingApplicationPath;
        break;
      }
      if (g_input_only_connections[i].config_assembly
          != connection_object->connection_path.connection_point[2]) {
        *extended_error =
          kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
        break;
      }

      for (int j = 0; j < OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH; j++) {
        if (kConnectionStateNonExistent
            == g_input_only_connections[i].connection_data[j].state) {
          return &(g_input_only_connections[i].connection_data[j]);
        }
      }
      *extended_error =
        kConnectionManagerExtendedStatusCodeTargetObjectOutOfConnections;
      break;
    }
  }
  return input_only_connection;
}

ConnectionObject *GetListenOnlyConnection(
  const ConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error) {
  ConnectionObject *listen_only_connection = NULL; //TODO: This variable has no use

  if ( kForwardOpenConnectionTypeMulticastConnection
       != (connection_object->t_to_o_network_connection_parameter
           & kForwardOpenConnectionTypeMulticastConnection) ) {
    /* a listen only connection has to be a multicast connection. */
    *extended_error =
      kConnectionManagerExtendedStatusCodeNonListenOnlyConnectionNotOpened;   /* maybe not the best error message however there is no suitable definition in the cip spec */
    return NULL;
  }

  for (int i = 0; i < OPENER_CIP_NUM_LISTEN_ONLY_CONNS; i++) {
    if (g_listen_only_connections[i].output_assembly
        == connection_object->connection_path.connection_point[0]) { /* we have the same output assembly */
      if (g_listen_only_connections[i].input_assembly
          != connection_object->connection_path.connection_point[1]) {
        *extended_error =
          kConnectionManagerExtendedStatusCodeInvalidProducingApplicationPath;
        break;
      }
      if (g_listen_only_connections[i].config_assembly
          != connection_object->connection_path.connection_point[2]) {
        *extended_error =
          kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
        break;
      }

      if ( NULL
           == GetExistingProducerMulticastConnection(
             connection_object->connection_path.connection_point[1]) ) {
        *extended_error =
          kConnectionManagerExtendedStatusCodeNonListenOnlyConnectionNotOpened;
        break;
      }

      for (int j = 0; j < OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH; j++) {
        if (kConnectionStateNonExistent
            == g_listen_only_connections[i].connection_data[j].state) {
          return &(g_listen_only_connections[i].connection_data[j]);
        }
      }
      *extended_error =
        kConnectionManagerExtendedStatusCodeTargetObjectOutOfConnections;
      break;
    }
  }
  return listen_only_connection;
}

ConnectionObject *GetExistingProducerMulticastConnection(
  const EipUint32 input_point) {
  ConnectionObject *producer_multicast_connection = g_active_connection_list;

  while (NULL != producer_multicast_connection) {
    if ( (kConnectionTypeIoExclusiveOwner
          == producer_multicast_connection->instance_type)
         || (kConnectionTypeIoInputOnly
             == producer_multicast_connection->instance_type) ) {
      if ( (input_point
            == producer_multicast_connection->connection_path.connection_point[
              1])
           && ( kForwardOpenConnectionTypeMulticastConnection
                == (producer_multicast_connection
                    ->t_to_o_network_connection_parameter
                    & kForwardOpenConnectionTypeMulticastConnection) )
           && (kEipInvalidSocket
               != producer_multicast_connection->socket[
                 kUdpCommuncationDirectionProducing]) ) {
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

ConnectionObject *GetNextNonControlMasterConnection(const EipUint32 input_point)
{
  ConnectionObject *next_non_control_master_connection =
    g_active_connection_list;

  while (NULL != next_non_control_master_connection) {
    if ( (kConnectionTypeIoExclusiveOwner
          == next_non_control_master_connection->instance_type)
         || (kConnectionTypeIoInputOnly
             == next_non_control_master_connection->instance_type) ) {
      if ( (input_point
            == next_non_control_master_connection->connection_path
            .connection_point[1])
           && ( kForwardOpenConnectionTypeMulticastConnection
                == (next_non_control_master_connection
                    ->t_to_o_network_connection_parameter
                    & kForwardOpenConnectionTypeMulticastConnection) )
           && (kEipInvalidSocket
               == next_non_control_master_connection->socket[
                 kUdpCommuncationDirectionProducing]) ) {
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

void CloseAllConnectionsForInputWithSameType(const EipUint32 input_point,
                                             const ConnectionType instance_type)
{
  ConnectionObject *connection = g_active_connection_list;

  while (NULL != connection) {
    if ( (instance_type == connection->instance_type)
         && (input_point == connection->connection_path.connection_point[1]) ) {
      ConnectionObject *connection_to_delete = connection;
      connection = connection->next_connection_object;
      CheckIoConnectionEvent(
        connection_to_delete->connection_path.connection_point[0],
        connection_to_delete->connection_path.connection_point[1],
        kIoConnectionEventClosed);

      assert(connection_to_delete->connection_close_function != NULL);
      connection_to_delete->connection_close_function(connection_to_delete);
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

EipBool8 ConnectionWithSameConfigPointExists(const EipUint32 config_point) {
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
  memset( g_exlusive_owner_connections, 0,
          OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS *
          sizeof(ExclusiveOwnerConnection) );
  memset( g_input_only_connections, 0,
          OPENER_CIP_NUM_INPUT_ONLY_CONNS * sizeof(InputOnlyConnection) );
  memset( g_listen_only_connections, 0,
          OPENER_CIP_NUM_LISTEN_ONLY_CONNS * sizeof(ListenOnlyConnection) );
}
