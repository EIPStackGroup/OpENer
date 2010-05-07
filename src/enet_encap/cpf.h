/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef _CPF_H
#define _CPF_H

#include "typedefs.h"
#include "ciptypes.h"
#include "encap.h"

/*/ CPF is Common Packet Format
 CPF packet := <number of items> {<items>}
 item := <TypeID> <Length> <data>
 <number of items> := two bytes
 <TypeID> := two bytes
 <Length> := two bytes
 <data> := <the number of bytes specified by Length>
 */

/* define Item ID numbers used for address and data items in CPF structures */
#define CIP_ITEM_ID_NULL                                0x0000  /* Null Address Item */
#define CIP_ITEM_ID_LISTIDENTITY_RESPONSE               0x000C
#define CIP_ITEM_ID_CONNECTIONBASED                     0x00A1  /* Connected Address Item */
#define CIP_ITEM_ID_CONNECTIONTRANSPORTPACKET           0x00B1  /* Connected Data Item */
#define CIP_ITEM_ID_UNCONNECTEDMESSAGE                  0x00B2  /* Unconnected Data Item */
#define CIP_ITEM_ID_LISTSERVICE_RESPONSE                0x0100
#define CIP_ITEM_ID_SOCKADDRINFO_O_TO_T                 0x8000  /* Sockaddr info item originator to target (data) */
#define CIP_ITEM_ID_SOCKADDRINFO_T_TO_O                 0x8001  /* Sockaddr info item target to originator (data) */
#define CIP_ITEM_ID_SEQUENCEDADDRESS                    0x8002  /* Sequenced Address item */

typedef struct
{
  EIP_UINT32 ConnectionIdentifier;
  EIP_UINT32 SequenceNumber;
} S_Address_Data;

typedef struct
{
  EIP_UINT16 TypeID;
  EIP_UINT16 Length;
  S_Address_Data Data;
} S_Address_Item;

typedef struct
{
  EIP_UINT16 TypeID;
  EIP_UINT16 Length;
  EIP_UINT8 *Data;
} S_Data_Item;

typedef struct
{
  EIP_UINT16 TypeID;
  EIP_UINT16 Length;
  EIP_INT16 nsin_family;
  EIP_UINT16 nsin_port;
  EIP_UINT32 nsin_addr;
  EIP_UINT8 nasin_zero[8];
} S_SockAddrInfo_Item;

/* this one case of a CPF packet is supported:*/

typedef struct
{
  EIP_UINT16 ItemCount;
  S_Address_Item stAddr_Item;
  S_Data_Item stDataI_Item;
  S_SockAddrInfo_Item AddrInfo[2];
} S_CIP_CPF_Data;

/*! \ingroup ENCAP
 * Parse the CPF data from a received unconnected explicit message and
 * hand the data on to the message router 
 *
 * @param  pa_stReceiveData pointer to the encapsulation structure with the received message
 * @param  pa_acReplyBuf reply buffer
 * @return number of bytes to be sent back. < 0 if nothing should be sent
 */
int notifyCPF(struct S_Encapsulation_Data * pa_stReceiveData,
    EIP_UINT8 * pa_acReplyBuf);

/*! \ingroup ENCAP
 * Parse the CPF data from a received connected explicit message, check
 * the connection status, update any timers, and hand the data on to 
 * the message router 
 *
 * @param  pa_stReceiveData pointer to the encapsulation structure with the received message
 * @param  pa_acReplyBuf reply buffer
 * @return number of bytes to be sent back. < 0 if nothing should be sent
 */
int notifyConnectedCPF(struct S_Encapsulation_Data * pa_stReceiveData,
    EIP_UINT8 * pa_acReplyBuf);

/*! \ingroup ENCAP
 *  Create CPF structure out of the received data.
 *  @param  pa_Data		pointer to data which need to be structured.
 *  @param  pa_DataLength	length of data in pa_Data.
 *  @param  pa_CPF_data	pointer to structure of CPF data item.
 *  @return status
 * 	       EIP_OK .. success
 * 	       EIP_ERROR .. error
 */
EIP_STATUS createCPFstructure(EIP_UINT8 * pa_Data, int pa_DataLength,
    S_CIP_CPF_Data * pa_CPF_data);

/*! \ingroup ENCAP
 * Copy data from MRResponse struct and CPFDataItem into linear memory in pa_msg for transmission over in encapsulation.
 * @param  pa_MRResponse	pointer to message router response which has to be aligned into linear memory.
 * @param  pa_CPFDataItem	pointer to CPF structure which has to be aligned into linear memory.
 * @param  pa_msg		pointer to linear memory.
 * @return length of reply in pa_msg in bytes
 * 	   EIP_ERROR .. error
 */
int assembleLinearMsg(S_CIP_MR_Response * pa_MRResponse,
    S_CIP_CPF_Data * pa_CPFDataItem, EIP_UINT8 * pa_msg);

/*!\ingroup ENCAP 
 * \brief Data storage for the any cpf data
 * Currently we are single threaded and need only one cpf at the time.
 * For future extensions towards multithreading maybe more cpf data items may be necessary
 */
extern S_CIP_CPF_Data g_stCPFDataItem;

#endif
