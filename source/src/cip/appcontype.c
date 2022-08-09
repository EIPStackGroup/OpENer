/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>

#include "appcontype.h"

#include "cipconnectionmanager.h"
#include "cipconnectionobject.h"
#include "opener_api.h"
#include "assert.h"
#include "trace.h"
#include "cipepath.h"

/** @brief Exclusive Owner connection data */
typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  CipConnectionObject connection_data; /**< the connection data, only one connection is allowed per O-to-T point*/
} ExclusiveOwnerConnection;

/** @brief Input Only connection data */
typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  CipConnectionObject connection_data[
    OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH];                                   /*< the connection data */
} InputOnlyConnection;

/** @brief Listen Only connection data */
typedef struct {
  unsigned int output_assembly; /**< the O-to-T point for the connection */
  unsigned int input_assembly; /**< the T-to-O point for the connection */
  unsigned int config_assembly; /**< the config point for the connection */
  CipConnectionObject connection_data[
    OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH
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
CipConnectionObject *GetExclusiveOwnerConnection(
  const CipConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error);

/** @brief Takes an ConnectionObject and searches and returns an Input Only Connection based on the ConnectionObject,
 * if there is non it returns NULL
 *
 * @param connection_object Connection Object which will be searched for in the Input Only Connections
 * @param extended_error Pointer to the extended error variable, will be written if an error occurs
 * @return The corresponding Exclusive Owner Connection or NULL if there is non
 */
CipConnectionObject *GetInputOnlyConnection(
  const CipConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error);

/** @brief Takes an ConnectionObject and searches and returns an Listen Only Connection based on the ConnectionObject,
 * if there is non it returns NULL
 *
 * @param connection_object Connection Object which will be searched for in the Listen Only Connections
 * @param extended_error Pointer to the extended error variable, will be written if an error occurs
 * @return The corresponding Exclusive Owner Connection or NULL if there is non
 */
CipConnectionObject *GetListenOnlyConnection(
  const CipConnectionObject *const RESTRICT connection_object,
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

CipConnectionObject *GetIoConnectionForConnectionData(
  CipConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error) {

  *extended_error = 0;

  CipConnectionObject *io_connection = GetExclusiveOwnerConnection(
    connection_object,
    extended_error);
  if (NULL == io_connection) {
    if (kConnectionManagerExtendedStatusCodeSuccess == *extended_error) {
      /* we found no connection and don't have an error so try input only next */
      io_connection = GetInputOnlyConnection(connection_object, extended_error);
      if (NULL == io_connection) {
        if (kConnectionManagerExtendedStatusCodeSuccess == *extended_error) {
          /* we found no connection and don't have an error so try listen only next */
          io_connection = GetListenOnlyConnection(connection_object,
                                                  extended_error);
          if ( (NULL == io_connection) &&
               (kCipErrorSuccess == *extended_error) ) {
            /* no application connection type was found that suits the given data */
            *extended_error =
              kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
          } else {
            ConnectionObjectSetInstanceType(connection_object,
                                            kConnectionObjectInstanceTypeIOListenOnly);
            OPENER_TRACE_INFO("IO Listen only connection requested\n");
            //Is listen only connection
          }
        }
      } else {
        ConnectionObjectSetInstanceType(connection_object,
                                        kConnectionObjectInstanceTypeIOInputOnly);
        OPENER_TRACE_INFO("IO Input only connection requested\n");
        //is Input only connection
      }
    }
  } else {
    ConnectionObjectSetInstanceType(connection_object,
                                    kConnectionObjectInstanceTypeIOExclusiveOwner);
    OPENER_TRACE_INFO("IO Exclusive Owner connection requested\n");
    //Is exclusive owner connection
  }

  if (NULL != io_connection) {
    ConnectionObjectDeepCopy(io_connection, connection_object);
  }

  return io_connection;
}

CipConnectionObject *GetExclusiveOwnerConnection(
  const CipConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error) {

  for (size_t i = 0; i < OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS; ++i) {
    if ( (g_exlusive_owner_connections[i].output_assembly ==
          connection_object->consumed_path.instance_id)
         && (g_exlusive_owner_connections[i].input_assembly ==
             connection_object->produced_path.instance_id)
         && (g_exlusive_owner_connections[i].config_assembly ==
             connection_object->configuration_path.instance_id) ) {

      /* check if on other connection point with the same output assembly is currently connected */
      const CipConnectionObject *const exclusive_owner =
        GetConnectedOutputAssembly(
          connection_object->produced_path.instance_id);
      if ( NULL
           != exclusive_owner ) {
        if(kConnectionObjectStateEstablished ==
           ConnectionObjectGetState(exclusive_owner) ) {
          *extended_error =
            kConnectionManagerExtendedStatusCodeErrorOwnershipConflict;
          OPENER_TRACE_INFO("Hit an Ownership conflict in appcontype.c:198\n");
          break;
        }
        if(kConnectionObjectStateTimedOut ==
           ConnectionObjectGetState(exclusive_owner)
           && ConnectionObjectEqualOriginator(connection_object,
                                              exclusive_owner) ) {
          g_exlusive_owner_connections[i].connection_data.
          connection_close_function(&(g_exlusive_owner_connections[i].
                                      connection_data) );
          return &(g_exlusive_owner_connections[i].connection_data);
        } else {
          *extended_error =
            kConnectionManagerExtendedStatusCodeErrorOwnershipConflict;
          OPENER_TRACE_INFO(
            "Hit an Ownership conflict with timed out connection");
          break;
        }
      }
      return &(g_exlusive_owner_connections[i].connection_data);
    }
  }
  return NULL;
}

CipConnectionObject *GetInputOnlyConnection(
  const CipConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error) {
  EipUint16 err = 0;

  for (size_t i = 0; i < OPENER_CIP_NUM_INPUT_ONLY_CONNS; ++i) {
    if (g_input_only_connections[i].output_assembly
        == connection_object->consumed_path.instance_id) { /* we have the same output assembly */
      if (g_input_only_connections[i].input_assembly
          != connection_object->produced_path.instance_id) {
        err = kConnectionManagerExtendedStatusCodeInvalidProducingApplicationPath;
        continue;
      }
      if (g_input_only_connections[i].config_assembly
          != connection_object->configuration_path.instance_id) {
        err = kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
        continue;
      }

      for (size_t j = 0; j < OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH;
           ++j) {
        if (kConnectionObjectStateTimedOut
            == ConnectionObjectGetState(&(g_input_only_connections[i].
                                          connection_data[j]) )
            && ConnectionObjectEqualOriginator(connection_object,
                                               &(g_input_only_connections[i].
                                                 connection_data[j]))) {
          g_input_only_connections[i].connection_data[j].
          connection_close_function(
            &g_input_only_connections[i].connection_data[j]);
          return &(g_input_only_connections[i].connection_data[j]);
        }
      }

      for (size_t j = 0; j < OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH;
           ++j) {
        if (kConnectionObjectStateNonExistent
            == ConnectionObjectGetState(&(g_input_only_connections[i].
                                          connection_data[j]) ) ) {
          return &(g_input_only_connections[i].connection_data[j]);
        }
      }
      err = kConnectionManagerExtendedStatusCodeTargetObjectOutOfConnections;
      break;
    }
  }

  *extended_error = err;
  return NULL;
}

CipConnectionObject *GetListenOnlyConnection(
  const CipConnectionObject *const RESTRICT connection_object,
  EipUint16 *const extended_error) {
  EipUint16 err = 0;

  for (size_t i = 0; i < OPENER_CIP_NUM_LISTEN_ONLY_CONNS; i++) {
    if (g_listen_only_connections[i].output_assembly
        == connection_object->consumed_path.instance_id) { /* we have the same output assembly */
      if (g_listen_only_connections[i].input_assembly
          != connection_object->produced_path.instance_id) {
        err = kConnectionManagerExtendedStatusCodeInvalidProducingApplicationPath;
        continue;
      }
      if (g_listen_only_connections[i].config_assembly
          != connection_object->configuration_path.instance_id) {
        err = kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo;
        continue;
      }

      /* Here we look for both Point-to-Point and Multicast IO connections */
      if ( NULL == GetExistingProducerIoConnection(false,
                                                   connection_object->
                                                   produced_path.instance_id)) {
        err = kConnectionManagerExtendedStatusCodeNonListenOnlyConnectionNotOpened;
        break;
      }

      for (size_t j = 0; j < OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH;
           ++j) {
        if (kConnectionObjectStateTimedOut
            == ConnectionObjectGetState(&(g_listen_only_connections[i].
                                          connection_data[j]) )
            && ConnectionObjectEqualOriginator(connection_object,
                                               &(g_listen_only_connections[i].
                                                 connection_data[j]))) {
          g_listen_only_connections[i].connection_data[j].
          connection_close_function(
            &g_listen_only_connections[i].connection_data[j]);
          return &(g_listen_only_connections[i].connection_data[j]);
        }
      }

      for (size_t j = 0; j < OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH;
           j++) {
        if (kConnectionObjectStateNonExistent
            == ConnectionObjectGetState(&(g_listen_only_connections[i].
                                          connection_data[j]) ) ) {
          return &(g_listen_only_connections[i].connection_data[j]);
        }
      }
      err = kConnectionManagerExtendedStatusCodeTargetObjectOutOfConnections;
      break;
    }
  }

  *extended_error = err;
  return NULL;
}

CipConnectionObject *GetExistingProducerIoConnection(
  const bool multicast_only,
  const EipUint32 input_point) {
  const DoublyLinkedListNode *node = connection_list.first;

  while (NULL != node) {
    CipConnectionObject *producer_io_connection = node->data;
    if (ConnectionObjectIsTypeIOConnection(producer_io_connection) &&
        (input_point == producer_io_connection->produced_path.instance_id) &&
        (kEipInvalidSocket !=
         producer_io_connection->socket[kUdpCommuncationDirectionProducing]) )
    {
      ConnectionObjectConnectionType cnxn_type =
        ConnectionObjectGetTToOConnectionType(producer_io_connection);
      /* we have a connection that produces the same input assembly,
       * and manages the connection.
       */
      if (kConnectionObjectConnectionTypeMulticast == cnxn_type) {
        return producer_io_connection;
      }
      if (!multicast_only &&
          kConnectionObjectConnectionTypePointToPoint == cnxn_type)
      {
        return producer_io_connection;
      }
    }
    node = node->next;
  }
  return NULL;
}

CipConnectionObject *GetNextNonControlMasterConnection(
  const EipUint32 input_point) {
  const DoublyLinkedListNode *node = connection_list.first;

  while (NULL != node) {
    CipConnectionObject *next_non_control_master_connection =
      node->data;
    if ( true ==
         ConnectionObjectIsTypeNonLOIOConnection(
           next_non_control_master_connection)
         && kConnectionObjectStateEstablished ==
         ConnectionObjectGetState(next_non_control_master_connection)
         && input_point ==
         next_non_control_master_connection->produced_path.instance_id
         &&  kConnectionObjectConnectionTypeMulticast ==
         ConnectionObjectGetTToOConnectionType(
           next_non_control_master_connection)
         && (kEipInvalidSocket ==
             next_non_control_master_connection->socket[
               kUdpCommuncationDirectionProducing
             ]) ) {
      /* we have a connection that produces the same input assembly,
       * is a multicast producer and does not manage the connection.
       */
      return next_non_control_master_connection;
    }
    node = node->next;
  }
  return NULL;
}

void CloseAllConnectionsForInputWithSameType(const EipUint32 input_point,
                                             const ConnectionObjectInstanceType instance_type)
{

  OPENER_TRACE_INFO("Close all instance type %d only connections\n",
                    instance_type);
  const DoublyLinkedListNode *node = connection_list.first;
  while (NULL != node) {
    CipConnectionObject *const connection = node->data;
    node = node->next;
    if ( (instance_type == ConnectionObjectGetInstanceType(connection) )
         && (input_point == connection->produced_path.instance_id) ) {
      CipConnectionObject *connection_to_delete = connection;
      CheckIoConnectionEvent(
        connection_to_delete->consumed_path.instance_id,
        connection_to_delete->produced_path.instance_id,
        kIoConnectionEventClosed);

      assert(connection_to_delete->connection_close_function != NULL);
      connection_to_delete->connection_close_function(connection_to_delete);
    }
  }
}

void CloseAllConnections(void) {
  const DoublyLinkedListNode *node = connection_list.first;
  while (NULL != node) {
    CipConnectionObject *const connection = node->data;
    assert(connection->connection_close_function != NULL);
    connection->connection_close_function(connection);
    node = connection_list.first;
  }
}

bool ConnectionWithSameConfigPointExists(const EipUint32 config_point) {
  const DoublyLinkedListNode *node = connection_list.first;

  while (NULL != node) {
    const CipConnectionObject *const connection = node->data;
    OPENER_ASSERT(NULL != connection);
    if (config_point == connection->configuration_path.instance_id) {
      return true;
    }
    node = node->next;
  }
  return false;
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
