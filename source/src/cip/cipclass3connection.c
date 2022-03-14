/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>

#include "cipclass3connection.h"

#include "encap.h"

/**** Global variables ****/
extern CipConnectionObject explicit_connection_object_pool[
  OPENER_CIP_NUM_EXPLICIT_CONNS];

CipConnectionObject *GetFreeExplicitConnection(void);

void Class3ConnectionTimeoutHandler(CipConnectionObject *connection_object) {
  CheckForTimedOutConnectionsAndCloseTCPConnections(connection_object,
                                                    CloseSessionBySessionHandle);
  CloseConnection(connection_object);
}

/**** Implementation ****/
CipError EstablishClass3Connection(
  CipConnectionObject *RESTRICT const connection_object,
  EipUint16 *const extended_error) {
  CipError cip_error = kCipErrorSuccess;

  CipConnectionObject *explicit_connection = GetFreeExplicitConnection();

  if (NULL == explicit_connection) {
    cip_error = kCipErrorConnectionFailure;
    *extended_error =
      kConnectionManagerExtendedStatusCodeErrorNoMoreConnectionsAvailable;
  } else {
    ConnectionObjectDeepCopy(explicit_connection, connection_object);

    ConnectionObjectGeneralConfiguration(explicit_connection);

    ConnectionObjectSetInstanceType(explicit_connection,
                                    kConnectionObjectInstanceTypeExplicitMessaging);

    /* set the connection call backs */
    explicit_connection->connection_close_function =
      CloseConnection;
    /* explicit connection have to be closed on time out*/
    explicit_connection->connection_timeout_function =
      Class3ConnectionTimeoutHandler;

    AddNewActiveConnection(explicit_connection);
  }
  return cip_error;
}

/** @brief Searches and returns a free explicit connection slot
 *
 * @return Free explicit connection slot, or NULL if no slot is free
 */
CipConnectionObject *GetFreeExplicitConnection(void) {
  for (size_t i = 0; i < OPENER_CIP_NUM_EXPLICIT_CONNS; ++i) {
    if (ConnectionObjectGetState(&(explicit_connection_object_pool[i]) ) ==
        kConnectionObjectStateNonExistent) {
      return &(explicit_connection_object_pool[i]);
    }
  }
  return NULL;
}

void InitializeClass3ConnectionData(void) {
  memset( explicit_connection_object_pool, 0,
          OPENER_CIP_NUM_EXPLICIT_CONNS * sizeof(CipConnectionObject) );
}

EipStatus CipClass3ConnectionObjectStateEstablishedHandler(
  CipConnectionObject *RESTRICT const connection_object,
  ConnectionObjectState new_state) {
  switch(new_state) {
    case kConnectionObjectStateNonExistent:
      ConnectionObjectInitializeEmpty(connection_object);
      ConnectionObjectSetState(connection_object, new_state);
      return kEipStatusOk;
    default: return kEipStatusError;
  }
}

EipStatus CipClass3ConnectionObjectStateNonExistentHandler(
  CipConnectionObject *RESTRICT const connection_object,
  ConnectionObjectState new_state) {
  switch(new_state) {
    case kConnectionObjectStateEstablished:
      ConnectionObjectSetState(connection_object, new_state);
      return kEipStatusOk;
    default: return kEipStatusError;
  }
}
