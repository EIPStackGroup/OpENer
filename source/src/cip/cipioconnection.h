/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/**
 * @file cipioconnection.h
 * CIP I/O Connection implementation
 * =================================
 *
 *
 * I/O Connection Object State Transition Diagram
 * ----------------------------------------------
 * @dot
 *   digraph IOConnectionObjectStateTransition {
 *     A[label="Any State"]
 *     N[label="Non-existent"]
 *     C[label="Configuring"]
 *     E[label="Established"]
 *     W[label="Waiting for Connection ID"]
 *     T[label="Timed Out"]
 *
 *     A->N [label="Delete"]
 *     N->C [label="Create"]
 *     C->C [label="Get/Set/Apply Attribute"]
 *     C->W [label="Apply Attribute"]
 *     W->W [label="Get/Set Attribute"]
 *     C->E [label="Apply Attribute"]
 *     E->E [label="Get/Set/Apply Attribute, Reset, Message Produced/Consumed"]
 *     W->E [label="Apply Attribute"]
 *     E->T [label="Inactivity/Watchdog"]
 *     T->E [label="Reset"]
 *     T->N [label="Delete"]
 *   }
 * @enddot
 *
 */

#ifndef OPENER_CIPIOCONNECTION_H_
#define OPENER_CIPIOCONNECTION_H_

#include "opener_api.h"
#include "cipconnectionmanager.h"
#include "cipconnectionobject.h"

/** @brief Setup all data in order to establish an IO connection
 *
 * This function can be called after all data has been parsed from the forward open request
 * @param connection_object pointer to the connection object structure holding the parsed data from the forward open request
 * @param extended_error the extended error code in case an error happened
 * @return general status on the establishment
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
CipError EstablishIoConnection(
  CipConnectionObject *RESTRICT const connection_object,
  EipUint16 *const extended_error);

/** @brief Take the data given in the connection object structure and open the necessary communication channels
 *
 * This function will use the g_stCPFDataItem!
 * @param connection_object pointer to the connection object data
 * @return general status on the open process
 *    - EIP_OK ... on success
 *    - On an error the general status code to be put into the response
 */
CipError OpenCommunicationChannels(CipConnectionObject *connection_object);

/** @brief close the communication channels of the given connection and remove it
 * from the active connections list.
 *
 * @param connection_object pointer to the connection object data
 */
void CloseCommunicationChannelsAndRemoveFromActiveConnectionsList(
  CipConnectionObject *connection_object);

extern EipUint8 *g_config_data_buffer;
extern unsigned int g_config_data_length;

#endif /* OPENER_CIPIOCONNECTION_H_ */
