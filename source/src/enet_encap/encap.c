/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "encap.h"

#include "opener_api.h"
#include "opener_user_conf.h"
#include "cpf.h"
#include "endianconv.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "cipconnectionmanager.h"
#include "cipidentity.h"
#include "ciptcpipinterface.h"
#include "generic_networkhandler.h"
#include "trace.h"
#include "socket_timer.h"
#include "opener_error.h"

/* IP address data taken from TCPIPInterfaceObject*/
const EipUint16 kSupportedProtocolVersion = 1; /**< Supported Encapsulation protocol version */

const CipUdint kEncapsulationHeaderOptionsFlag = 0x00; /**< Mask of which options are supported as of the current CIP specs no other option value as 0 should be supported.*/

const int kEncapsulationHeaderSessionHandlePosition = 4; /**< the position of the session handle within the encapsulation header*/

const EipUint16 kListIdentityDefaultDelayTime = 2000; /**< Default delay time for List Identity response */
const EipUint16 kListIdentityMinimumDelayTime = 500; /**< Minimum delay time for List Identity response */

typedef enum {
  kSessionStatusInvalid = -1,
  kSessionStatusValid = 0
} SessionStatus;

const int kSenderContextSize = 8; /**< size of sender context in encapsulation header*/

/** @brief definition of known encapsulation commands */
typedef enum {
  kEncapsulationCommandNoOperation = 0x0000, /**< only allowed for TCP */
  kEncapsulationCommandListServices = 0x0004, /**< allowed for both UDP and TCP */
  kEncapsulationCommandListIdentity = 0x0063, /**< allowed for both UDP and TCP */
  kEncapsulationCommandListInterfaces = 0x0064, /**< optional, allowed for both UDP and TCP */
  kEncapsulationCommandRegisterSession = 0x0065, /**< only allowed for TCP */
  kEncapsulationCommandUnregisterSession = 0x0066, /**< only allowed for TCP */
  kEncapsulationCommandSendRequestReplyData = 0x006F, /**< only allowed for TCP */
  kEncapsulationCommandSendUnitData = 0x0070 /**< only allowed for TCP */
} EncapsulationCommand;

/** @brief definition of capability flags */
typedef enum {
  kCapabilityFlagsCipTcp = 0x0020,
  kCapabilityFlagsCipUdpClass0or1 = 0x0100
} CapabilityFlags;

#define ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES 2 /**< According to EIP spec at least 2 delayed message requests should be supported */

/* Encapsulation layer data  */

/** @brief Delayed Encapsulation Message structure */
typedef struct {
  EipInt32 time_out; /**< time out in milli seconds */
  int socket; /**< associated socket */
  struct sockaddr_in receiver;
  ENIPMessage outgoing_message;
} DelayedEncapsulationMessage;

EncapsulationServiceInformation g_service_information;

int g_registered_sessions[OPENER_NUMBER_OF_SUPPORTED_SESSIONS];

DelayedEncapsulationMessage g_delayed_encapsulation_messages[ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES];

/*** private functions ***/
void HandleReceivedListIdentityCommandTcp(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message);

void HandleReceivedListIdentityCommandUdp(const int socket,
                                          const struct sockaddr_in *const from_address,
                                          const EncapsulationData *const receive_data);

EipStatus HandleReceivedUnregisterSessionCommand(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message);

EipStatus HandleReceivedSendUnitDataCommand(const EncapsulationData *const receive_data, const struct sockaddr *const originator_address,
    ENIPMessage *const outgoing_message);

EipStatus HandleReceivedInvalidCommand(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message);

int GetFreeSessionIndex(void);

SessionStatus CheckRegisteredSessions(const EncapsulationData *const receive_data);

void DetermineDelayTime(const EipByte *buffer_start, DelayedEncapsulationMessage *const delayed_message_buffer);

/*   @brief Initializes session list and interface information. */
void EncapsulationInit(void) {

  DetermineEndianess();

  /*initialize random numbers for random delayed response message generation
   * we use the ip address as seed as suggested in the spec */
  srand(g_tcpip.interface_configuration.ip_address);

  /* initialize Sessions to invalid == free session */
  for(size_t i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; i++) {
    g_registered_sessions[i] = kEipInvalidSocket;
  }

  for(size_t i = 0; i < ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES; i++) {
    g_delayed_encapsulation_messages[i].socket = kEipInvalidSocket;
  }

  /*TODO make the service information configurable*/
  /* initialize service information */
  g_service_information.type_code = kCipItemIdListServiceResponse;
  g_service_information.length = sizeof(g_service_information);
  g_service_information.encapsulation_protocol_version = 1;
  g_service_information.capability_flags = kCapabilityFlagsCipTcp | kCapabilityFlagsCipUdpClass0or1;
  snprintf((char*) g_service_information.name_of_service, sizeof(g_service_information.name_of_service), "Communications");
}

EipStatus HandleReceivedExplictTcpData(int socket, EipUint8 *buffer, size_t length, int *number_of_remaining_bytes, struct sockaddr *originator_address,
    ENIPMessage *const outgoing_message) {
  OPENER_TRACE_INFO("Handles data for TCP socket: %d\n", socket);
  EipStatus return_value = kEipStatusOk;
  EncapsulationData encapsulation_data = { 0 };
  /* eat the encapsulation header*/
  /* the structure contains a pointer to the encapsulated data*/
  /* returns how many bytes are left after the encapsulated data*/
  const int remaining_bytes = CreateEncapsulationStructure(buffer, length, &encapsulation_data);

  if(remaining_bytes >= 0) {
    *number_of_remaining_bytes = remaining_bytes;
  } else {
    OPENER_TRACE_ERR("Fragmented packet detected! Fragmented packets are not supported!\n");
    *number_of_remaining_bytes = 0;
    return kEipStatusError;
  }

  if(kEncapsulationHeaderOptionsFlag == encapsulation_data.options) /*TODO generate appropriate error response*/
  {
    if(*number_of_remaining_bytes >= 0) /* check if the message is corrupt: header size + claimed payload size > than what we actually received*/
    {
      /* full package or more received */
      encapsulation_data.status = kEncapsulationProtocolSuccess;
      return_value = kEipStatusOkSend;
      /* most of these functions need a reply to be send */
      switch(encapsulation_data.command_code){
        case (kEncapsulationCommandNoOperation):
          OPENER_TRACE_INFO("NOP\n");
          /* NOP needs no reply and does nothing */
          return_value = kEipStatusOk;
          break;

        case (kEncapsulationCommandListServices):
          OPENER_TRACE_INFO("List services\n");
          HandleReceivedListServicesCommand(&encapsulation_data, outgoing_message);
          break;

        case (kEncapsulationCommandListIdentity):
          OPENER_TRACE_INFO("List identity\n");
          HandleReceivedListIdentityCommandTcp(&encapsulation_data, outgoing_message);
          break;

        case (kEncapsulationCommandListInterfaces):
          OPENER_TRACE_INFO("List interfaces\n");
          HandleReceivedListInterfacesCommand(&encapsulation_data, outgoing_message);
          break;

        case (kEncapsulationCommandRegisterSession):
          OPENER_TRACE_INFO("Register session\n");
          HandleReceivedRegisterSessionCommand(socket, &encapsulation_data, outgoing_message);
          break;

        case (kEncapsulationCommandUnregisterSession):
          OPENER_TRACE_INFO("unregister session\n");
          return_value = HandleReceivedUnregisterSessionCommand(&encapsulation_data, outgoing_message);
          break;

        case (kEncapsulationCommandSendRequestReplyData):
          OPENER_TRACE_INFO("Send Request/Reply Data\n");
          return_value = HandleReceivedSendRequestResponseDataCommand(&encapsulation_data, originator_address, outgoing_message);
          break;

        case (kEncapsulationCommandSendUnitData):
          OPENER_TRACE_INFO("Send Unit Data\n");
          return_value = HandleReceivedSendUnitDataCommand(&encapsulation_data, originator_address, outgoing_message);
          break;

        default:
          return_value = HandleReceivedInvalidCommand(&encapsulation_data, outgoing_message);
          break;
      }
    }
  }

  return return_value;
}

EipStatus HandleReceivedExplictUdpData(const int socket, const struct sockaddr_in *from_address, const EipUint8 *buffer, const size_t buffer_length,
    int *number_of_remaining_bytes,
    bool unicast, ENIPMessage *const outgoing_message) {
  EipStatus return_value = kEipStatusOk;
  EncapsulationData encapsulation_data = { 0 };
  /* eat the encapsulation header*/
  /* the structure contains a pointer to the encapsulated data*/
  /* returns how many bytes are left after the encapsulated data*/
  const int remaining_bytes = CreateEncapsulationStructure(buffer, buffer_length, &encapsulation_data);

  if(remaining_bytes >= 0) {
    *number_of_remaining_bytes = remaining_bytes;
  } else {
    OPENER_TRACE_ERR("Fragmented packet detected! Fragmented packets are not supported!\n");
    *number_of_remaining_bytes = 0;
    return kEipStatusError;
  }

  if(kEncapsulationHeaderOptionsFlag == encapsulation_data.options) /*TODO generate appropriate error response*/
  {
    if(*number_of_remaining_bytes >= 0) /* check if the message is corrupt: header size + claimed payload size > than what we actually received*/
    {
      /* full package or more received */
      encapsulation_data.status = kEncapsulationProtocolSuccess;
      return_value = kEipStatusOkSend;
      /* most of these functions need a reply to be send */
      switch(encapsulation_data.command_code){
        case (kEncapsulationCommandListServices):
          OPENER_TRACE_INFO("List Service\n");
          HandleReceivedListServicesCommand(&encapsulation_data, outgoing_message);
          break;

        case (kEncapsulationCommandListIdentity):
          OPENER_TRACE_INFO("List Identity\n");
          if(unicast == true) {
            HandleReceivedListIdentityCommandTcp(&encapsulation_data, outgoing_message);
          } else {
            HandleReceivedListIdentityCommandUdp(socket,
                                                 from_address,
                                                 &encapsulation_data);
            /* as the response has to be delayed do not send it now */
            return_value = kEipStatusOk;
          }
          break;

        case (kEncapsulationCommandListInterfaces):
          OPENER_TRACE_INFO("List Interfaces\n");
          HandleReceivedListInterfacesCommand(&encapsulation_data, outgoing_message);
          break;

          /* The following commands are not to be sent via UDP */
        case (kEncapsulationCommandNoOperation):
        case (kEncapsulationCommandRegisterSession):
        case (kEncapsulationCommandUnregisterSession):
        case (kEncapsulationCommandSendRequestReplyData):
        case (kEncapsulationCommandSendUnitData):
        default:
          OPENER_TRACE_INFO("No command\n");
          //TODO: Check this
          encapsulation_data.status = kEncapsulationProtocolInvalidCommand;
          encapsulation_data.data_length = 0;
          break;
      }
    }
  }
  return return_value;
}

void SkipEncapsulationHeader(ENIPMessage *const outgoing_message) {
  /* Move pointer over Header, but do not add to size */
  outgoing_message->current_message_position += ENCAPSULATION_HEADER_LENGTH;
}

void GenerateEncapsulationHeader(const EncapsulationData *const receive_data, const size_t command_specific_data_length, const CipSessionHandle session_handle,
    const EncapsulationProtocolErrorCode encapsulation_protocol_status, ENIPMessage *const outgoing_message) {
  AddIntToMessage(receive_data->command_code, outgoing_message);
  AddIntToMessage(command_specific_data_length, outgoing_message);
  AddDintToMessage(session_handle, outgoing_message); //Session handle
  AddDintToMessage(encapsulation_protocol_status, outgoing_message); //Status
  memcpy(outgoing_message->current_message_position, receive_data->sender_context, kSenderContextSize); // sender context
  outgoing_message->current_message_position += kSenderContextSize;
  outgoing_message->used_message_length += kSenderContextSize;
  AddDintToMessage(0, outgoing_message); // options
}

/** @brief generate reply with "Communications Services" + compatibility Flags.
 *  @param receive_data pointer to structure with received data
 *  @param outgoing_message The outgoing ENIP message
 */
void HandleReceivedListServicesCommand(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message) {

  /* Create encapsulation header */
  const size_t kListServicesCommandSpecificDataLength = sizeof(CipUint) + sizeof(g_service_information);
  GenerateEncapsulationHeader(receive_data, kListServicesCommandSpecificDataLength, 0,
  /* Session handle will be ignored */
  kEncapsulationProtocolSuccess,
  /* Protocol status */
  outgoing_message);

  /* Command specific data copy Interface data to msg for sending */
  AddIntToMessage(1, outgoing_message); // Item count
  AddIntToMessage(g_service_information.type_code, outgoing_message);
  AddIntToMessage((EipUint16) (g_service_information.length - 4), outgoing_message);
  AddIntToMessage(g_service_information.encapsulation_protocol_version, outgoing_message);
  AddIntToMessage(g_service_information.capability_flags, outgoing_message);
  memcpy(outgoing_message->current_message_position, g_service_information.name_of_service, sizeof(g_service_information.name_of_service));
  outgoing_message->current_message_position += sizeof(g_service_information.name_of_service);
  outgoing_message->used_message_length += sizeof(g_service_information.name_of_service);
}

void HandleReceivedListInterfacesCommand(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message) {

  /* Encapsulation header */
  const size_t kListInterfacesCommandSpecificDataLength = sizeof(CipUint);

  GenerateEncapsulationHeader(receive_data, kListInterfacesCommandSpecificDataLength, 0,
  /* Session handle will be ignored */
  kEncapsulationProtocolSuccess, outgoing_message);
  /* Command specific data */
  AddIntToMessage(0x0000, outgoing_message); /* Set Item Count to 0: no Target Items follow. */
}

void HandleReceivedListIdentityCommandTcp(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message) {
  EncapsulateListIdentityResponseMessage(receive_data, outgoing_message);
}

void HandleReceivedListIdentityCommandUdp(const int socket,
                                          const struct sockaddr_in *const from_address,
                                          const EncapsulationData *const receive_data)
{
  DelayedEncapsulationMessage *delayed_message_buffer = NULL;
  ENIPMessage *p_outgoing_message = NULL;

  for(size_t i = 0; i < ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES; i++) {
    if(kEipInvalidSocket == g_delayed_encapsulation_messages[i].socket) {
      delayed_message_buffer = &(g_delayed_encapsulation_messages[i]);

      p_outgoing_message = &(delayed_message_buffer->outgoing_message);
      InitializeENIPMessage(p_outgoing_message);
      break;
    }
  }

  if(NULL != delayed_message_buffer) {
    delayed_message_buffer->socket = socket;
    memcpy((&delayed_message_buffer->receiver), from_address, sizeof(struct sockaddr_in));

    DetermineDelayTime(receive_data->communication_buffer_start, delayed_message_buffer);

    EncapsulateListIdentityResponseMessage(receive_data, p_outgoing_message);
  }
}

CipUint ListIdentityGetCipIdentityItemLength() {
  return sizeof(CipUint) + sizeof(CipInt) + sizeof(CipUint) + sizeof(CipUdint) + 8 * sizeof(CipUsint) + sizeof(CipUint) + sizeof(CipUint) + sizeof(CipUint)
    + 2 * sizeof(CipUsint) + sizeof(CipWord) + sizeof(CipUdint) + sizeof(CipUsint) + g_identity.product_name.length + sizeof(CipUsint);
}

void EncodeListIdentityCipIdentityItem(ENIPMessage *const outgoing_message) {
  /* Item ID*/
  const CipUint kItemIDCipIdentity = 0x0C;
  AddIntToMessage(kItemIDCipIdentity, outgoing_message);

  AddIntToMessage(ListIdentityGetCipIdentityItemLength(), outgoing_message);

  AddIntToMessage(kSupportedProtocolVersion, outgoing_message);

  EncapsulateIpAddress(htons(kOpenerEthernetPort), g_tcpip.interface_configuration.ip_address, outgoing_message);

  /** Array of USINT - length 8 shall be set to zero */
  FillNextNMessageOctetsWithValueAndMoveToNextPosition(0, 8, outgoing_message);

  AddIntToMessage(g_identity.vendor_id, outgoing_message);
  AddIntToMessage(g_identity.device_type, outgoing_message);
  AddIntToMessage(g_identity.product_code, outgoing_message);
  AddSintToMessage(g_identity.revision.major_revision, outgoing_message);
  AddSintToMessage(g_identity.revision.minor_revision, outgoing_message);
  AddIntToMessage(g_identity.status, outgoing_message);
  AddDintToMessage(g_identity.serial_number, outgoing_message);
  AddSintToMessage((unsigned char) g_identity.product_name.length, outgoing_message);
//TODO Change to EncodeCipString
  memcpy(outgoing_message->current_message_position, g_identity.product_name.string, g_identity.product_name.length);
  outgoing_message->current_message_position += g_identity.product_name.length;
  outgoing_message->used_message_length += g_identity.product_name.length;

  AddSintToMessage(g_identity.state, outgoing_message);
}

void EncapsulateListIdentityResponseMessage(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message) {

  const CipUint kEncapsulationCommandListIdentityLength = ListIdentityGetCipIdentityItemLength() + sizeof(CipUint) + sizeof(CipUint) + sizeof(CipUint); /* Last element is item count */

  GenerateEncapsulationHeader(receive_data, kEncapsulationCommandListIdentityLength, 0,
  /* Session handle will be ignored by receiver */
  kEncapsulationProtocolSuccess, outgoing_message);

  AddIntToMessage(1, outgoing_message); /* Item count: one item */
  EncodeListIdentityCipIdentityItem(outgoing_message);

}

void DetermineDelayTime(const EipByte *buffer_start, DelayedEncapsulationMessage *const delayed_message_buffer) {

  buffer_start += 12; /* start of the sender context */
  EipUint16 maximum_delay_time = GetUintFromMessage((const EipUint8** const ) &buffer_start);

  if(0 == maximum_delay_time) {
    maximum_delay_time = kListIdentityDefaultDelayTime;
  } else if(kListIdentityMinimumDelayTime > maximum_delay_time) { /* if maximum_delay_time is between 1 and 500ms set it to 500ms */
    maximum_delay_time = kListIdentityMinimumDelayTime;
  }

  delayed_message_buffer->time_out = rand() % maximum_delay_time;
}

void EncapsulateRegisterSessionCommandResponseMessage(const EncapsulationData *const receive_data, const CipSessionHandle session_handle,
    const EncapsulationProtocolErrorCode encapsulation_protocol_status, ENIPMessage *const outgoing_message) {

  /* Encapsulation header */
  const size_t kListInterfacesCommandSpecificDataLength = sizeof(CipUint) + sizeof(CipUint);
  assert(kListInterfacesCommandSpecificDataLength == 4);
  GenerateEncapsulationHeader(receive_data, kListInterfacesCommandSpecificDataLength, session_handle, encapsulation_protocol_status, outgoing_message);

  AddIntToMessage(1, outgoing_message); /* protocol version*/
  AddIntToMessage(0, outgoing_message); /* Options flag, shall be set to zero */
}

/* @brief Check supported protocol, generate session handle, send replay back to originator.
 * @param socket Socket this request is associated to. Needed for double register check
 * @param receive_data Pointer to received data with request/response.
 */
void HandleReceivedRegisterSessionCommand(int socket, const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message) {
  int session_index = 0;
  CipSessionHandle session_handle = 0;
  EncapsulationProtocolErrorCode encapsulation_protocol_status = kEncapsulationProtocolSuccess;

  EipUint16 protocol_version = GetUintFromMessage((const EipUint8** const ) &receive_data->current_communication_buffer_position);
  EipUint16 option_flag = GetUintFromMessage((const EipUint8** const ) &receive_data->current_communication_buffer_position);

  /* check if requested protocol version is supported and the register session option flag is zero*/
  if((0 < protocol_version) && (protocol_version <= kSupportedProtocolVersion) && (0 == option_flag)) { /*Option field should be zero*/
    /* check if the socket has already a session open */
    for(unsigned int i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i) {
      if(g_registered_sessions[i] == socket) {
        /* the socket has already registered a session this is not allowed*/
        OPENER_TRACE_INFO(
            "Error: A session is already registered at socket %d\n",
            socket);
        session_handle = i + 1; /*return the already assigned session back, the cip spec is not clear about this needs to be tested*/
        encapsulation_protocol_status = kEncapsulationProtocolInvalidCommand;
        session_index = kSessionStatusInvalid;
        break;
      }
    }

    if(kSessionStatusInvalid != session_index) {
      session_index = GetFreeSessionIndex();
      if(kSessionStatusInvalid == session_index) /* no more sessions available */
      {
        encapsulation_protocol_status = kEncapsulationProtocolInsufficientMemory;
      } else { /* successful session registered */
        SocketTimer *socket_timer = SocketTimerArrayGetEmptySocketTimer(g_timestamps,
        OPENER_NUMBER_OF_SUPPORTED_SESSIONS);
        SocketTimerSetSocket(socket_timer, socket);
        SocketTimerSetLastUpdate(socket_timer, g_actual_time);
        g_registered_sessions[session_index] = socket; /* store associated socket */
        session_handle = (CipSessionHandle)(session_index + 1);
        encapsulation_protocol_status = kEncapsulationProtocolSuccess;
      }
    }
  } else { /* protocol not supported */
    encapsulation_protocol_status = kEncapsulationProtocolUnsupportedProtocol;
  }

  EncapsulateRegisterSessionCommandResponseMessage(receive_data, session_handle, encapsulation_protocol_status, outgoing_message);

}

/** @brief Unregister encapsulation session
 * @param receive_data Pointer to structure with data and header information.
 * @param outgoing_message The outgoing ENIP message
 * @return kEipStatusOkSend: a response needs to be sent, others: EIP stack status
 *
 * Close all corresponding TCP connections and delete session handle.
 */
EipStatus HandleReceivedUnregisterSessionCommand(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message) {
  OPENER_TRACE_INFO("encap.c: Unregister Session Command\n");
  if((0 < receive_data->session_handle) && (receive_data->session_handle <=
  OPENER_NUMBER_OF_SUPPORTED_SESSIONS)) {
    CipSessionHandle i = receive_data->session_handle - 1;
    if(kEipInvalidSocket != g_registered_sessions[i]) {
      CloseTcpSocket(g_registered_sessions[i]);
      g_registered_sessions[i] = kEipInvalidSocket;
      CloseClass3ConnectionBasedOnSession(i + 1);
      return kEipStatusOk;
    }
  }

  /* no such session registered */
  GenerateEncapsulationHeader(receive_data, 0, receive_data->session_handle, kEncapsulationProtocolInvalidSessionHandle, outgoing_message);
  return kEipStatusOkSend;
}

/** @brief Call Connection Manager.
 *  @param receive_data Pointer to structure with data and header information.
 *  @param originator_address Address of the originator as received from socket
 *  @param outgoing_message The outgoing ENIP message
 */
EipStatus HandleReceivedSendUnitDataCommand(const EncapsulationData *const receive_data, const struct sockaddr *const originator_address,
    ENIPMessage *const outgoing_message) {
  EipStatus return_value = kEipStatusOkSend;
  /*EipStatus*/return_value = kEipStatusOk; /* TODO: Shouldn't this be kEipStatusOk cause we must not send any response if data_length < 6? */

  if(receive_data->data_length >= 6) {
    /* Command specific data UDINT .. Interface Handle, UINT .. Timeout, CPF packets */
    /* don't use the data yet */
    GetDintFromMessage((const EipUint8** const ) &receive_data->current_communication_buffer_position); /* skip over null interface handle*/
    GetIntFromMessage((const EipUint8** const ) &receive_data->current_communication_buffer_position); /* skip over unused timeout value*/
    ((EncapsulationData* const ) receive_data)->data_length -= 6; /* the rest is in CPF format*/

    if(kSessionStatusValid == CheckRegisteredSessions(receive_data)) /* see if the EIP session is registered*/
    {
      return_value = NotifyConnectedCommonPacketFormat(receive_data, originator_address, outgoing_message);
    } else { /* received a package with non registered session handle */
      InitializeENIPMessage(outgoing_message);
      GenerateEncapsulationHeader(receive_data, 0, receive_data->session_handle, kEncapsulationProtocolInvalidSessionHandle, outgoing_message);
      return_value = kEipStatusOkSend; /* TODO: Needs to be here if line with first TODO of this function is adjusted. */
    }
  }
  return return_value;
}

/** @brief Call UCMM or Message Router if UCMM not implemented.
 *  @param receive_data Pointer to structure with data and header information.
 *  @param originator_address Address of the originator as received from socket
 *  @param outgoing_message The outgoing ENIP message
 *  @return status      kEipStatusOk .. success.
 *                      kEipStatusOkSend .. success & need to send response
 *                      kEipStatusError .. error
 */
EipStatus HandleReceivedSendRequestResponseDataCommand(const EncapsulationData *const receive_data, const struct sockaddr *const originator_address,
    ENIPMessage *const outgoing_message) {
  EipStatus return_value = kEipStatusOkSend;
  /* EipStatus*/return_value = kEipStatusOk; /* TODO: Shouldn't this be kEipStatusOk cause we must not send any response if data_length < 6? */

  if(receive_data->data_length >= 6) {
    /* Command specific data UDINT .. Interface Handle, UINT .. Timeout, CPF packets */
    /* don't use the data yet */
    GetDintFromMessage((const EipUint8** const ) &receive_data->current_communication_buffer_position); /* skip over null interface handle*/
    GetIntFromMessage((const EipUint8** const ) &receive_data->current_communication_buffer_position); /* skip over unused timeout value*/
    ((EncapsulationData* const ) receive_data)->data_length -= 6; /* the rest is in CPF format*/

    if(kSessionStatusValid == CheckRegisteredSessions(receive_data)) /* see if the EIP session is registered*/
    {
      return_value = NotifyCommonPacketFormat(receive_data, originator_address, outgoing_message);
    } else { /* received a package with non registered session handle */
      InitializeENIPMessage(outgoing_message);
      GenerateEncapsulationHeader(receive_data, 0, receive_data->session_handle, kEncapsulationProtocolInvalidSessionHandle, outgoing_message);
      return_value = kEipStatusOkSend; /* TODO: Needs to be here if line with first TODO of this function is adjusted. */
    }
  }
  return return_value;
}

EipStatus HandleReceivedInvalidCommand(const EncapsulationData *const receive_data, ENIPMessage *const outgoing_message) {

  /* Encapsulation header */
  GenerateEncapsulationHeader(receive_data, 0, receive_data->session_handle, kEncapsulationProtocolInvalidCommand, outgoing_message);
  return kEipStatusOkSend;

}

/** @brief search for available sessions an return index.
 *  @return return index of free session in anRegisteredSessions.
 *                      kInvalidSession .. no free session available
 */
int GetFreeSessionIndex(void) {
  for(int session_index = 0; session_index < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; session_index++) {
    if(kEipInvalidSocket == g_registered_sessions[session_index]) {
      return session_index;
    }
  }
  return kSessionStatusInvalid;
}

/** @brief copy data from pa_buf in little endian to host in structure.
 * @param receive_buffer Received message
 * @param receive_buffer_length Length of the data in receive_buffer. Might be more than one message
 * @param encapsulation_data	structure to which data shall be copied
 * @return return difference between bytes in pa_buf an data_length
 *              0 .. full package received
 *                      >0 .. more than one packet received
 *                      <0 .. only fragment of data portion received
 */
int_fast32_t CreateEncapsulationStructure(const EipUint8 *receive_buffer,
                                          size_t receive_buffer_length,
                                          EncapsulationData *const encapsulation_data)
{
  encapsulation_data->communication_buffer_start = (EipUint8*) receive_buffer;
  encapsulation_data->command_code = GetUintFromMessage(&receive_buffer);
  encapsulation_data->data_length = GetUintFromMessage(&receive_buffer);
  encapsulation_data->session_handle = GetUdintFromMessage(&receive_buffer);
  encapsulation_data->status = GetUdintFromMessage(&receive_buffer);

  memcpy(encapsulation_data->sender_context, receive_buffer, kSenderContextSize);
  receive_buffer += kSenderContextSize;
  encapsulation_data->options = GetUdintFromMessage(&receive_buffer);
  encapsulation_data->current_communication_buffer_position = (EipUint8*) receive_buffer;

  /* Ensure buffer length fits in an int32 before casting in the return expression. */
  OPENER_ASSERT(INT32_MAX >= receive_buffer_length);

  return ( (int32_t)receive_buffer_length - ENCAPSULATION_HEADER_LENGTH -
           encapsulation_data->data_length );
}

/** @brief Check if received package belongs to registered session.
 *  @param receive_data Received data.
 *  @return 0 .. Session registered
 *              kInvalidSession .. invalid session -> return unsupported command received
 */
SessionStatus CheckRegisteredSessions(const EncapsulationData *const receive_data) {

  /* Skip the check when fuzzing
   in order to increase our code coverage
   we are simply bypassing all the session checks
   */
#ifdef FUZZING_AFL
  return kSessionStatusValid;
#endif

  if((0 < receive_data->session_handle) && (receive_data->session_handle <=
  OPENER_NUMBER_OF_SUPPORTED_SESSIONS)) {
    if(kEipInvalidSocket != g_registered_sessions[receive_data->session_handle - 1]) {
      return kSessionStatusValid;
    }
  }
  return kSessionStatusInvalid;
}

void CloseSessionBySessionHandle(const CipConnectionObject *const connection_object) {
  OPENER_TRACE_INFO("encap.c: Close session by handle\n");
  CipSessionHandle session_handle = connection_object->associated_encapsulation_session;
  CloseTcpSocket(g_registered_sessions[session_handle - 1]);
  g_registered_sessions[session_handle - 1] = kEipInvalidSocket;
  OPENER_TRACE_INFO("encap.c: Close session by handle done\n");
}

void CloseSession(int socket) {
  OPENER_TRACE_INFO("encap.c: Close session\n");
  for(unsigned int i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i) {
    if(g_registered_sessions[i] == socket) {
      CloseTcpSocket(socket);
      g_registered_sessions[i] = kEipInvalidSocket;
      CloseClass3ConnectionBasedOnSession(i + 1);
      break;
    }
  }OPENER_TRACE_INFO("encap.c: Close session done\n");
}

void RemoveSession(const int socket) {
  OPENER_TRACE_INFO("encap.c: Removing session\n");
  for(unsigned int i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i) {
    if(g_registered_sessions[i] == socket) {
      g_registered_sessions[i] = kEipInvalidSocket;
      CloseClass3ConnectionBasedOnSession(i + 1);
      break;
    }
  }OPENER_TRACE_INFO("encap.c: Session removed\n");
}

void EncapsulationShutDown(void) {
  OPENER_TRACE_INFO("encap.c: Encapsulation shutdown\n");
  for(size_t i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i) {
    if(kEipInvalidSocket != g_registered_sessions[i]) {
      CloseTcpSocket(g_registered_sessions[i]);
      g_registered_sessions[i] = kEipInvalidSocket;
    }
  }
}

void ManageEncapsulationMessages(const MilliSeconds elapsed_time) {
  for(size_t i = 0; i < ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES; i++) {
    if(kEipInvalidSocket != g_delayed_encapsulation_messages[i].socket) {
      g_delayed_encapsulation_messages[i].time_out -= elapsed_time;
      if(0 >= g_delayed_encapsulation_messages[i].time_out) {
        /* If delay is reached or passed, send the UDP message */
        sendto(g_delayed_encapsulation_messages[i].socket, (char*) g_delayed_encapsulation_messages[i].outgoing_message.message_buffer,
          g_delayed_encapsulation_messages[i].outgoing_message.used_message_length, 0, (struct sockaddr*) &(g_delayed_encapsulation_messages[i].receiver),
          sizeof(struct sockaddr));
        g_delayed_encapsulation_messages[i].socket = kEipInvalidSocket;
      }
    }
  }
}

void CloseEncapsulationSessionBySockAddr(const CipConnectionObject *const connection_object) {
  for(size_t i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i) {
    if(kEipInvalidSocket != g_registered_sessions[i]) {
      struct sockaddr_in encapsulation_session_addr = { 0 };
      socklen_t addrlength = sizeof(encapsulation_session_addr);
      if(getpeername(g_registered_sessions[i], (struct sockaddr*) &encapsulation_session_addr, &addrlength) < 0) { /* got error */
        int error_code = GetSocketErrorNumber();
        char *error_message = GetErrorMessage(error_code);
        OPENER_TRACE_ERR(
            "encap.c: error on getting peer name on closing session: %d - %s\n",
            error_code, error_message);
        FreeErrorMessage(error_message);
      }
      if(encapsulation_session_addr.sin_addr.s_addr == connection_object->originator_address.sin_addr.s_addr) {
        CloseSession(g_registered_sessions[i]);
      }
    }
  }
}

CipSessionHandle GetSessionFromSocket(const int socket_handle) {
  for(CipSessionHandle i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i) {
    if(socket_handle == g_registered_sessions[i]) {
      return i;
    }
  }
  return OPENER_NUMBER_OF_SUPPORTED_SESSIONS;
}

void CloseClass3ConnectionBasedOnSession(CipSessionHandle encapsulation_session_handle) {
  DoublyLinkedListNode *node = connection_list.first;
  while(NULL != node) {
    CipConnectionObject *connection_object = node->data;
    if(kConnectionObjectTransportClassTriggerTransportClass3 == ConnectionObjectGetTransportClassTriggerTransportClass(connection_object)
      && connection_object->associated_encapsulation_session == encapsulation_session_handle) {
      connection_object->connection_close_function(connection_object);
    }
    node = node->next;
  }
}
