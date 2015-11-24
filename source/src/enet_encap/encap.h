/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef OPENER_ENCAP_H_
#define OPENER_ENCAP_H_

#include "typedefs.h"

/** @file encap.h
 * @brief This file contains the public interface of the encapsulation layer
 */

/**  @defgroup ENCAP OpENer Ethernet encapsulation layer
 * The Ethernet encapsulation layer handles provides the abstraction between the Ethernet and the CIP layer.
 */

/*** defines ***/

#define ENCAPSULATION_HEADER_LENGTH	24

static const int kOpenerEthernetPort = 0xAF12;

/* definition of status codes in encapsulation protocol */
typedef enum {
  kEncapsulationProtocolSuccess = 0x0000,
  kEncapsulationProtocolInvalidCommand = 0x0001,
  kEncapsulationProtocolInsufficientMemory = 0x0002,
  kEncapsulationProtocolIncorrectData = 0x0003,
  kEncapsulationProtocolInvalidSessionHandle = 0x0064,
  kEncapsulationProtocolInvalidLength = 0x0065,
  kEncapsulationProtocolUnsupportedProtocol = 0x0069
} EncapsulationProtocolStatus;

/*** structs ***/
typedef struct encapsulation_data {
  EipUint16 command_code;
  EipUint16 data_length;
  EipUint32 session_handle;
  EipUint32 status;
  EipUint32 options;
  EipUint8 *communication_buffer_start; /**< Pointer to the communication buffer used for this message */
  EipUint8 *current_communication_buffer_position; /**< The current position in the communication buffer during the decoding process */
} EncapsulationData;

typedef struct encapsulation_interface_information {
  EipUint16 type_code;
  EipUint16 length;
  EipUint16 encapsulation_protocol_version;
  EipUint16 capability_flags;
  EipInt8 name_of_service[16];
} EncapsulationInterfaceInformation;

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
void ManageEncapsulationMessages(void);

#endif /* OPENER_ENCAP_H_ */
