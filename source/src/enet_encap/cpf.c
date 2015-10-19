/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include "opener_api.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "endianconv.h"
#include "ciperror.h"
#include "cpf.h"
#include "cipconnectionmanager.h"
#include "trace.h"
#include <string.h>

/* CPF global data items */
CipCommonPacketFormatData g_common_packet_format_data_item;

int NotifyCommonPacketFormat(EncapsulationData *receive_data,
                             EipUint8 *reply_buffer) {
  int return_value;

  if ((return_value = CreateCommonPacketFormatStructure(
      receive_data->current_communication_buffer_position,
      receive_data->data_length, &g_common_packet_format_data_item))
      == kEipStatusError) {
    OPENER_TRACE_ERR("notifyCPF: error from createCPFstructure\n");
  } else {
    return_value = kEipStatusOk; /* In cases of errors we normally need to send an error response */
    if (g_common_packet_format_data_item.address_item.type_id == kNullAddressId) /* check if NullAddressItem received, otherwise it is no unconnected message and should not be here*/
    { /* found null address item*/
      if (g_common_packet_format_data_item.data_item.type_id
          == kUnconnectedMessageId) { /* unconnected data item received*/
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
  int return_value;
  ConnectionObject *connection_object;

  return_value = CreateCommonPacketFormatStructure(
      received_data->current_communication_buffer_position,
      received_data->data_length, &g_common_packet_format_data_item);

  if (kEipStatusError == return_value) {
    OPENER_TRACE_ERR("notifyConnectedCPF: error from createCPFstructure\n");
  } else {
    return_value = kEipStatusError; /* For connected explicit messages status always has to be 0*/
    if (g_common_packet_format_data_item.address_item.type_id
        == kConnectionBasedId) /* check if ConnectedAddressItem received, otherwise it is no connected message and should not be here*/
        { /* ConnectedAddressItem item */
      connection_object = GetConnectedObject(
          g_common_packet_format_data_item.address_item.data
              .connection_identifier);
      if (NULL != connection_object) {
        /* reset the watchdog timer */
        connection_object->inactivity_watchdog_timer = (connection_object
            ->o_to_t_requested_packet_interval / 1000)
            << (2 + connection_object->connection_timeout_multiplier);

        /*TODO check connection id  and sequence count    */
        if (g_common_packet_format_data_item.data_item.type_id
            == kConnectedTransportPacketId) { /* connected data item received*/
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

/*   INT16 createCPFstructure(INT8 *pa_Data, INT16 pa_DataLength, S_CIP_CPF_Data *pa_CPF_data)
 *   create CPF structure out of pa_data.
 *      pa_Data		pointer to data which need to be structured.
 *      pa_DataLength	length of data in pa_Data.
 *      pa_CPF_data	pointer to structure of CPF data item.
 *  return status
 * 		0 .. success
 * 	       -1 .. error
 */
EipStatus CreateCommonPacketFormatStructure(
    EipUint8 *data, int data_length,
    CipCommonPacketFormatData *common_packet_format_data) {
  int len_count, i, j;

  common_packet_format_data->address_info_item[0].type_id = 0;
  common_packet_format_data->address_info_item[1].type_id = 0;

  len_count = 0;
  common_packet_format_data->item_count = GetIntFromMessage(&data);
  len_count += 2;
  if (common_packet_format_data->item_count >= 1) {
    common_packet_format_data->address_item.type_id = GetIntFromMessage(&data);
    common_packet_format_data->address_item.length = GetIntFromMessage(&data);
    len_count += 4;
    if (common_packet_format_data->address_item.length >= 4) {
      common_packet_format_data->address_item.data.connection_identifier =
          GetDintFromMessage(&data);
      len_count += 4;
    }
    if (common_packet_format_data->address_item.length == 8) {
      common_packet_format_data->address_item.data.sequence_number =
          GetDintFromMessage(&data);
      len_count += 4;
    }
  }
  if (common_packet_format_data->item_count >= 2) {
    common_packet_format_data->data_item.type_id = GetIntFromMessage(&data);
    common_packet_format_data->data_item.length = GetIntFromMessage(&data);
    common_packet_format_data->data_item.data = data;
    data += common_packet_format_data->data_item.length;
    len_count += (4 + common_packet_format_data->data_item.length);
  }
  for (j = 0; j < (common_packet_format_data->item_count - 2); j++) /* TODO there needs to be a limit check here???*/
  {
    common_packet_format_data->address_info_item[j].type_id = GetIntFromMessage(
        &data);
    len_count += 2;
    if ((common_packet_format_data->address_info_item[j].type_id
        == kSocketAddressInfoOriginatorToTargetId)
        || (common_packet_format_data->address_info_item[j].type_id
            == kSocketAddressInfoTargetToOriginatorId)) {
      common_packet_format_data->address_info_item[j].length =
          GetIntFromMessage(&data);
      common_packet_format_data->address_info_item[j].nsin_family =
          GetIntFromMessage(&data);
      common_packet_format_data->address_info_item[j].nsin_port =
          GetIntFromMessage(&data);
      common_packet_format_data->address_info_item[j].nsin_addr =
          GetDintFromMessage(&data);
      for (i = 0; i < 8; i++) {
        common_packet_format_data->address_info_item[j].nasin_zero[i] = *data;
        data++;
      }
      len_count += 18;
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
  if (len_count == data_length) { /* length of data is equal to length of Addr and length of Data */
    return kEipStatusOk;
  } else {
    OPENER_TRACE_WARN(
        "something is wrong with the length in MR @ createCPFstructure\n");
    if (common_packet_format_data->item_count > 2) {
      /* there is an optional packet in data stream which is not sockaddr item */
      return kEipStatusOk;
    } else { /* something with the length was wrong */
      return kEipStatusError;
    }
  }
}

/*   INT8 assembleLinearMsg(S_CIP_MR_Response *pa_MRResponse, S_CIP_CPF_Data *pa_CPFDataItem, INT8 *pa_msg)
 *   copy data from MRResponse struct and CPFDataItem into linear memory in pa_msg for transmission over in encapsulation.
 *      pa_MRResponse	pointer to message router response which has to be aligned into linear memory.
 *      pa_CPFDataItem	pointer to CPF structure which has to be aligned into linear memory.
 *      pa_msg		pointer to linear memory.
 *  return length of reply in pa_msg in bytes
 * 			-1 .. error
 */
int AssembleLinearMessage(
    CipMessageRouterResponse *message_router_response,
    CipCommonPacketFormatData * common_packet_format_data_item,
    EipUint8 *message) {
  int i, j, type;

  int size = 0;
  if (message_router_response) {
    /* add Interface Handle and Timeout = 0 -> only for SendRRData and SendUnitData necessary */
    AddDintToMessage(0, &message);
    AddIntToMessage(0, &message);
    size += 6;
  }

  AddIntToMessage(common_packet_format_data_item->item_count, &message); /* item count */
  size += 2;
  /* process Address Item */
  if (common_packet_format_data_item->address_item.type_id == kNullAddressId) { /* null address item -> address length set to 0 */
    AddIntToMessage(kNullAddressId, &message);
    AddIntToMessage(0, &message);
    size += 4;
  }
  if (common_packet_format_data_item->address_item.type_id
      == kConnectionBasedId) { /* connected data item -> address length set to 4 and copy ConnectionIdentifier */
    AddIntToMessage(kConnectionBasedId, &message);
    AddIntToMessage(4, &message);
    AddDintToMessage(
        common_packet_format_data_item->address_item.data.connection_identifier,
        &message);
    size += 8;
  }
  /* sequence number????? */
  if (common_packet_format_data_item->address_item.type_id
      == kSequencedAddressItemId) { /* sequenced address item -> address length set to 8 and copy ConnectionIdentifier and SequenceNumber */
    AddIntToMessage(kSequencedAddressItemId, &message);
    AddIntToMessage(8, &message);
    AddDintToMessage(
        common_packet_format_data_item->address_item.data.connection_identifier,
        &message);
    AddDintToMessage(
        common_packet_format_data_item->address_item.data.sequence_number,
        &message);
    size += 12;
  }

  /* process Data Item */
  if ((common_packet_format_data_item->data_item.type_id
      == kUnconnectedMessageId)
      || (common_packet_format_data_item->data_item.type_id
          == kConnectedTransportPacketId)) {
    if (message_router_response) {
      AddIntToMessage(common_packet_format_data_item->data_item.type_id,
                      &message);

      if (common_packet_format_data_item->data_item.type_id
          == kConnectedTransportPacketId) {
        AddIntToMessage(
            (EipUint16) (message_router_response->data_length + 4 + 2
                + (2 * message_router_response->size_of_additional_status)),
            &message);

        AddIntToMessage(
            (EipUint16) g_common_packet_format_data_item.address_item.data
                .sequence_number,
            &message);

        size += (4 + message_router_response->data_length + 4 + 2
            + (2 * message_router_response->size_of_additional_status));
      } else {
        AddIntToMessage(
            (EipUint16) (message_router_response->data_length + 4
                + (2 * message_router_response->size_of_additional_status)),
            &message);
        size += (4 + message_router_response->data_length + 4
            + (2 * message_router_response->size_of_additional_status));
      }

      /* write message router response into linear memory */
      *message = message_router_response->reply_service;
      message++;
      *message = message_router_response->reserved; /* reserved = 0 */
      message++;
      *message = message_router_response->general_status;
      message++;
      *message = message_router_response->size_of_additional_status;
      message++;
      for (i = 0; i < message_router_response->size_of_additional_status; i++)
        AddIntToMessage(message_router_response->additional_status[i],
                        &message);

      for (i = 0; i < message_router_response->data_length; i++) {
        *message = (EipUint8) *(message_router_response->data + i);
        message++;
      }
    } else { /* connected IO Message to send */
      AddIntToMessage(common_packet_format_data_item->data_item.type_id,
                      &message);
      AddIntToMessage(common_packet_format_data_item->data_item.length,
                      &message);
      for (i = 0; i < common_packet_format_data_item->data_item.length; i++) {
        *message = (EipUint8) *(common_packet_format_data_item->data_item.data
            + i);
        message++;
      }
      size += (common_packet_format_data_item->data_item.length + 4);
    }
  }
  /* process SockAddr Info Items */
  /* make sure first the O->T and then T->O appears on the wire.
   * EtherNet/IP specification doesn't demand it, but there are EIP
   * devices which depend on CPF items to appear in the order of their
   * ID number */
  for (type = kSocketAddressInfoOriginatorToTargetId;
      type <= kSocketAddressInfoTargetToOriginatorId; type++) {
    for (j = 0; j < 2; j++) {
      if (common_packet_format_data_item->address_info_item[j].type_id
          == type) {
        AddIntToMessage(
            common_packet_format_data_item->address_info_item[j].type_id,
            &message);
        AddIntToMessage(
            common_packet_format_data_item->address_info_item[j].length,
            &message);

        EncapsulateIpAddressCommonPaketFormat(
            common_packet_format_data_item->address_info_item[j].nsin_port,
            common_packet_format_data_item->address_info_item[j].nsin_addr,
            message);
        message += 8;

        memset(message, 0, 8);
        message += 8;
        size += 20;
        break;
      }
    }
  }
  return size;
}

