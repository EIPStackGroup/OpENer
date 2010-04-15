/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef ENCAP_H_
#define ENCAP_H_

#include "typedefs.h"


/**  \defgroup ENCAP OpENer Ethernet encapsulation layer
 * The Ethernet encapsulation layer handles provides the abstraction between the Ethernet and the CIP layer.
 */

/*** defines ***/

#define ENCAPSULATION_HEADER_LENGTH 24
#define OPENER_ETHERNET_PORT 0xAF12


/*** structs ***/
struct S_Encapsulation_Data
  {
    EIP_UINT16 nCommand_code;
    EIP_UINT16 nData_length;
    EIP_UINT32 nSession_handle;
    EIP_UINT32 nStatus;
    /* The sender context is not needed any more with the new minimum data copy design */
    /* EIP_UINT8 anSender_context[SENDER_CONTEXT_SIZE];  */
    EIP_UINT32 nOptions;
    EIP_UINT8 *m_acCommBufferStart;       /*Pointer to the communication buffer used for this message */
    EIP_UINT8 *m_acCurrentCommBufferPos;  /*The current position in the communication buffer during the decoding process */
  };

struct S_Encapsulation_Interface_Information
  {
    EIP_UINT16 TypeCode;
    EIP_UINT16 Length;
    EIP_UINT16 EncapsulationProtocolVersion;
    EIP_UINT16 CapabilityFlags;
    EIP_INT8 NameofService[16];
  };

/*** global variables (public) ***/

/*** public functions ***/
/*! \ingroup ENCAP 
 * \brief Initialize the encapsulation layer.
 */
void encapInit(void);

/*! \ingroup ENCAP
 * \brief Shutdown the encapsulation layer.
 *
 * This means that all open sessions including their sockets are closed.
 */
void encapShutDown(void);

#endif /*ENCAP_H_*/
