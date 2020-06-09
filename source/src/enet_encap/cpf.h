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
 * CPF packet := \<number of items\> {\<items\>}
 * item := \<TypeID\> \<Length\> \<data\>
 * \<number of items\> := two bytes
 * \<TypeID\> := two bytes
 * \<Length\> := two bytes
 * \<data\> := \<the number of bytes specified by Length\>
 */

/** @brief Definition of Item ID numbers used for address and data items in CPF structures */
typedef enum {
  kCipItemIdNullAddress = 0x0000, /**< Type: Address; Indicates that encapsulation routing is not needed. */
  kCipItemIdListIdentityResponse = 0x000C,
  kCipItemIdConnectionAddress = 0x00A1, /**< Type: Address; Connection-based, used for connected messages, see Vol.2, p.42 */
  kCipItemIdConnectedDataItem = 0x00B1, /**< Type: Data; Connected data item, see Vol.2, p.43 */
  kCipItemIdUnconnectedDataItem = 0x00B2, /**< Type: Data; Unconnected message */
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
  CipUint type_id;
  CipUint length;
  AddressData data;
} AddressItem;

typedef struct {
  EipUint16 type_id;
  EipUint16 length;
  EipUint8 *data;
} DataItem;

/** @brief CPF Sockaddr Item
 *
 */
typedef struct {
  CipUint type_id; /**< Either 0x8000 for O->T or 0x8001 for T->O */
  CipUint length; /**< Length shall be 16 bytes */
  CipInt sin_family; /**< Shall be AF_INET = 2 in big endian order */
  CipUint sin_port; /**< For point-to-point connection this shall be set to the used UDP port (recommended port = 0x08AE). For multicast this shall be set to 0x08AE and treated by the receiver as don't care. Big endian order */
  CipUdint sin_addr; /**< For multicast connections shall be set to the multicast address. For point-to-point shall be treated as don't care, recommended value 0. Big endian order. */
  CipUsint nasin_zero[8]; /**< Length of 8, Recommended value zero */
} SocketAddressInfoItem;

/* this one case of a CPF packet is supported:*/
/** @brief A variant of a CPF packet, including item count, one address item, one data item, and two Sockaddr Info items */
typedef struct {
  EipUint16 item_count; /**< Up to four for this structure allowed */
  AddressItem address_item;
  DataItem data_item;
  SocketAddressInfoItem address_info_item[2];
} CipCommonPacketFormatData;

/** @ingroup ENCAP
 * Parse the CPF data from a received unconnected explicit message and
 * hand the data on to the message router
 *
 * @param received_data pointer to the encapsulation structure with the received message
 * @param originator_address Address struct of the originator
 * @param outgoing_message The outgoing ENIP message struct
 * @return kEipStatusOkSend: a response needs to be sent, others: EIP stack status
 */
EipStatus NotifyCommonPacketFormat(
  const EncapsulationData *const received_data,
  const struct sockaddr *const originator_address,
  ENIPMessage *const outgoing_message);

/** @ingroup ENCAP
 * Parse the CPF data from a received connected explicit message, check
 * the connection status, update any timers, and hand the data on to
 * the message router
 *
 * @param received_data pointer to the encapsulation structure with the received message
 * @param originator_address Address struct of the originator
 * @param outgoing_message The outgoing ENIP message struct
 * @return kEipStatusOkSend: a response needs to be sent, others: EIP stack status
 */
EipStatus NotifyConnectedCommonPacketFormat(
  const EncapsulationData *const received_data,
  const struct sockaddr *const originator_address,
  ENIPMessage *const outgoing_message);

/** @ingroup ENCAP
 *  Create CPF structure out of the received data.
 *  @param  data		pointer to data which need to be structured.
 *  @param  data_length	length of data in pa_Data.
 *  @param  common_packet_format_data	pointer to structure of CPF data item.
 *  @return status
 *             EIP_OK .. success
 *             EIP_ERROR .. error
 */
EipStatus CreateCommonPacketFormatStructure(
  const EipUint8 *data,
  size_t data_length,
  CipCommonPacketFormatData *common_packet_format_data);

/** @ingroup ENCAP
 * Copy data from CPFDataItem into linear memory in message for transmission over in encapsulation.
 * @param  common_packet_format_data_item pointer to CPF structure which has to be aligned into linear memory.
 * @param  message Modified ENIPMessage struct
 * @return length of modification in bytes
 *     kEipStatusError .. error
 */
void AssembleIOMessage(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const message);


/** @ingroup ENCAP
 * @brief Copy data from message_router_response struct and common_packet_format_data_item into
 * ENIPMessage struct outgoing_message via encapsulation.
 *
 * @param  message_router_response	pointer to message router response which has to be aligned into linear memory.
 * @param  common_packet_format_data_item	pointer to CPF structure which has to be aligned into linear memory.
 * @param  outgoing_message Modified ENIPMessage struct
 * @return length of modification in bytes
 *         kEipStatusError .. error
 */
EipStatus AssembleLinearMessage(
  const CipMessageRouterResponse *const message_router_response,
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message);

/** @ingroup ENCAP
 * @brief Data storage for the any CPF data
 * Currently we are single threaded and need only one CPF at the time.
 * For future extensions towards multithreading maybe more CPF data items may be necessary
 */
extern CipCommonPacketFormatData g_common_packet_format_data_item;

#endif /* OPENER_CPF_H_ */
