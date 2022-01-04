/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_ENCAP_H_
#define OPENER_ENCAP_H_

#include "typedefs.h"
#include "cipconnectionobject.h"
#include "generic_networkhandler.h"

/** @file encap.h
 * @brief This file contains the public interface of the encapsulation layer
 */

/**  @defgroup ENCAP OpENer Ethernet encapsulation layer
 * The Ethernet encapsulation layer handles provides the abstraction between the Ethernet and the CIP layer.
 */

/*** defines ***/

#define ENCAPSULATION_HEADER_LENGTH     24

/** @brief definition of status codes in encapsulation protocol
 * All other codes are either legacy codes, or reserved for future use
 *  */
typedef enum {
  kEncapsulationProtocolSuccess = 0x0000,
  kEncapsulationProtocolInvalidCommand = 0x0001,
  kEncapsulationProtocolInsufficientMemory = 0x0002,
  kEncapsulationProtocolIncorrectData = 0x0003,
  kEncapsulationProtocolInvalidSessionHandle = 0x0064,
  kEncapsulationProtocolInvalidLength = 0x0065,
  kEncapsulationProtocolUnsupportedProtocol = 0x0069
} EncapsulationProtocolErrorCode;

/*** structs ***/
typedef struct encapsulation_data {
  CipUint command_code;
  CipUint data_length;
  CipSessionHandle session_handle;
  CipUdint status;
  CipOctet sender_context[8]; /**< length of 8, according to the specification */
  CipUdint options;
  const EipUint8 *communication_buffer_start; /**< Pointer to the communication buffer used for this message */
  const EipUint8 *current_communication_buffer_position; /**< The current position in the communication buffer during the decoding process */
} EncapsulationData;

typedef struct encapsulation_service_information {
  EipUint16 type_code;
  EipUint16 length;
  EipUint16 encapsulation_protocol_version;
  EipUint16 capability_flags;
  EipInt8 name_of_service[16];
} EncapsulationServiceInformation;

/*** global variables (public) ***/

/*** public functions ***/
/** @ingroup ENCAP
 * @brief Initialize the encapsulation layer.
 */
void EncapsulationInit(void);

/** @ingroup ENCAP
 * @brief Shutdown the encapsulation layer.
 *
 * This means that all open sessions including their sockets are closed.
 */
void EncapsulationShutDown(void);

/** @ingroup ENCAP
 * @brief Handle delayed encapsulation message responses
 *
 * Certain encapsulation message requests require a delayed sending of the response
 * message. This functions checks if messages need to be sent and performs the
 * sending.
 */
void ManageEncapsulationMessages(const MilliSeconds elapsed_time);

CipSessionHandle GetSessionFromSocket(const int socket_handle);

void RemoveSession(const int socket);

void CloseSessionBySessionHandle(const CipConnectionObject *const connection_object);

void CloseEncapsulationSessionBySockAddr(const CipConnectionObject *const connection_object);

void CloseClass3ConnectionBasedOnSession(CipSessionHandle encapsulation_session_handle);

/* No reason to use this functions outside the encapsulation layer, they are here for testing */
typedef struct enip_message ENIPMessage;

void EncapsulateListIdentityResponseMessage(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message);

int_fast32_t CreateEncapsulationStructure(const EipUint8 *receive_buffer,
                                          size_t receive_buffer_length,
                                          EncapsulationData *const encapsulation_data);

void SkipEncapsulationHeader(ENIPMessage *const outgoing_message);

void GenerateEncapsulationHeader(const EncapsulationData *const receive_data, const size_t command_specific_data_length, const CipSessionHandle session_handle,
    const EncapsulationProtocolErrorCode encapsulation_protocol_status, ENIPMessage *const outgoing_message);

void HandleReceivedListServicesCommand(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message);

void HandleReceivedListInterfacesCommand(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message);

void HandleReceivedRegisterSessionCommand(int socket, const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message);

EipStatus HandleReceivedSendRequestResponseDataCommand(const EncapsulationData *const receive_data, const struct sockaddr *const originator_address,
    ENIPMessage *const outgoing_message);

#endif /* OPENER_ENCAP_H_ */
