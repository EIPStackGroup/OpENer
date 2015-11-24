/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef OPENER_CPF_H_
#define OPENER_CPF_H_

#include "typedefs.h"
#include "ciptypes.h"
#include "encap.h"

/** @ingroup ENCAP
 * @brief CPF is Common Packet Format
 * CPF packet := <number of items> {<items>}
 * item := <TypeID> <Length> <data>
 * <number of items> := two bytes
 * <TypeID> := two bytes
 * <Length> := two bytes
 * <data> := <the number of bytes specified by Length>
 */

/** @brief Definition of Item ID numbers used for address and data items in CPF structures */
typedef enum {
  kCipItemIdNullAddress = 0x0000, /**< Type: Address; Indicates that encapsulation routing is not needed. */
  kCipItemIdListIdentityResponse = 0x000C,
  kCipItemIdConnectionBased = 0x00A1, /**< Type: Address; Connection-based, used for connected messages */
  kCipItemIdConnectedTransportPacket = 0x00B1, /**< Type: Data; Connected Transport packet */
  kCipItemIdUnconnectedMessage = 0x00B2, /**< Type: Data; Unconnected message */
  kCipItemIdListServiceResponse = 0x0100,
  kCipItemIdSocketAddressInfoOriginatorToTarget = 0x8000, /**< Type: Data; Sockaddr info item originator to target */
  kCipItemIdSocketAddressInfoTargetToOriginator = 0x8001, /**< Type: Data; Sockaddr info item target to originator */
  kCipItemIdSequencedAddressItem = 0x8002 /**< Sequenced Address item */
} CipItemId;

typedef struct {
  EipUint32 connection_identifier;
  EipUint32 sequence_number;
} AddressData;

typedef struct {
  EipUint16 type_id;
  EipUint16 length;
  AddressData data;
} AddressItem;

typedef struct {
  EipUint16 type_id;
  EipUint16 length;
  EipUint8 *data;
} S_Data_Item;

typedef struct {
  EipUint16 type_id;
  EipUint16 length;
  EipInt16 nsin_family;
  EipUint16 nsin_port;
  EipUint32 nsin_addr;
  EipUint8 nasin_zero[8];
} SocketAddrressInfoItem;

/* this one case of a CPF packet is supported:*/

typedef struct {
  EipUint16 item_count;
  AddressItem address_item;
  S_Data_Item data_item;
  SocketAddrressInfoItem address_info_item[2];
} CipCommonPacketFormatData;

/** @ingroup ENCAP
 * Parse the CPF data from a received unconnected explicit message and
 * hand the data on to the message router 
 *
 * @param  received_data pointer to the encapsulation structure with the received message
 * @param  reply_buffer reply buffer
 * @return number of bytes to be sent back. < 0 if nothing should be sent
 */
int NotifyCommonPacketFormat(EncapsulationData *received_data,
                             EipUint8 *reply_buffer);

/** @ingroup ENCAP
 * Parse the CPF data from a received connected explicit message, check
 * the connection status, update any timers, and hand the data on to 
 * the message router 
 *
 * @param  received_data pointer to the encapsulation structure with the received message
 * @param  reply_buffer reply buffer
 * @return number of bytes to be sent back. < 0 if nothing should be sent
 */
int NotifyConnectedCommonPacketFormat(EncapsulationData *received_data,
                                      EipUint8 *reply_buffer);

/** @ingroup ENCAP
 *  Create CPF structure out of the received data.
 *  @param  data		pointer to data which need to be structured.
 *  @param  data_length	length of data in pa_Data.
 *  @param  common_packet_format_data	pointer to structure of CPF data item.
 *  @return status
 * 	       EIP_OK .. success
 * 	       EIP_ERROR .. error
 */
EipStatus CreateCommonPacketFormatStructure(
    EipUint8 *data, int data_length,
    CipCommonPacketFormatData *common_packet_format_data);

/** @ingroup ENCAP
 * Copy data from MRResponse struct and CPFDataItem into linear memory in pa_msg for transmission over in encapsulation.
 * @param  message_router_response	pointer to message router response which has to be aligned into linear memory.
 * @param  common_packet_format_data_item	pointer to CPF structure which has to be aligned into linear memory.
 * @param  message		pointer to linear memory.
 * @return length of reply in pa_msg in bytes
 * 	   EIP_ERROR .. error
 */
int AssembleLinearMessage(
    CipMessageRouterResponse *message_router_response,
    CipCommonPacketFormatData *common_packet_format_data_item,
    EipUint8 *message);

/** @ingroup ENCAP
 * @brief Data storage for the any CPF data
 * Currently we are single threaded and need only one CPF at the time.
 * For future extensions towards multithreading maybe more CPF data items may be necessary
 */
extern CipCommonPacketFormatData g_common_packet_format_data_item;

#endif /* OPENER_CPF_H_ */
