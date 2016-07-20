/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <string.h>

#include "cpf.h"

#include "opener_api.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "endianconv.h"
#include "ciperror.h"
#include "cipconnectionmanager.h"
#include "trace.h"

const size_t sequenced_address_item_length = 8;

CipCommonPacketFormatData g_common_packet_format_data_item; /**< CPF global data items */

int NotifyCommonPacketFormat(EncapsulationData *receive_data,
                             EipUint8 *reply_buffer) {
  int return_value = kEipStatusError;

  if ((return_value = CreateCommonPacketFormatStructure(
      receive_data->current_communication_buffer_position,
      receive_data->data_length, &g_common_packet_format_data_item))
      == kEipStatusError) {
    OPENER_TRACE_ERR("notifyCPF: error from createCPFstructure\n");
  } else {
    return_value = kEipStatusOk; /* In cases of errors we normally need to send an error response */
    if (g_common_packet_format_data_item.address_item.type_id
        == kCipItemIdNullAddress) /* check if NullAddressItem received, otherwise it is no unconnected message and should not be here*/
        { /* found null address item*/
      if (g_common_packet_format_data_item.data_item.type_id
          == kCipItemIdUnconnectedDataItem) { /* unconnected data item received*/
        return_value = NotifyMR(
            g_common_packet_format_data_item.data_item.data,
            g_common_packet_format_data_item.data_item.length);
        if (return_value != kEipStatusError) {
          return_value = AssembleLinearMessage(
              &g_message_router_response, &g_common_packet_format_data_item,
              reply_buffer);
        }
      } else {
        /* wrong data item detected*/
        OPENER_TRACE_ERR(
            "notifyCPF: got something besides the expected CIP_ITEM_ID_UNCONNECTEDMESSAGE\n");
        receive_data->status = kEncapsulationProtocolIncorrectData;
      }
    } else {
      OPENER_TRACE_ERR(
          "notifyCPF: got something besides the expected CIP_ITEM_ID_NULL\n");
      receive_data->status = kEncapsulationProtocolIncorrectData;
    }
  }
  return return_value;
}

int NotifyConnectedCommonPacketFormat(EncapsulationData *received_data,
                                      EipUint8 *reply_buffer) {

  int return_value = CreateCommonPacketFormatStructure(
      received_data->current_communication_buffer_position,
      received_data->data_length, &g_common_packet_format_data_item);

  if (kEipStatusError == return_value) {
    OPENER_TRACE_ERR("notifyConnectedCPF: error from createCPFstructure\n");
  } else {
    return_value = kEipStatusError; /* For connected explicit messages status always has to be 0*/
    if (g_common_packet_format_data_item.address_item.type_id
        == kCipItemIdConnectionAddress) /* check if ConnectedAddressItem received, otherwise it is no connected message and should not be here*/
        { /* ConnectedAddressItem item */
      ConnectionObject *connection_object = GetConnectedObject(
          g_common_packet_format_data_item.address_item.data
              .connection_identifier);
      if (NULL != connection_object) {
        /* reset the watchdog timer */
        connection_object->inactivity_watchdog_timer = (connection_object
            ->o_to_t_requested_packet_interval / 1000)
            << (2 + connection_object->connection_timeout_multiplier);

        /*TODO check connection id  and sequence count    */
        if (g_common_packet_format_data_item.data_item.type_id
            == kCipItemIdConnectedDataItem) { /* connected data item received*/
          EipUint8 *pnBuf = g_common_packet_format_data_item.data_item.data;
          g_common_packet_format_data_item.address_item.data.sequence_number =
              (EipUint32) GetIntFromMessage(&pnBuf);
          return_value = NotifyMR(
              pnBuf, g_common_packet_format_data_item.data_item.length - 2);

          if (return_value != kEipStatusError) {
            g_common_packet_format_data_item.address_item.data
                .connection_identifier = connection_object
                ->produced_connection_id;
            return_value = AssembleLinearMessage(
                &g_message_router_response, &g_common_packet_format_data_item,
                reply_buffer);
          }
        } else {
          /* wrong data item detected*/
          OPENER_TRACE_ERR(
              "notifyConnectedCPF: got something besides the expected CIP_ITEM_ID_UNCONNECTEDMESSAGE\n");
        }
      } else {
        OPENER_TRACE_ERR(
            "notifyConnectedCPF: connection with given ID could not be found\n");
      }
    } else {
      OPENER_TRACE_ERR(
          "notifyConnectedCPF: got something besides the expected CIP_ITEM_ID_NULL\n");
    }
  }
  return return_value;
}

/**
 * @brief Creates Common Packet Format structure out of data.
 * @param data Pointer to data which need to be structured.
 * @param data_length	Length of data in pa_Data.
 * @param common_packet_format_data	Pointer to structure of CPF data item.
 *
 *   @return kEipStatusOk .. success
 * 	       kEipStatusError .. error
 */
EipStatus CreateCommonPacketFormatStructure(
    EipUint8 *data, int data_length,
    CipCommonPacketFormatData *common_packet_format_data) {

  common_packet_format_data->address_info_item[0].type_id = 0;
  common_packet_format_data->address_info_item[1].type_id = 0;

  int length_count = 0;
  common_packet_format_data->item_count = GetIntFromMessage(&data);
  length_count += 2;
  if (common_packet_format_data->item_count >= 1) {
    common_packet_format_data->address_item.type_id = GetIntFromMessage(&data);
    common_packet_format_data->address_item.length = GetIntFromMessage(&data);
    length_count += 4;
    if (common_packet_format_data->address_item.length >= 4) {
      common_packet_format_data->address_item.data.connection_identifier =
          GetDintFromMessage(&data);
      length_count += 4;
    }
    if (common_packet_format_data->address_item.length == 8) {
      common_packet_format_data->address_item.data.sequence_number =
          GetDintFromMessage(&data);
      length_count += 4;
    }
  }
  if (common_packet_format_data->item_count >= 2) {
    common_packet_format_data->data_item.type_id = GetIntFromMessage(&data);
    common_packet_format_data->data_item.length = GetIntFromMessage(&data);
    common_packet_format_data->data_item.data = data;
    data += common_packet_format_data->data_item.length;
    length_count += (4 + common_packet_format_data->data_item.length);
  }
  for (int j = 0; j < (common_packet_format_data->item_count - 2); j++) /* TODO there needs to be a limit check here???*/
  {
    common_packet_format_data->address_info_item[j].type_id = GetIntFromMessage(
        &data);
    length_count += 2;
    if ((common_packet_format_data->address_info_item[j].type_id
        == kCipItemIdSocketAddressInfoOriginatorToTarget)
        || (common_packet_format_data->address_info_item[j].type_id
            == kCipItemIdSocketAddressInfoTargetToOriginator)) {
      common_packet_format_data->address_info_item[j].length =
          GetIntFromMessage(&data);
      common_packet_format_data->address_info_item[j].sin_family =
          GetIntFromMessage(&data);
      common_packet_format_data->address_info_item[j].sin_port =
          GetIntFromMessage(&data);
      common_packet_format_data->address_info_item[j].sin_addr =
          GetDintFromMessage(&data);
      for (int i = 0; i < 8; i++) {
        common_packet_format_data->address_info_item[j].nasin_zero[i] = *data;
        data++;
      }
      length_count += 18;
    } else { /* no sockaddr item found */
      common_packet_format_data->address_info_item[j].type_id = 0; /* mark as not set */
      data -= 2;
    }
  }
  /* set the addressInfoItems to not set if they were not received */
  if (common_packet_format_data->item_count < 4) {
    common_packet_format_data->address_info_item[1].type_id = 0;
    if (common_packet_format_data->item_count < 3) {
      common_packet_format_data->address_info_item[0].type_id = 0;
    }
  }
  if (length_count == data_length) { /* length of data is equal to length of Addr and length of Data */
    return kEipStatusOk;
  } else {
    OPENER_TRACE_WARN(
        "something is wrong with the length in Message Router @ CreateCommonPacketFormatStructure\n");
    if (common_packet_format_data->item_count > 2) {
      /* there is an optional packet in data stream which is not sockaddr item */
      return kEipStatusOk;
    } else { /* something with the length was wrong */
      return kEipStatusError;
    }
  }
}

/* null address item -> address length set to 0 */
/**
 * Encodes a Null Address Item into the message frame
 * @param message The message frame
 * @param size The actual size of the message frame
 *
 * @return The new size of the message frame after encoding
 */
int EncodeNullAddressItem(EipUint8** message, int size) {
  /* null address item -> address length set to 0 */
  size += AddIntToMessage(kCipItemIdNullAddress, message);
  size += AddIntToMessage(0, message);
  return size;
}

/* connected data item -> address length set to 4 and copy ConnectionIdentifier */
/**
 * Encodes a Connected Address Item into the message frame
 * @param message The message frame
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param size The actual size of the message frame
 *
 * @return The new size of the message frame after encoding
 */
int EncodeConnectedAddressItem(
    EipUint8** message,
    CipCommonPacketFormatData* common_packet_format_data_item, int size) {
  /* connected data item -> address length set to 4 and copy ConnectionIdentifier */
  size += AddIntToMessage(kCipItemIdConnectionAddress, message);
  size += AddIntToMessage(4, message);
  size += AddDintToMessage(
      common_packet_format_data_item->address_item.data.connection_identifier,
      message);
  return size;
}

/**
 * @brief Encodes a sequenced address item into the message
 *
 * @param message Pointer to the message memory start
 * @param common_packet_format_data_item Common Packet Format item which is used in the encoding
 * @param size Size of the message at the start of the function
 *
 * @return New message size after encoding
 */
int EncodeSequencedAddressItem(
    EipUint8** message,
    CipCommonPacketFormatData* common_packet_format_data_item, int size) {
  /* sequenced address item -> address length set to 8 and copy ConnectionIdentifier and SequenceNumber */
  size += AddIntToMessage(kCipItemIdSequencedAddressItem, message);
  size += AddIntToMessage(sequenced_address_item_length, message);
  size += AddDintToMessage(
      common_packet_format_data_item->address_item.data.connection_identifier,
      message);
  size += AddDintToMessage(
      common_packet_format_data_item->address_item.data.sequence_number,
      message);
  return size;
}

/**
 * Adds the item count to the message frame
 *
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param message The message frame
 * @param size The actual size of the message frame
 *
 * @return The new size of the message frame after encoding
 */
int EncodeItemCount(CipCommonPacketFormatData* common_packet_format_data_item,
                    EipUint8** message, int size) {
  size += AddIntToMessage(common_packet_format_data_item->item_count, message); /* item count */
  return size;
}

/**
 * Adds the data item type to the message frame
 *
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param message The message frame
 * @param size The actual size of the message frame
 *
 * @return The new size of the message frame after encoding
 */
int EncodeDataItemType(
    CipCommonPacketFormatData* common_packet_format_data_item,
    EipUint8** message, int size) {
  size += AddIntToMessage(common_packet_format_data_item->data_item.type_id,
                          message);
  return size;
}

/**
 * Adds the data item section length to the message frame
 *
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param message The message frame
 * @param size The actual size of the message frame
 *
 * @return The new size of the message frame after encoding
 */
int EncodeDataItemLength(
    CipCommonPacketFormatData* common_packet_format_data_item,
    EipUint8** message, int size) {
  size += AddIntToMessage(common_packet_format_data_item->data_item.length,
                          message);
  return size;
}

/**
 * Adds the data items to the message frame
 *
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param message The message frame
 * @param size The actual size of the message frame
 *
 * @return The new size of the message frame after encoding
 */
int EncodeDataItemData(
    CipCommonPacketFormatData* common_packet_format_data_item,
    EipUint8** message, int size) {
  for (int i = 0; i < common_packet_format_data_item->data_item.length; i++) {
    size += AddSintToMessage(
        *(common_packet_format_data_item->data_item.data + i), message);
  }
  return size;
}

int EncodeConnectedDataItemLength(
    CipMessageRouterResponse* message_router_response, EipUint8** message,
    int size) {
  size += AddIntToMessage(
      (EipUint16) (message_router_response->data_length + 4 + 2
          + (2 * message_router_response->size_of_additional_status)),
      message);
  return size;
}

int EncodeSequenceNumber(
    int size, const CipCommonPacketFormatData* common_packet_format_data_item,
    EipUint8** message) {
  /* 2 bytes*/
  size += AddIntToMessage(
      (EipUint16) common_packet_format_data_item->address_item.data
          .sequence_number,
      message);
  return size;
}

int EncodeReplyService(int size, EipUint8** message,
                       CipMessageRouterResponse* message_router_response) {
  size += AddSintToMessage(message_router_response->reply_service, message);
  return size;
}

int EnocdeReservedFieldOfLengthByte(
    int size, EipUint8** message,
    CipMessageRouterResponse* message_router_response) {
  size += AddSintToMessage(message_router_response->reserved, message);
  return size;
}

int EncodeGeneralStatus(int size, EipUint8** message,
                        CipMessageRouterResponse* message_router_response) {
  size += AddSintToMessage(message_router_response->general_status, message);
  return size;
}

int EncodeExtendedStatusLength(
    int size, EipUint8** message,
    CipMessageRouterResponse* message_router_response) {
  size += AddSintToMessage(message_router_response->size_of_additional_status,
                           message);
  return size;
}

int EncodeExtendedStatusDataItems(
    int size, CipMessageRouterResponse* message_router_response,
    EipUint8** message) {
  for (int i = 0; i < message_router_response->size_of_additional_status; i++)
    size += AddIntToMessage(message_router_response->additional_status[i],
                            message);

  return size;
}

int EncodeExtendedStatus(int size, EipUint8** message,
                         CipMessageRouterResponse* message_router_response) {
  size = EncodeExtendedStatusLength(size, message, message_router_response);
  size = EncodeExtendedStatusDataItems(size, message_router_response, message);

  return size;
}

int EncodeUnconnectedDataItemLength(
    int size, CipMessageRouterResponse* message_router_response,
    EipUint8** message) {
  /* Unconnected Item */
  size += AddIntToMessage(
      (EipUint16) (message_router_response->data_length + 4
          + (2 * message_router_response->size_of_additional_status)),
      message);
  return size;
}

int EncodeMessageRouterResponseData(
    int size, CipMessageRouterResponse* message_router_response,
    EipUint8** message) {
  for (int i = 0; i < message_router_response->data_length; i++) {
    size += AddSintToMessage((message_router_response->data)[i], &*message);
  }
  return size;
}

int EncodeSockaddrInfoItemTypeId(
    int size, int item_type,
    CipCommonPacketFormatData* common_packet_format_data_item,
    EipUint8** message) {
  OPENER_ASSERT(item_type == 0 || item_type == 1);
  size += AddIntToMessage(
      common_packet_format_data_item->address_info_item[item_type].type_id,
      message);

  return size;
}

int EncodeSockaddrInfoLength(
    int size, int j, CipCommonPacketFormatData* common_packet_format_data_item,
    EipUint8** message) {
  size += AddIntToMessage(
      common_packet_format_data_item->address_info_item[j].length, message);
  return size;
}

/** @brief Copy data from message_router_response struct and common_packet_format_data_item into linear memory in
 * pa_msg for transmission over in encapsulation.
 *
 * @param message_router_response	pointer to message router response which has to be aligned into linear memory.
 * @param common_packet_format_data_item pointer to CPF structure which has to be aligned into linear memory.
 * @param message		pointer to linear memory.
 *  @return length of reply in message in bytes
 * 			-1 .. error
 */
int AssembleLinearMessage(
    CipMessageRouterResponse *message_router_response,
    CipCommonPacketFormatData *common_packet_format_data_item,
    EipUint8 *message) {

  int message_size = 0;

  if (message_router_response) {
    /* add Interface Handle and Timeout = 0 -> only for SendRRData and SendUnitData necessary */
    AddDintToMessage(0, &message);
    AddIntToMessage(0, &message);
    message_size += 6;
  }

  message_size = EncodeItemCount(common_packet_format_data_item, &message,
                                 message_size);

  /* process Address Item */
  switch (common_packet_format_data_item->address_item.type_id) {
    case kCipItemIdNullAddress: {
      message_size = EncodeNullAddressItem(&message, message_size);
      break;
    }
    case kCipItemIdConnectionAddress: {
      message_size = EncodeConnectedAddressItem(&message,
                                                common_packet_format_data_item,
                                                message_size);
      break;
    }
    case kCipItemIdSequencedAddressItem: {
      message_size = EncodeSequencedAddressItem(&message,
                                                common_packet_format_data_item,
                                                message_size);
      break;
    }
  }

  /* process Data Item */
  if ((common_packet_format_data_item->data_item.type_id
      == kCipItemIdUnconnectedDataItem)
      || (common_packet_format_data_item->data_item.type_id
          == kCipItemIdConnectedDataItem)) {

    if (message_router_response) {
      message_size = EncodeDataItemType(common_packet_format_data_item,
                                         &message, message_size);

      if (common_packet_format_data_item->data_item.type_id
          == kCipItemIdConnectedDataItem) { /* Connected Item */
        message_size = EncodeConnectedDataItemLength(message_router_response,
                                                     &message, message_size);
        message_size = EncodeSequenceNumber(message_size,
                                            &g_common_packet_format_data_item,
                                            &message);

      } else { /* Unconnected Item */
        message_size = EncodeUnconnectedDataItemLength(message_size,
                                                       message_router_response,
                                                       &message);
      }

      /* write message router response into linear memory */
      message_size = EncodeReplyService(message_size, &message,
                                        message_router_response);
      message_size = EnocdeReservedFieldOfLengthByte(message_size, &message,
                                                     message_router_response);
      message_size = EncodeGeneralStatus(message_size, &message,
                                         message_router_response);
      message_size = EncodeExtendedStatus(message_size, &message,
                                          message_router_response);
      message_size = EncodeMessageRouterResponseData(message_size,
                                                     message_router_response,
                                                     &message);
    } else { /* connected IO Message to send */
      message_size = EncodeDataItemType(common_packet_format_data_item,
                                        &message, message_size);

      message_size = EncodeDataItemLength(common_packet_format_data_item,
                                          &message, message_size);

      message_size = EncodeDataItemData(common_packet_format_data_item,
                                        &message, message_size);
    }
  }

  /* process SockAddr Info Items */
  /* make sure first the O->T and then T->O appears on the wire.
   * EtherNet/IP specification doesn't demand it, but there are EIP
   * devices which depend on CPF items to appear in the order of their
   * ID number */
  for (int type = kCipItemIdSocketAddressInfoOriginatorToTarget;
      type <= kCipItemIdSocketAddressInfoTargetToOriginator; type++) {
    for (int j = 0; j < 2; j++) {
      if (common_packet_format_data_item->address_info_item[j].type_id
          == type) {
        message_size = EncodeSockaddrInfoItemTypeId(
            message_size, j, common_packet_format_data_item, &message);

        message_size = EncodeSockaddrInfoLength(message_size, j,
                                                common_packet_format_data_item,
                                                &message);

        message_size += EncapsulateIpAddress(
            common_packet_format_data_item->address_info_item[j].sin_port,
            common_packet_format_data_item->address_info_item[j].sin_addr,
            &message);

        message_size += FillNextNMessageOctetsWithValueAndMoveToNextPosition(
            0, 8, &message);
        break;
      }
    }
  }
  return message_size;
}

int AssembleIOMessage(CipCommonPacketFormatData *common_packet_format_data_item,
                      EipUint8 *message) {
  return AssembleLinearMessage(0, common_packet_format_data_item,
                               &g_message_data_reply_buffer[0]);
}

