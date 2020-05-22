/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_CIPCLASS3CONNECTION_H_
#define OPENER_CIPCLASS3CONNECTION_H_

/** @file cipclass3connection.h
 *  @brief CIP Class 3 connection
 *   * Explicit Connection Object State Transition Diagram
 * ----------------------------------------------
 * @dot
 *   digraph ExplicitConnectionObjectStateTransition {
 *     A[label="Any State"]
 *     N[label="Non-existent"]
 *     D[label="Deferred Delete"]
 *     E[label="Established"]
 *
 *     A->N [label="Delete"]
 *     N->E [label="Open Explicit Messaging Connection Response"]
 *     E->N [label="Delete or inactivity time-out"]
 *     E->E [label="Get/Set/Apply Attribute, Reset, Message Produced/Consumed"]
 *     E->D [label="Inactivity time-out and deferred delete set"]
 *     D->N [label="Delete"]
 *   }
 * @enddot
 */

#include "opener_api.h"
#include "cipconnectionmanager.h"
#include "cipconnectionobject.h"

typedef EipStatus (*CipConnectionStateHandler)(CipConnectionObject *RESTRICT
                                               const connection_object,
                                               ConnectionObjectState new_state);

EipStatus CipClass3ConnectionObjectStateEstablishedHandler(
  CipConnectionObject *RESTRICT const connection_object,
  ConnectionObjectState new_state);

/** @brief Check if Class3 connection is available and if yes setup all data.
 *
 * This function can be called after all data has been parsed from the forward open request
 * @param connection_object pointer to the connection object structure holding the parsed data from the forward open request
 * @param extended_error the extended error code in case an error happened
 * @return general status on the establishment
 *    - kEipStatusOk ... on success
 *    - On an error the general status code to be put into the response
 */
CipError EstablishClass3Connection(
  CipConnectionObject *RESTRICT const connection_object,
  EipUint16 *const extended_error);

/** @brief Initializes the explicit connections mechanism
 *
 *  Prepares the available explicit connection slots for use at the start of the OpENer
 */
void InitializeClass3ConnectionData(void);

#endif /* OPENER_CIPCLASS3CONNECTION_H_ */
