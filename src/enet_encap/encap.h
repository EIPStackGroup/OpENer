/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#ifndef ENCAP_H_
#define ENCAP_H_

#include "typedefs.h"

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
    // The sender context is not needed any more with the new minumum data copy design
    // EIP_UINT8 anSender_context[SENDER_CONTEXT_SIZE];  
    EIP_UINT32 nOptions;
    EIP_UINT8 *pEncapsulation_Data;
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
extern EIP_UINT8 g_acCommBuf[];   //!<Buffer to be used for communication data (in/out), has to be provided by the platform specific code.

/*** public functions ***/
void encapInit(void);
void closeSession(int pa_nSocket); //!<The remote host closed the connection. Clean up and close the session

#endif /*ENCAP_H_*/
