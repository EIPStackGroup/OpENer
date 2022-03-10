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
#include "encap.h"
#include "enipmessage.h"

const size_t kItemCountFieldSize = 2; /**< The size of the item count field in the message */
const size_t KItemDataTypeIdFieldLength = 2; /**< The size of the item count field in the message */

/** @brief Size, in bytes, of the encoded sequenced address item data field.
 *
 * Data type and value per @cite CipVol2, Table 2-6.6.
 */
const EipUint16 kSequencedAddressItemLength = 8;

CipCommonPacketFormatData g_common_packet_format_data_item; /**< CPF global data items */

static void InitializeMessageRouterResponse(
  CipMessageRouterResponse *const message_router_response) {
  memset(message_router_response, 0, sizeof(*message_router_response) );
  InitializeENIPMessage(&message_router_response->message);
}

EipStatus NotifyCommonPacketFormat(const EncapsulationData *const received_data,
                                   const struct sockaddr *const originator_address,
                                   ENIPMessage *const outgoing_message) {
  EipStatus return_value = kEipStatusError;
  CipMessageRouterResponse message_router_response;
  InitializeMessageRouterResponse(&message_router_response);

  if(kEipStatusError
     == (return_value =
           CreateCommonPacketFormatStructure(received_data->
                                             current_communication_buffer_position,
                                             received_data->data_length,
                                             &
                                             g_common_packet_format_data_item) ) )
  {
    OPENER_TRACE_ERR("notifyCPF: error from createCPFstructure\n");
  } else {
    return_value = kEipStatusOkSend; /* In cases of errors we normally need to send an error response */
    if(g_common_packet_format_data_item.address_item.type_id ==
       kCipItemIdNullAddress)                                                          /* check if NullAddressItem received, otherwise it is no unconnected message and should not be here*/
    { /* found null address item*/
      if(g_common_packet_format_data_item.data_item.type_id ==
         kCipItemIdUnconnectedDataItem) {                                                       /* unconnected data item received*/
        return_value = NotifyMessageRouter(
          g_common_packet_format_data_item.data_item.data,
          g_common_packet_format_data_item.data_item.length,
          &message_router_response,
          originator_address,
          received_data->session_handle);
        if(return_value != kEipStatusError) {
          SkipEncapsulationHeader(outgoing_message);
          /* TODO: Here we get the status. What to do? kEipStatusError from AssembleLinearMessage().
           *  Its not clear how to transport this error information to the requester. */
          EipStatus status = AssembleLinearMessage(&message_router_response,
                                                   &g_common_packet_format_data_item,
                                                   outgoing_message);
          (void)status; /* Suppress unused variable warning. */

          /* Save pointer and move to start for Encapusulation Header */
          CipOctet *buffer = outgoing_message->current_message_position;
          outgoing_message->current_message_position =
            outgoing_message->message_buffer;
          GenerateEncapsulationHeader(received_data,
                                      outgoing_message->used_message_length,
                                      received_data->session_handle,
                                      kEncapsulationProtocolSuccess,
                                      outgoing_message);
          /* Move pointer back to last octet */
          outgoing_message->current_message_position = buffer;
          return_value = kEipStatusOkSend;
        }
      } else {
        /* wrong data item detected*/
        OPENER_TRACE_ERR(
          "notifyCPF: got something besides the expected CIP_ITEM_ID_UNCONNECTEDMESSAGE\n");
        GenerateEncapsulationHeader(received_data,
                                    0,
                                    received_data->session_handle,
                                    kEncapsulationProtocolIncorrectData,
                                    outgoing_message);
        return_value = kEipStatusOkSend;
      }
    } else {
      OPENER_TRACE_ERR(
        "notifyCPF: got something besides the expected CIP_ITEM_ID_NULL\n");
      GenerateEncapsulationHeader(received_data,
                                  0,
                                  received_data->session_handle,
                                  kEncapsulationProtocolIncorrectData,
                                  outgoing_message);
      return_value = kEipStatusOkSend;
    }
  }
  return return_value;
}

EipStatus NotifyConnectedCommonPacketFormat(
  const EncapsulationData *const received_data,
  const struct sockaddr *const originator_address,
  ENIPMessage *const outgoing_message) {

  EipStatus return_value = CreateCommonPacketFormatStructure(
    received_data->current_communication_buffer_position,
    received_data->data_length,
    &g_common_packet_format_data_item);

  if(kEipStatusError == return_value) {
    OPENER_TRACE_ERR("notifyConnectedCPF: error from createCPFstructure\n");
  } else {
    return_value = kEipStatusError; /* For connected explicit messages status always has to be 0*/
    if(g_common_packet_format_data_item.address_item.type_id ==
       kCipItemIdConnectionAddress)                                                          /* check if ConnectedAddressItem received, otherwise it is no connected message and should not be here*/
    { /* ConnectedAddressItem item */
      CipConnectionObject *connection_object = GetConnectedObject(
        g_common_packet_format_data_item.address_item.data.connection_identifier);
      if(NULL != connection_object) {
        /* reset the watchdog timer */
        ConnectionObjectResetInactivityWatchdogTimerValue(connection_object);

        /*TODO check connection id  and sequence count */
        if(g_common_packet_format_data_item.data_item.type_id ==
           kCipItemIdConnectedDataItem) {                                                       /* connected data item received*/
          EipUint8 *buffer = g_common_packet_format_data_item.data_item.data;
          g_common_packet_format_data_item.address_item.data.sequence_number =
            GetUintFromMessage( (const EipUint8 **const ) &buffer );
          OPENER_TRACE_INFO(
            "Class 3 sequence number: %d, last sequence number: %d\n",
            g_common_packet_format_data_item.address_item.data.sequence_number,
            connection_object->sequence_count_consuming);
          if(connection_object->sequence_count_consuming ==
             g_common_packet_format_data_item.address_item.data.sequence_number)
          {
            memcpy(outgoing_message,
                   &(connection_object->last_reply_sent),
                   sizeof(ENIPMessage) );
            outgoing_message->current_message_position =
              outgoing_message->message_buffer;
            /* Regenerate encapsulation header for new message */
            outgoing_message->used_message_length -=
              ENCAPSULATION_HEADER_LENGTH;
            GenerateEncapsulationHeader(received_data,
                                        outgoing_message->used_message_length,
                                        received_data->session_handle,
                                        kEncapsulationProtocolSuccess,
                                        outgoing_message);
            outgoing_message->current_message_position = buffer;
            /* End regenerate encapsulation header for new message */
            return kEipStatusOkSend;
          }
          connection_object->sequence_count_consuming =
            g_common_packet_format_data_item.address_item.data.sequence_number;

          ConnectionObjectResetInactivityWatchdogTimerValue(connection_object);

          CipMessageRouterResponse message_router_response;
          InitializeMessageRouterResponse(&message_router_response);
          return_value = NotifyMessageRouter(buffer,
                                             g_common_packet_format_data_item.data_item.length - 2,
                                             &message_router_response,
                                             originator_address,
                                             received_data->session_handle);

          if(return_value != kEipStatusError) {
            g_common_packet_format_data_item.address_item.data.
            connection_identifier =
              connection_object->cip_produced_connection_id;
            SkipEncapsulationHeader(outgoing_message);
            /* TODO: Here we get the status. What to do? kEipStatusError from AssembleLinearMessage().
             *  Its not clear how to transport this error information to the requester. */
            EipStatus status = AssembleLinearMessage(&message_router_response,
                                                     &g_common_packet_format_data_item,
                                                     outgoing_message);
            (void)status; /* Suppress unused variable warning. */

            CipOctet *pos = outgoing_message->current_message_position;
            outgoing_message->current_message_position =
              outgoing_message->message_buffer;
            GenerateEncapsulationHeader(received_data,
                                        outgoing_message->used_message_length,
                                        received_data->session_handle,
                                        kEncapsulationProtocolSuccess,
                                        outgoing_message);
            outgoing_message->current_message_position = pos;
            memcpy(&connection_object->last_reply_sent,
                   outgoing_message,
                   sizeof(ENIPMessage) );
            return_value = kEipStatusOkSend;
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
  // return outgoing_message->used_message_length;
  return (0 !=
          outgoing_message->used_message_length ? kEipStatusOkSend :
          kEipStatusOk);                                                                 /* TODO: What would the right EipStatus to return? */
}

/**
 * @brief Creates Common Packet Format structure out of data.
 * @param data Pointer to data which need to be structured.
 * @param data_length	Length of data in pa_Data.
 * @param common_packet_format_data	Pointer to structure of CPF data item.
 *
 *   @return kEipStatusOk .. success
 *             kEipStatusError .. error
 */
EipStatus CreateCommonPacketFormatStructure(const EipUint8 *data,
                                            size_t data_length,
                                            CipCommonPacketFormatData *common_packet_format_data)
{

  common_packet_format_data->address_info_item[0].type_id = 0;
  common_packet_format_data->address_info_item[1].type_id = 0;

  size_t length_count = 0;
  CipUint item_count = GetUintFromMessage(&data);
  //OPENER_ASSERT(4U >= item_count);/* Sanitizing data - probably needs to be changed for productive code */
  common_packet_format_data->item_count = item_count;
  length_count += 2;
  if(common_packet_format_data->item_count >= 1U) {
    common_packet_format_data->address_item.type_id = GetUintFromMessage(&data);
    common_packet_format_data->address_item.length = GetUintFromMessage(&data);
    length_count += 4;
    if(common_packet_format_data->address_item.length >= 4) {
      common_packet_format_data->address_item.data.connection_identifier =
        GetUdintFromMessage(&data);
      length_count += 4;
    }
    if(common_packet_format_data->address_item.length == 8) {
      common_packet_format_data->address_item.data.sequence_number =
        GetUdintFromMessage(&data);
      length_count += 4;
    }
  }
  if(common_packet_format_data->item_count >= 2) {
    common_packet_format_data->data_item.type_id = GetUintFromMessage(&data);
    common_packet_format_data->data_item.length = GetUintFromMessage(&data);
    common_packet_format_data->data_item.data = (EipUint8 *) data;
    if(data_length >=
       length_count + 4 + common_packet_format_data->data_item.length) {
      data += common_packet_format_data->data_item.length;
      length_count += (4 + common_packet_format_data->data_item.length);
    } else {
      return kEipStatusError;
    }

    /* Data type per CIP Volume 2, Edition 1.4, Table 2-6.1. */
    CipUint address_item_count = (CipUint)(common_packet_format_data->item_count - 2U);

    for(size_t j = 0; j < (address_item_count > 2 ? 2 : address_item_count);
        j++)                                                                      /* TODO there needs to be a limit check here???*/
    {
      common_packet_format_data->address_info_item[j].type_id =
        GetIntFromMessage(&data);
      OPENER_TRACE_INFO("Sockaddr type id: %x\n",
                        common_packet_format_data->address_info_item[j].type_id);
      length_count += 2;
      if( (common_packet_format_data->address_info_item[j].type_id ==
           kCipItemIdSocketAddressInfoOriginatorToTarget)
          || (common_packet_format_data->address_info_item[j].type_id ==
              kCipItemIdSocketAddressInfoTargetToOriginator) ) {
        common_packet_format_data->address_info_item[j].length =
          GetIntFromMessage(&data);
        common_packet_format_data->address_info_item[j].sin_family =
          GetIntFromMessage(&data);
        common_packet_format_data->address_info_item[j].sin_port =
          GetIntFromMessage(&data);
        common_packet_format_data->address_info_item[j].sin_addr =
          GetUdintFromMessage(&data);
        for(size_t i = 0; i < 8; i++) {
          common_packet_format_data->address_info_item[j].nasin_zero[i] = *data;
          data++;
        }
        length_count += 18;
      } else { /* no sockaddr item found */
        common_packet_format_data->address_info_item[j].type_id = 0; /* mark as not set */
        data -= 2;
      }
    }
  }
  /* set the addressInfoItems to not set if they were not received */
  if(common_packet_format_data->item_count < 4) {
    common_packet_format_data->address_info_item[1].type_id = 0;
    if(common_packet_format_data->item_count < 3) {
      common_packet_format_data->address_info_item[0].type_id = 0;
    }
  }
  if(length_count == data_length) { /* length of data is equal to length of Addr and length of Data */
    return kEipStatusOk;
  } else {
    OPENER_TRACE_WARN(
      "something is wrong with the length in Message Router @ CreateCommonPacketFormatStructure\n");
    if(common_packet_format_data->item_count > 2) {
      /* there is an optional packet in data stream which is not sockaddr item */
      return kEipStatusOk;
    } else { /* something with the length was wrong */
      return kEipStatusError;
    }
  }
}

/**
 * @brief Encodes a Null Address Item into the message frame
 * @param outgoing_message The outgoing message object
 */
void EncodeNullAddressItem(ENIPMessage *const outgoing_message) {
  AddIntToMessage(kCipItemIdNullAddress, outgoing_message);
  /* null address item -> address length set to 0 */
  AddIntToMessage(0, outgoing_message);
}

/**
 * Encodes a Connected Address Item into the message frame
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param outgoing_message The outgoing message object
 */
void EncodeConnectedAddressItem(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {
  /* connected data item -> address length set to 4 and copy ConnectionIdentifier */
  AddIntToMessage(kCipItemIdConnectionAddress, outgoing_message);
  AddIntToMessage(4, outgoing_message);
  AddDintToMessage(
    common_packet_format_data_item->address_item.data.connection_identifier,
    outgoing_message);
}

/**
 * @brief Encodes a sequenced address item into the message
 *
 * @param common_packet_format_data_item Common Packet Format item which is used in the encoding
 * @param outgoing_message The outgoing message object
 */
void EncodeSequencedAddressItem(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {
  /* sequenced address item -> address length set to 8 and copy ConnectionIdentifier and SequenceNumber */
  AddIntToMessage(kCipItemIdSequencedAddressItem, outgoing_message);
  AddIntToMessage(kSequencedAddressItemLength, outgoing_message);
  AddDintToMessage(
    common_packet_format_data_item->address_item.data.connection_identifier,
    outgoing_message);
  AddDintToMessage(
    common_packet_format_data_item->address_item.data.sequence_number,
    outgoing_message);
}

/**
 * @brief Adds the item count to the message frame
 *
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param outgoing_message The outgoing message object
 */
void EncodeItemCount(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {
  AddIntToMessage(common_packet_format_data_item->item_count, outgoing_message); /* item count */
}

/**
 * Adds the data item type to the message frame
 *
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param outgoing_message The outgoing message object
 */
void EncodeDataItemType(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {
  AddIntToMessage(common_packet_format_data_item->data_item.type_id,
                  outgoing_message);
}

/**
 * Adds the data item section length to the message frame
 *
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param outgoing_message The outgoing message object
 */
void EncodeDataItemLength(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {
  AddIntToMessage(common_packet_format_data_item->data_item.length,
                  outgoing_message);
}

/**
 * Adds the data items to the message frame
 *
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param outgoing_message The outgoing message object
 */
void EncodeDataItemData(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {
  memcpy(outgoing_message->current_message_position,
         common_packet_format_data_item->data_item.data,
         common_packet_format_data_item->data_item.length);
  outgoing_message->current_message_position +=
    common_packet_format_data_item->data_item.length;
  outgoing_message->used_message_length +=
    common_packet_format_data_item->data_item.length;
}

/**
 * @brief Encodes the Connected Data item length
 *
 * @param message_router_response The Router Response message which shall be answered
 * @param outgoing_message The outgoing message object
 */

void EncodeConnectedDataItemLength(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  AddIntToMessage( (EipUint16) (message_router_response->message.
                                used_message_length + 4 + 2                                 /* TODO: Magic numbers */
                                + (2 *
                                   message_router_response->
                                   size_of_additional_status) ),
                   outgoing_message );
}

/**
 * @brief Encodes a sequence number into the message
 *
 * @param common_packet_format_data_item
 * @param outgoing_message The outgoing message object
 */
void EncodeSequenceNumber(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {
  AddIntToMessage(
    (EipUint16) common_packet_format_data_item->address_item.data.sequence_number,
    outgoing_message );
}

/**
 * @brief Encodes the reply service code for the requested service
 *
 * @param message_router_response The router response message data structure to be processed
 * @param outgoing_message The outgoing message object
 */
void EncodeReplyService(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  AddSintToMessage(message_router_response->reply_service, outgoing_message);
}

/**
 * @brief Encodes the reserved byte in the message router response
 *
 * @param message_router_response Router Response message to be processed
 * @param outgoing_message The outgoing message object
 */
void EncodeReservedFieldOfLengthByte(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  AddSintToMessage(message_router_response->reserved, outgoing_message);
}

/**
 * @brief Encodes the general status of a Router Response
 *
 * @param message_router_response Router Response message to be processed
 * @param outgoing_message The outgoing message object
 */
void EncodeGeneralStatus(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  AddSintToMessage(message_router_response->general_status, outgoing_message);
}

/**
 * @brief Encodes the length of the extended status data part
 *
 * @param message_router_response Router Response message to be processed
 * @param outgoing_message The outgoing message object
 */

void EncodeExtendedStatusLength(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  AddSintToMessage(message_router_response->size_of_additional_status,
                   outgoing_message);
}

/**
 * @brief Encodes the extended status data items
 *
 * @param message_router_response Router Response message to be processed
 * @param outgoing_message The outgoing message object
 */
void EncodeExtendedStatusDataItems(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  for(size_t i = 0;
      i < message_router_response->size_of_additional_status &&
      i < MAX_SIZE_OF_ADD_STATUS; i++) {
    AddIntToMessage(message_router_response->additional_status[i],
                    outgoing_message);
  }
}

/**
 * @brief Encodes the extended status (length and data) into the message
 *
 * This function uses EncodeExtendedStatusLength and EncodeExtendedStatusDataItems
 * to encode the complete extended status information into the message
 *
 * @param message_router_response Router Response message to be processed
 * @param outgoing_message The outgoing message object
 */

void EncodeExtendedStatus(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  EncodeExtendedStatusLength(message_router_response, outgoing_message);
  EncodeExtendedStatusDataItems(message_router_response, outgoing_message);
}

/**
 * @brief Encode the data item length of the unconnected data segment
 *
 * @param message_router_response Router Response message to be processed
 * @param outgoing_message The outgoing message object
 *
 */
void EncodeUnconnectedDataItemLength(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  AddIntToMessage( (EipUint16) (message_router_response->message.
                                used_message_length + 4                                 /* TODO: Magic number */
                                + (2 *
                                   message_router_response->
                                   size_of_additional_status) ),
                   outgoing_message );
}

/**
 * @brief Encodes the Message Router Response data
 *
 * @param message_router_response Router Response message to be processed
 * @param outgoing_message The outgoing message object
 */
void EncodeMessageRouterResponseData(
  const CipMessageRouterResponse *const message_router_response,
  ENIPMessage *const outgoing_message) {
  memcpy(outgoing_message->current_message_position,
         message_router_response->message.message_buffer,
         message_router_response->message.used_message_length);

  outgoing_message->current_message_position +=
    message_router_response->message.used_message_length;
  outgoing_message->used_message_length +=
    message_router_response->message.used_message_length;
}

/**
 * @brief Encodes the sockaddr info type id into the message
 *
 * @param item_type
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param outgoing_message The outgoing message object
 */
void EncodeSockaddrInfoItemTypeId(int item_type,
                                  const CipCommonPacketFormatData *const common_packet_format_data_item,
                                  ENIPMessage *const outgoing_message) {
  OPENER_ASSERT(item_type == 0 || item_type == 1);
  AddIntToMessage(
    common_packet_format_data_item->address_info_item[item_type].type_id,
    outgoing_message);
}

/**
 * @brief Encodes the sockaddr info length into the message
 *
 * @param item_type
 * @param common_packet_format_data_item The Common Packet Format data structure from which the message is constructed
 * @param outgoing_message The outgoing message object
 */
void EncodeSockaddrInfoLength(int item_type,
                              const CipCommonPacketFormatData *const common_packet_format_data_item,
                              ENIPMessage *const outgoing_message) {
  AddIntToMessage(
    common_packet_format_data_item->address_info_item[item_type].length,
    outgoing_message);
}

EipStatus AssembleLinearMessage(
  const CipMessageRouterResponse *const message_router_response,
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {

  if(message_router_response) {
    /* add Interface Handle and Timeout = 0 -> only for SendRRData and SendUnitData necessary */
    AddDintToMessage(0, outgoing_message);
    AddIntToMessage(0, outgoing_message);
  }

  EncodeItemCount(common_packet_format_data_item, outgoing_message);

  /* process Address Item */
  switch(common_packet_format_data_item->address_item.type_id) {
    case kCipItemIdNullAddress: {
      EncodeNullAddressItem(outgoing_message);
      break;
    }
    case kCipItemIdConnectionAddress: {
      EncodeConnectedAddressItem(common_packet_format_data_item,
                                 outgoing_message);
      break;
    }
    case kCipItemIdSequencedAddressItem: {
      EncodeSequencedAddressItem(common_packet_format_data_item,
                                 outgoing_message);
      break;
    }
    default:
      OPENER_TRACE_INFO("Unknown CIP Item in AssembleLinearMessage");
      return kEipStatusError;
  }

  /* process Data Item */
  if( (common_packet_format_data_item->data_item.type_id ==
       kCipItemIdUnconnectedDataItem)
      || (common_packet_format_data_item->data_item.type_id ==
          kCipItemIdConnectedDataItem) ) {

    if(message_router_response) {
      EncodeDataItemType(common_packet_format_data_item, outgoing_message);

      if(common_packet_format_data_item->data_item.type_id ==
         kCipItemIdConnectedDataItem) {                                                      /* Connected Item */
        EncodeConnectedDataItemLength(message_router_response,
                                      outgoing_message);
        EncodeSequenceNumber(&g_common_packet_format_data_item,
                             outgoing_message);

      } else { /* Unconnected Item */
        EncodeUnconnectedDataItemLength(message_router_response,
                                        outgoing_message);
      }

      /* write message router response into linear memory */
      EncodeReplyService(message_router_response, outgoing_message);
      EncodeReservedFieldOfLengthByte(message_router_response,
                                      outgoing_message);
      EncodeGeneralStatus(message_router_response, outgoing_message);
      EncodeExtendedStatus(message_router_response, outgoing_message);
      EncodeMessageRouterResponseData(message_router_response,
                                      outgoing_message);
    } else { /* connected IO Message to send */
      EncodeDataItemType(common_packet_format_data_item, outgoing_message);

      EncodeDataItemLength(common_packet_format_data_item, outgoing_message);

      EncodeDataItemData(common_packet_format_data_item, outgoing_message);
    }
  }

  /* process SockAddr Info Items */
  /* make sure first the O->T and then T->O appears on the wire.
   * EtherNet/IP specification doesn't demand it, but there are EIP
   * devices which depend on CPF items to appear in the order of their
   * ID number */
  for(int type = kCipItemIdSocketAddressInfoOriginatorToTarget;
      type <= kCipItemIdSocketAddressInfoTargetToOriginator; type++) {
    for(int j = 0; j < 2; j++) {
      if(common_packet_format_data_item->address_info_item[j].type_id == type) {
        EncodeSockaddrInfoItemTypeId(j,
                                     common_packet_format_data_item,
                                     outgoing_message);

        EncodeSockaddrInfoLength(j,
                                 common_packet_format_data_item,
                                 outgoing_message);

        EncapsulateIpAddress(
          common_packet_format_data_item->address_info_item[j].sin_port,
          common_packet_format_data_item->address_info_item[j].sin_addr,
          outgoing_message);

        FillNextNMessageOctetsWithValueAndMoveToNextPosition(0,
                                                             8,
                                                             outgoing_message);
        break;
      }
    }
  }
  return kEipStatusOk;
}

void AssembleIOMessage(
  const CipCommonPacketFormatData *const common_packet_format_data_item,
  ENIPMessage *const outgoing_message) {
  AssembleLinearMessage(0, common_packet_format_data_item, outgoing_message);
}
