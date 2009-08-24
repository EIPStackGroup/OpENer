/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#ifndef _CPF_H
#define _CPF_H

#include "typedefs.h"
#include "ciptypes.h"

// CPF is Common Packet Format
// CPF packet := <number of items> {<items>}
// item := <TypeID> <Length> <data>
// <number of items> := two bytes
// <TypeID> := two bytes
// <Length> := two bytes
// <data> := <the number of bytes specified by Length>

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

// this one case of a CPF packet is supported:

typedef struct
  {
    EIP_UINT16 ItemCount;
    S_Address_Item stAddr_Item;
    S_Data_Item stDataI_Item;
    S_SockAddrInfo_Item AddrInfo[2];
  } S_CIP_CPF_Data;

int notifyCPF(EIP_UINT8 * pa_nData, // message data
    EIP_INT16 pa_nData_length, // data length
    EIP_UINT8 * pa_replybuf); // reply buffer

int notifyConnectedCPF(EIP_UINT8 * pa_nData, // message data
    EIP_INT16 pa_nData_length, // data length
    EIP_UINT8 * pa_replybuf); // reply buffer


EIP_STATUS createCPFstructure(EIP_UINT8 * pa_Data, int pa_DataLength,
    S_CIP_CPF_Data * pa_CPF_data);

int assembleLinearMsg(S_CIP_MR_Response * pa_MRResponse,
    S_CIP_CPF_Data * pa_CPFDataItem, EIP_UINT8 * pa_msg);

/*!Data storage for the any cpf data
 * Currently we are single threaded and need only one cpf at the time.
 * For future extensions towards multithreading maybe more cpf data items may be necessary
 */
extern S_CIP_CPF_Data g_stCPFDataItem;

#endif
