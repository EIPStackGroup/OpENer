/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>
#include <stdbool.h>

#include "cipioconnection.h"

#include "generic_networkhandler.h"
#include "cipconnectionmanager.h"
#include "cipassembly.h"
#include "ciptcpipinterface.h"
#include "cipcommon.h"
#include "appcontype.h"
#include "cpf.h"
#include "trace.h"
#include "endianconv.h"


/*The port to be used per default for I/O messages on UDP.*/
const int kOpenerEipIoUdpPort = 0x08AE;

/* producing multicast connection have to consider the rules that apply for
 * application connection types.
 */
EipStatus OpenProducingMulticastConnection(
  CipConnectionObject *connection_object,
  CipCommonPacketFormatData *common_packet_format_data);

EipStatus OpenMulticastConnection(
  UdpCommuncationDirection direction,
  CipConnectionObject *connection_object,
  CipCommonPacketFormatData *common_packet_format_data);

EipStatus OpenConsumingPointToPointConnection(
  CipConnectionObject *const connection_object,
  CipCommonPacketFormatData *const common_packet_format_data);

EipStatus OpenProducingPointToPointConnection(
  CipConnectionObject *connection_object,
  CipCommonPacketFormatData *common_packet_format_data);

EipUint16 HandleConfigData(CipConnectionObject *connection_object);

/* Regularly close the IO connection. If it is an exclusive owner or input only
 * connection and in charge of the connection a new owner will be searched
 */
void CloseIoConnection(CipConnectionObject *connection_object);

void HandleIoConnectionTimeOut(CipConnectionObject *connection_object);

/** @brief  Send the data from the produced CIP Object of the connection via the socket of the connection object
 *   on UDP.
 *      @param connection_object  pointer to the connection object
 *      @return status  EIP_OK .. success
 *                     EIP_ERROR .. error
 */
EipStatus SendConnectedData(CipConnectionObject *connection_object);

EipStatus HandleReceivedIoConnectionData(
  CipConnectionObject *connection_object,
  const EipUint8 *data,
  EipUint16 data_length);

/**** Global variables ****/
EipUint8 *g_config_data_buffer = NULL; /**< buffers for the config data coming with a forward open request. */
unsigned int g_config_data_length = 0; /**< length of g_config_data_buffer. Initialized with 0 */

EipUint32 g_run_idle_state; /**< buffer for holding the run idle information. */

EipUint16 ProcessProductionInhibitTime(CipConnectionObject *io_connection_object)
{
  if ( kConnectionObjectTransportClassTriggerProductionTriggerCyclic
       == ConnectionObjectGetTransportClassTriggerProductionTrigger(
         io_connection_object) ) {
    if ( 256 ==
         ConnectionObjectGetProductionInhibitTime(io_connection_object) ) {
      OPENER_TRACE_INFO("No PIT segment available\n");
      /* there was no PIT segment in the connection path; set PIT to one fourth of RPI */
      ConnectionObjectSetProductionInhibitTime(io_connection_object,
                                               ConnectionObjectGetTToORequestedPacketInterval(
                                                 io_connection_object)
                                               / 4000);
    } else {
      /* If a production inhibit time is provided, it needs to be smaller than the Requested Packet Interval */
      if ( ConnectionObjectGetProductionInhibitTime(io_connection_object)
           > (ConnectionObjectGetTToORequestedPacketInterval(
                io_connection_object)
              / 1000) ) {
        /* see section C-1.4.3.3 */
        return kConnectionManagerExtendedStatusCodeRpiNotSupported; /**< RPI not supported. Extended Error code deprecated */
      }
    }
  }
  return kConnectionManagerExtendedStatusCodeSuccess;
}

void SetIoConnectionCallbacks(CipConnectionObject *const io_connection_object) {
  io_connection_object->connection_close_function = CloseIoConnection;
  io_connection_object->connection_timeout_function = HandleIoConnectionTimeOut;
  io_connection_object->connection_send_data_function = SendConnectedData;
  io_connection_object->connection_receive_data_function =
    HandleReceivedIoConnectionData;
}

EipUint16 SetupIoConnectionOriginatorToTargetConnectionPoint(
  CipConnectionObject *const io_connection_object,
  CipConnectionObject *const RESTRICT connection_object
  ) {
  CipClass *const assembly_class = GetCipClass(kCipAssemblyClassCode);
  CipInstance *instance = NULL;
  if ( NULL
       != ( instance =
              GetCipInstance(
                assembly_class,
                io_connection_object->consumed_path.instance_id) ) ) {
    /* consuming Connection Point is present */
    io_connection_object->consuming_instance = instance;
    io_connection_object->consumed_connection_path_length = 6;
    /*io_connection_object->consumed_path.class_id =
       io_connection_object->connection_path.class_id;
       io_connection_object->consumed_connection_path.instance_number =
       io_connection_object->connection_path.connection_point[
        kConnectionPointConsumer];*/
    io_connection_object->consumed_path.attribute_id_or_connection_point = 3;
    int data_size = ConnectionObjectGetOToTConnectionSize(io_connection_object);
    int diff_size = 0;

    /* an assembly object should always have an attribute 3 */
    CipAttributeStruct *attribute = GetCipAttribute(instance,
                                                    io_connection_object->consumed_path.attribute_id_or_connection_point);
    OPENER_ASSERT(attribute != NULL);
    bool is_heartbeat = ( ( (CipByteArray *) attribute->data )->length == 0 );
    if ( kConnectionObjectTransportClassTriggerTransportClass1
         == ConnectionObjectGetTransportClassTriggerTransportClass(
           io_connection_object) ) {
      /* class 1 connection */
      data_size -= 2; /* remove 16-bit sequence count length */
      diff_size += 2;
    }
    if ( (kOpenerConsumedDataHasRunIdleHeader) && (data_size > 0)
         && (!is_heartbeat) ) {
      /* we only have an run idle header if it is not an heartbeat connection */
      data_size -= 4; /* remove the 4 bytes needed for run/idle header */
      diff_size += 4;
    }
    if ( ( (CipByteArray *) attribute->data )->length != data_size ) {
      /*wrong connection size */
      connection_object->correct_originator_to_target_size =
        ( (CipByteArray *) attribute->data )->length + diff_size;
      return kConnectionManagerExtendedStatusCodeErrorInvalidOToTConnectionSize;
    }
  } else {
    return kConnectionManagerExtendedStatusCodeInvalidConsumingApplicationPath;
  }
  return kConnectionManagerExtendedStatusCodeSuccess;
}

EipUint16 SetupIoConnectionTargetToOriginatorConnectionPoint(
  CipConnectionObject *const io_connection_object,
  CipConnectionObject *const RESTRICT connection_object
  ) {
  DoublyLinkedListNode *node = connection_list.first;
  while (NULL != node &&
         kConnectionObjectConnectionTypeMulticast ==
         ConnectionObjectGetTToOConnectionType(io_connection_object) ) {
    CipConnectionObject *iterator = node->data;
    if(io_connection_object->produced_path.instance_id ==
       iterator->produced_path.instance_id) {
      //Check parameters
      if( ConnectionObjectGetTToORequestedPacketInterval(io_connection_object)
          !=
          ConnectionObjectGetTToORequestedPacketInterval(iterator) ) {
        return kConnectionManagerExtendedStatusCodeErrorRpiValuesNotAcceptable;
      }
      if( ConnectionObjectGetTToOConnectionSizeType(
            io_connection_object) !=
          ConnectionObjectGetTToOConnectionSizeType(
            iterator) ) {
        return
          kConnectionManagerExtendedStatusCodeMismatchedTToONetworkConnectionFixVar;
      }
      if ( ConnectionObjectGetTToOPriority(io_connection_object) !=
           ConnectionObjectGetTToOPriority(iterator) ) {
        return
          kConnectionManagerExtendedStatusCodeMismatchedTToONetworkConnectionPriority;
      }

      if( ConnectionObjectGetTransportClassTriggerTransportClass(
            io_connection_object) !=
          ConnectionObjectGetTransportClassTriggerTransportClass(iterator) ) {
        return kConnectionManagerExtendedStatusCodeMismatchedTransportClass;
      }


      if( ConnectionObjectGetTransportClassTriggerProductionTrigger(
            io_connection_object) !=
          ConnectionObjectGetTransportClassTriggerProductionTrigger(iterator) )
      {
        return
          kConnectionManagerExtendedStatusCodeMismatchedTToOProductionTrigger;
      }

      if( ConnectionObjectGetProductionInhibitTime(io_connection_object) !=
          ConnectionObjectGetProductionInhibitTime(iterator) ) {
        return
          kConnectionManagerExtendedStatusCodeMismatchedTToOProductionInhibitTimeSegment;
      }

    }

    node = node->next;
  }

  /*setup producer side*/
  CipClass *const assembly_class = GetCipClass(kCipAssemblyClassCode);
  CipInstance *instance = NULL;
  if ( NULL
       != ( instance =
              GetCipInstance(
                assembly_class,
                io_connection_object->produced_path.instance_id) ) ) {

    io_connection_object->producing_instance = instance;
    int data_size = ConnectionObjectGetTToOConnectionSize(io_connection_object);
    int diff_size = 0;
    /* an assembly object should always have an attribute 3 */
    io_connection_object->produced_path.attribute_id_or_connection_point = 3;
    CipAttributeStruct *attribute = GetCipAttribute(instance,
                                                    io_connection_object->produced_path.attribute_id_or_connection_point);
    OPENER_ASSERT(attribute != NULL);
    bool is_heartbeat = ( ( (CipByteArray *) attribute->data )->length == 0 );
    if ( kConnectionObjectTransportClassTriggerTransportClass1 ==
         ConnectionObjectGetTransportClassTriggerTransportClass(
           io_connection_object) ) {
      /* class 1 connection */
      data_size -= 2; /* remove 16-bit sequence count length */
      diff_size += 2;
    }
    if ( (kOpenerProducedDataHasRunIdleHeader) && (data_size > 0)
         && (!is_heartbeat) ) {
      /* we only have an run idle header if it is not an heartbeat connection */
      data_size -= 4; /* remove the 4 bytes needed for run/idle header */
      diff_size += 4;
    }
    if ( ( (CipByteArray *) attribute->data )->length != data_size ) {
      /*wrong connection size*/
      connection_object->correct_target_to_originator_size =
        ( (CipByteArray *) attribute->data )->length + diff_size;
      return kConnectionManagerExtendedStatusCodeErrorInvalidTToOConnectionSize;
    }
  } else {
    return kConnectionManagerExtendedStatusCodeInvalidProducingApplicationPath;
  }
  return kConnectionManagerExtendedStatusCodeSuccess;
}

/** @brief Establishes a new IO Type 1 Connection
 *
 * This function needs the guarantee that no Null request will be passed to it.
 * It will generate a new IO connection based on the data parsed in the Forward Open service
 *
 * @param connection_object pointer to the connection object structure holding the parsed data from the forward open request
 * @param extended_error the extended error code in case an error happened
 * @return general status on the establishment
 *    - kEipStatusOk ... on success
 *    - On an error the general status code to be put into the response
 */
EipStatus EstablishIoConnection(
  CipConnectionObject *RESTRICT const connection_object,
  EipUint16 *const extended_error
  ) {
  EipStatus eip_status = kEipStatusOk;

  CipConnectionObject *io_connection_object = GetIoConnectionForConnectionData(
    connection_object,
    extended_error);
  if(NULL == io_connection_object) {
    return kCipErrorConnectionFailure;
  }

  *extended_error = ProcessProductionInhibitTime(io_connection_object);

  if(0 != *extended_error) {
    return kCipErrorConnectionFailure;
  }

  SetIoConnectionCallbacks(io_connection_object);

  ConnectionObjectGeneralConfiguration(io_connection_object);

  ConnectionObjectConnectionType originator_to_target_connection_type =
    ConnectionObjectGetOToTConnectionType(io_connection_object);
  ConnectionObjectConnectionType target_to_originator_connection_type =
    ConnectionObjectGetTToOConnectionType(io_connection_object);

  /** Already handled by forward open */
  OPENER_ASSERT( !(originator_to_target_connection_type ==
                   kConnectionObjectConnectionTypeNull &&
                   target_to_originator_connection_type ==
                   kConnectionObjectConnectionTypeNull) );

  io_connection_object->consuming_instance = NULL;
  io_connection_object->consumed_connection_path_length = 0;
  io_connection_object->producing_instance = NULL;
  io_connection_object->produced_connection_path_length = 0;


  /* we don't need to check for zero as this is handled in the connection path parsing */

  if (originator_to_target_connection_type !=
      kConnectionObjectConnectionTypeNull) {                                         /*setup consumer side*/
    *extended_error = SetupIoConnectionOriginatorToTargetConnectionPoint(
      io_connection_object,
      connection_object);
    if (kConnectionManagerExtendedStatusCodeSuccess != *extended_error) {
      return kCipErrorConnectionFailure;
    }
  }


  if (target_to_originator_connection_type !=
      kConnectionObjectConnectionTypeNull) {                                         /*setup producer side*/
    *extended_error = SetupIoConnectionTargetToOriginatorConnectionPoint(
      io_connection_object,
      connection_object);
    if (kConnectionManagerExtendedStatusCodeSuccess != *extended_error) {
      return kCipErrorConnectionFailure;
    }
  }


  if (NULL != g_config_data_buffer) { /* config data has been sent with this forward open request */
    *extended_error = HandleConfigData(io_connection_object);
    if (kConnectionManagerExtendedStatusCodeSuccess != *extended_error) {
      return kCipErrorConnectionFailure;
    }
  }

  eip_status = OpenCommunicationChannels(io_connection_object);
  if (kEipStatusOk != eip_status) {
    *extended_error = 0; /*TODO find out the correct extended error code*/
    return eip_status;
  }

  AddNewActiveConnection(io_connection_object);
  CheckIoConnectionEvent(
    io_connection_object->consumed_path.instance_id,
    io_connection_object->produced_path.instance_id,
    kIoConnectionEventOpened);
  return eip_status;
}

/** @brief Open a Point2Point connection dependent on pa_direction.
 *
 * @param connection_object Pointer to registered Object in ConnectionManager.
 * @param common_packet_format_data Index of the connection object
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus OpenConsumingPointToPointConnection(
  CipConnectionObject *const connection_object,
  CipCommonPacketFormatData *const common_packet_format_data
  ) {

  int j = 0;

  if (common_packet_format_data->address_info_item[0].type_id == 0) { /* it is not used yet */
    j = 0;
  } else if (common_packet_format_data->address_info_item[1].type_id == 0) {
    j = 1;
  }

  struct sockaddr_in addr =
  { .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(
      kOpenerEipIoUdpPort) };

  CipUsint qos_for_socket = ConnectionObjectGetTToOPriority(connection_object);
  int socket = CreateUdpSocket(kUdpCommuncationDirectionConsuming,
                               &addr,
                               qos_for_socket);                                            /* the address is only needed for bind used if consuming */
  if (socket == kEipInvalidSocket) {
    OPENER_TRACE_ERR(
      "cannot create UDP socket in OpenPointToPointConnection\n");
    return kEipStatusError;
  }

  connection_object->originator_address = addr; /* store the address of the originator for packet scanning */
  addr.sin_addr.s_addr = INADDR_ANY; /* restore the address */
  connection_object->socket[kUdpCommuncationDirectionConsuming] = socket;

  common_packet_format_data->address_info_item[j].length = 16;
  common_packet_format_data->address_info_item[j].type_id =
    kCipItemIdSocketAddressInfoOriginatorToTarget;

  common_packet_format_data->address_info_item[j].sin_port = addr.sin_port;
  /*TODO should we add our own address here? */
  common_packet_format_data->address_info_item[j].sin_addr = addr.sin_addr
                                                             .s_addr;
  memset(common_packet_format_data->address_info_item[j].nasin_zero, 0, 8);
  common_packet_format_data->address_info_item[j].sin_family = htons(AF_INET);

  return kEipStatusOk;
}

EipStatus OpenProducingPointToPointConnection(
  CipConnectionObject *connection_object,
  CipCommonPacketFormatData *common_packet_format_data
  ) {
  in_port_t port = htons(kOpenerEipIoUdpPort); /* the default port to be used if no port information is part of the forward open request */

  if (kCipItemIdSocketAddressInfoTargetToOriginator
      == common_packet_format_data->address_info_item[0].type_id) {
    port = common_packet_format_data->address_info_item[0].sin_port;
  } else {
    if (kCipItemIdSocketAddressInfoTargetToOriginator
        == common_packet_format_data->address_info_item[1].type_id) {
      port = common_packet_format_data->address_info_item[1].sin_port;
    }
  }

  connection_object->remote_address.sin_family = AF_INET;
  connection_object->remote_address.sin_addr.s_addr = 0; /* we don't know the address of the originate will be set in the IApp_CreateUDPSocket */
  connection_object->remote_address.sin_port = port;

  CipUsint qos_for_socket = ConnectionObjectGetTToOPriority(connection_object);
  int socket = CreateUdpSocket(kUdpCommuncationDirectionProducing,
                               &connection_object->remote_address,
                               qos_for_socket);                                     /* the address is only needed for bind used if consuming */
  if (socket == kEipInvalidSocket) {
    OPENER_TRACE_ERR(
      "cannot create UDP socket in OpenPointToPointConnection\n");
    /* *pa_pnExtendedError = 0x0315; miscellaneous*/
    return kCipErrorConnectionFailure;
  }
  connection_object->socket[kUdpCommuncationDirectionProducing] = socket;

  return kEipStatusOk;
}

EipStatus OpenProducingMulticastConnection(
  CipConnectionObject *connection_object,
  CipCommonPacketFormatData *common_packet_format_data
  ) {
  CipConnectionObject *existing_connection_object =
    GetExistingProducerMulticastConnection(
      connection_object->produced_path.instance_id);

  int j = 0; /* allocate an unused sockaddr struct to use */
  if (g_common_packet_format_data_item.address_info_item[0].type_id == 0) { /* it is not used yet */
    j = 0;
  } else if (g_common_packet_format_data_item.address_info_item[1].type_id
             == 0) {
    j = 1;
  }

  common_packet_format_data->address_info_item[j].type_id =
    kCipItemIdSocketAddressInfoTargetToOriginator;

  if (NULL == existing_connection_object) { /* we are the first connection producing for the given Input Assembly */
    return OpenMulticastConnection(kUdpCommuncationDirectionProducing,
                                   connection_object,
                                   common_packet_format_data);
  } else {
    /* we need to inform our originator on the correct connection id */
    connection_object->cip_produced_connection_id = existing_connection_object
                                                    ->
                                                    cip_produced_connection_id;
  }

  /* we have a connection reuse the data and the socket */

  if (kConnectionObjectInstanceTypeIOExclusiveOwner ==
      connection_object->instance_type) {
    /* exclusive owners take the socket and further manage the connection
     * especially in the case of time outs.
     */
    connection_object->socket[kUdpCommuncationDirectionProducing] =
      existing_connection_object->socket[kUdpCommuncationDirectionProducing];
    existing_connection_object->socket[kUdpCommuncationDirectionProducing] =
      kEipInvalidSocket;
  } else { /* this connection will not produce the data */
    connection_object->socket[kUdpCommuncationDirectionProducing] =
      kEipInvalidSocket;
  }

  common_packet_format_data->address_info_item[j].length = 16;

  connection_object->remote_address.sin_family = AF_INET;
  connection_object->remote_address.sin_port = common_packet_format_data
                                               ->address_info_item[j].sin_port
                                                 = htons(kOpenerEipIoUdpPort);
  connection_object->remote_address.sin_addr.s_addr = common_packet_format_data
                                                      ->address_info_item[j].
                                                      sin_addr =
                                                        g_multicast_configuration
                                                        .
                                                        starting_multicast_address;
  memset(common_packet_format_data->address_info_item[j].nasin_zero, 0, 8);
  common_packet_format_data->address_info_item[j].sin_family = htons(AF_INET);

  return kEipStatusOk;
}

/** @brief Open a Multicast connection dependent on @var direction.
 *
 * @param direction Flag to indicate if consuming or producing.
 * @param connection_object  pointer to registered Object in ConnectionManager.
 * @param common_packet_format_data     received CPF Data Item.
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus OpenMulticastConnection(
  UdpCommuncationDirection direction,
  CipConnectionObject *connection_object,
  CipCommonPacketFormatData *common_packet_format_data
  ) {
  int j = -1;

  int address_info_item_which_contains_o_to_t = -1;
  int address_info_item_which_contains_t_to_o = -1;

  if(kCipItemIdSocketAddressInfoOriginatorToTarget
     == common_packet_format_data->address_info_item[0].type_id) {
    address_info_item_which_contains_o_to_t = 0;
  } else if(kCipItemIdSocketAddressInfoOriginatorToTarget
            == common_packet_format_data->address_info_item[1].type_id) {
    address_info_item_which_contains_o_to_t = 1;
  } else {
    OPENER_TRACE_INFO("No O->T Sockaddr info available\n");
  }

  if(kCipItemIdSocketAddressInfoTargetToOriginator
     == common_packet_format_data->address_info_item[0].type_id) {
    address_info_item_which_contains_t_to_o = 0;
  } else if(kCipItemIdSocketAddressInfoTargetToOriginator
            == common_packet_format_data->address_info_item[1].type_id) {
    address_info_item_which_contains_t_to_o = 1;
  } else {
    OPENER_TRACE_INFO("No T->O Sockaddr info available\n");
  }

  if(kUdpCommuncationDirectionConsuming == direction) {
    //OPENER_ASSERT(-1 != address_info_item_which_contains_o_to_t);
    j = address_info_item_which_contains_o_to_t;
  }

  if(kUdpCommuncationDirectionProducing == direction) {
    //OPENER_ASSERT(-1 != address_info_item_which_contains_o_to_t);
    j = address_info_item_which_contains_t_to_o;
  }

  /*****************/

  if (-1 == j) {
    OPENER_TRACE_ERR(
      "no suitable addr info item available / O->T: %d, T->O: %d, Selector: %d, direction: %d\n",
      address_info_item_which_contains_o_to_t,
      address_info_item_which_contains_t_to_o,
      j,
      direction);
    return kEipStatusError;
  }

  if (kCipItemIdSocketAddressInfoTargetToOriginator ==
      common_packet_format_data->address_info_item[j].type_id) {                                                  /* we are using an unused item initialize it with the default multicast address */
    common_packet_format_data->address_info_item[j].sin_family = htons(
      AF_INET);
    common_packet_format_data->address_info_item[j].sin_port = htons(
      kOpenerEipIoUdpPort);
    common_packet_format_data->address_info_item[j].sin_addr =
      g_multicast_configuration.starting_multicast_address;
    memset(common_packet_format_data->address_info_item[j].nasin_zero, 0, 8);
    common_packet_format_data->address_info_item[j].length = 16;
  }

  if (htons(AF_INET)
      != common_packet_format_data->address_info_item[j].sin_family) {
    OPENER_TRACE_ERR(
      "Sockaddr Info Item with wrong sin family value received\n");
    return kEipStatusError;
  }

  /* allocate an unused sockaddr struct to use */
  struct sockaddr_in socket_address = {0};
  socket_address.sin_family = ntohs(
    common_packet_format_data->address_info_item[j].sin_family);
  socket_address.sin_addr.s_addr =
    common_packet_format_data->address_info_item[j].sin_addr;
  socket_address.sin_port = common_packet_format_data->address_info_item[j]
                            .sin_port;

  CipUsint qos_for_socket = ConnectionObjectGetTToOPriority(connection_object);
  int socket = CreateUdpSocket(direction, &socket_address, qos_for_socket); /* the address is only needed for bind used if consuming */
  if (socket == kEipInvalidSocket) {
    OPENER_TRACE_ERR("cannot create UDP socket in OpenMulticastConnection\n");
    return kEipStatusError;
  }
  connection_object->socket[direction] = socket;

  if (direction == kUdpCommuncationDirectionConsuming) {
    common_packet_format_data->address_info_item[j].type_id =
      kCipItemIdSocketAddressInfoOriginatorToTarget;
    connection_object->originator_address = socket_address;
  } else {
    common_packet_format_data->address_info_item[j].type_id =
      kCipItemIdSocketAddressInfoTargetToOriginator;
    connection_object->remote_address = socket_address;
  }

  return kEipStatusOk;
}

EipUint16 HandleConfigData(CipConnectionObject *connection_object) {

  CipClass *const assembly_class = GetCipClass(kCipAssemblyClassCode);
  EipUint16 connection_manager_status = 0;
  CipInstance *config_instance = GetCipInstance(
    assembly_class, connection_object->configuration_path.instance_id);

  if (0 != g_config_data_length) {
    OPENER_ASSERT(NULL != config_instance);
    if ( ConnectionWithSameConfigPointExists(
           connection_object->configuration_path.instance_id) ) {
      /* there is a connected connection with the same config point
       * we have to have the same data as already present in the config point*/
      CipAttributeStruct *attribute_three = GetCipAttribute(
        config_instance,
        3);
      OPENER_ASSERT(NULL != attribute_three);
      CipByteArray *attribute_three_data =
        (CipByteArray *) attribute_three->data;
      OPENER_ASSERT(NULL != attribute_three_data);
      if (attribute_three_data->length != g_config_data_length) {
        connection_manager_status =
          kConnectionManagerExtendedStatusCodeErrorOwnershipConflict;
        OPENER_TRACE_INFO(
          "Hit an Ownership conflict in cipioconnection.c occurrence 1");
      } else {
        /*FIXME check if this is correct */
        if ( memcmp(attribute_three_data->data, g_config_data_buffer,
                    g_config_data_length) ) {
          connection_manager_status =
            kConnectionManagerExtendedStatusCodeErrorOwnershipConflict;
          OPENER_TRACE_INFO(
            "Hit an Ownership conflict in cipioconnection.c occurrence 2");
        }
      }
    } else {
      /* put the data on the configuration assembly object with the current
         design this can be done rather efficiently */
      if ( kEipStatusOk
           != NotifyAssemblyConnectedDataReceived(config_instance,
                                                  g_config_data_buffer,
                                                  g_config_data_length) ) {
        OPENER_TRACE_WARN("Configuration data was invalid\n");
        connection_manager_status =
          kConnectionManagerExtendedStatusCodeInvalidConfigurationApplicationPath;
      }
    }
  }
  return connection_manager_status;
}

void CloseIoConnection(CipConnectionObject *connection_object) {

  CheckIoConnectionEvent(connection_object->consumed_path.instance_id,
                         connection_object->produced_path.instance_id,
                         kIoConnectionEventClosed);

  if ( kConnectionObjectInstanceTypeIOExclusiveOwner ==
       ConnectionObjectGetInstanceType(connection_object)
       || kConnectionObjectInstanceTypeIOInputOnly ==
       ConnectionObjectGetInstanceType(connection_object) ) {
    if ( ( kConnectionObjectConnectionTypeMulticast
           == ConnectionObjectGetTToOConnectionType(connection_object) )
         && (kEipInvalidSocket
             != connection_object->socket[kUdpCommuncationDirectionProducing]) )
    {
      CipConnectionObject *next_non_control_master_connection =
        GetNextNonControlMasterConnection(
          connection_object->produced_path.instance_id);
      if (NULL != next_non_control_master_connection) {

        /* Transfer socket ownership */
        next_non_control_master_connection->socket[
          kUdpCommuncationDirectionProducing] =
          connection_object->socket[kUdpCommuncationDirectionProducing];

        connection_object->socket[kUdpCommuncationDirectionProducing] =
          kEipInvalidSocket;
        /* End */

        memcpy( &(next_non_control_master_connection->remote_address),
                &(connection_object->remote_address),
                sizeof(next_non_control_master_connection->remote_address) );
        next_non_control_master_connection->eip_level_sequence_count_producing
          =
            connection_object->eip_level_sequence_count_producing;
        next_non_control_master_connection->sequence_count_producing =
          connection_object->sequence_count_producing;
        next_non_control_master_connection->transmission_trigger_timer =
          connection_object->transmission_trigger_timer;
      } else { /* this was the last master connection close all listen only connections listening on the port */
        CloseAllConnectionsForInputWithSameType(
          connection_object->produced_path.instance_id,
          kConnectionObjectInstanceTypeIOListenOnly);
      }
    }
  }

  CloseCommunicationChannelsAndRemoveFromActiveConnectionsList(
    connection_object);
}

void HandleIoConnectionTimeOut(CipConnectionObject *connection_object) {
  CheckIoConnectionEvent(connection_object->produced_path.instance_id,
                         connection_object->consumed_path.instance_id,
                         kIoConnectionEventTimedOut);

  if(connection_object->last_package_watchdog_timer ==
     connection_object->inactivity_watchdog_timer) {
    CheckForTimedOutConnectionsAndCloseTCPConnections(connection_object,
                                                      CloseEncapsulationSessionBySockAddr);
  }

  if ( kConnectionObjectConnectionTypeMulticast
       == ConnectionObjectGetTToOConnectionType(connection_object) ) {
    switch (ConnectionObjectGetInstanceType(connection_object) ) {
      case kConnectionObjectInstanceTypeIOExclusiveOwner:
        CloseAllConnectionsForInputWithSameType(
          connection_object->produced_path.instance_id,
          kConnectionObjectInstanceTypeIOInputOnly);
        CloseAllConnectionsForInputWithSameType(
          connection_object->produced_path.instance_id,
          kConnectionObjectInstanceTypeIOListenOnly);
        break;
      case kConnectionObjectInstanceTypeIOInputOnly:
        if (kEipInvalidSocket
            != connection_object->socket[kUdpCommuncationDirectionProducing]) { /* we are the controlling input only connection find a new controller*/
          CipConnectionObject *next_non_control_master_connection =
            GetNextNonControlMasterConnection(
              connection_object->produced_path.instance_id);
          if (NULL != next_non_control_master_connection) {
            next_non_control_master_connection->socket[
              kUdpCommuncationDirectionProducing] =
              connection_object->socket[kUdpCommuncationDirectionProducing];
            connection_object->socket[kUdpCommuncationDirectionProducing] =
              kEipInvalidSocket;
            next_non_control_master_connection->transmission_trigger_timer =
              connection_object->transmission_trigger_timer;
          } else { /* this was the last master connection close all listen only connections listening on the port */
            CloseAllConnectionsForInputWithSameType(
              connection_object->produced_path.instance_id,
              kConnectionObjectInstanceTypeIOListenOnly);
          }
        }
        break;
      default:
        break;
    }
  }

  ConnectionObjectSetState(connection_object, kConnectionObjectStateTimedOut);
}

EipStatus SendConnectedData(CipConnectionObject *connection_object) {

  /* TODO think of adding an own send buffer to each connection object in order to preset up the whole message on connection opening and just change the variable data items e.g., sequence number */

  CipCommonPacketFormatData *common_packet_format_data =
    &g_common_packet_format_data_item;                                                      /* TODO think on adding a CPF data item to the S_CIP_ConnectionObject in order to remove the code here or even better allocate memory in the connection object for storing the message to send and just change the application data*/

  connection_object->eip_level_sequence_count_producing++;

  /* assembleCPFData */
  common_packet_format_data->item_count = 2;
  if ( kConnectionObjectTransportClassTriggerTransportClass0 !=
       ConnectionObjectGetTransportClassTriggerTransportClass(connection_object) )
  {                                                                                /* use Sequenced Address Items if not Connection Class 0 */
    common_packet_format_data->address_item.type_id =
      kCipItemIdSequencedAddressItem;
    common_packet_format_data->address_item.length = 8;
    common_packet_format_data->address_item.data.sequence_number =
      connection_object->eip_level_sequence_count_producing;
  } else {
    common_packet_format_data->address_item.type_id =
      kCipItemIdConnectionAddress;
    common_packet_format_data->address_item.length = 4;

  }
  common_packet_format_data->address_item.data.connection_identifier =
    connection_object->cip_produced_connection_id;

  common_packet_format_data->data_item.type_id = kCipItemIdConnectedDataItem;

  CipByteArray *producing_instance_attributes =
    (CipByteArray *) connection_object->producing_instance->attributes->data;
  common_packet_format_data->data_item.length = 0;

  /* notify the application that data will be sent immediately after the call */
  if ( BeforeAssemblyDataSend(connection_object->producing_instance) ) {
    /* the data has changed increase sequence counter */
    connection_object->sequence_count_producing++;
  }

  /* set AddressInfo Items to invalid Type */
  common_packet_format_data->address_info_item[0].type_id = 0;
  common_packet_format_data->address_info_item[1].type_id = 0;

  EipUint16 reply_length = AssembleIOMessage(common_packet_format_data,
                                             &g_message_data_reply_buffer[0]);

  EipUint8 *message_data_reply_buffer =
    &g_message_data_reply_buffer[reply_length - 2];
  common_packet_format_data->data_item.length = producing_instance_attributes
                                                ->length;
  if (kOpenerProducedDataHasRunIdleHeader) {
    common_packet_format_data->data_item.length += 4;
  }

  if (kConnectionObjectTransportClassTriggerTransportClass1 ==
      ConnectionObjectGetTransportClassTriggerTransportClass(connection_object) )
  {
    common_packet_format_data->data_item.length += 2;
    AddIntToMessage(common_packet_format_data->data_item.length,
                    &message_data_reply_buffer);
    AddIntToMessage(connection_object->sequence_count_producing,
                    &message_data_reply_buffer);
  } else {
    AddIntToMessage(common_packet_format_data->data_item.length,
                    &message_data_reply_buffer);
  }

  if (kOpenerProducedDataHasRunIdleHeader) {
    AddDintToMessage( g_run_idle_state, &(message_data_reply_buffer) );
  }

  memcpy(message_data_reply_buffer, producing_instance_attributes->data,
         producing_instance_attributes->length);

  reply_length += common_packet_format_data->data_item.length;

  return SendUdpData(
    &connection_object->remote_address,
    connection_object->socket[kUdpCommuncationDirectionProducing],
    &g_message_data_reply_buffer[0], reply_length);
}

EipStatus HandleReceivedIoConnectionData(
  CipConnectionObject *connection_object,
  const EipUint8 *data,
  EipUint16 data_length
  ) {

  /* check class 1 sequence number*/
  if (kConnectionObjectTransportClassTriggerTransportClass1 ==
      ConnectionObjectGetTransportClassTriggerTransportClass(connection_object) )
  {
    EipUint16 sequence_buffer = GetIntFromMessage( &(data) );
    if ( SEQ_LEQ16(sequence_buffer,
                   connection_object->sequence_count_consuming) ) {
      return kEipStatusOk; /* no new data for the assembly */
    }
    connection_object->sequence_count_consuming = sequence_buffer;
    data_length -= 2;
  }

  if (data_length > 0) {
    /* we have no heartbeat connection */
    if (kOpenerConsumedDataHasRunIdleHeader) {
      EipUint32 nRunIdleBuf = GetDintFromMessage( &(data) );
      if (g_run_idle_state != nRunIdleBuf) {
        RunIdleChanged(nRunIdleBuf);
      }
      g_run_idle_state = nRunIdleBuf;
      data_length -= 4;
    }

    if (NotifyAssemblyConnectedDataReceived(
          connection_object->consuming_instance, (EipUint8 *const)data,
          data_length) != 0) {
      return kEipStatusError;
    }
  }
  return kEipStatusOk;
}

EipStatus OpenCommunicationChannels(CipConnectionObject *connection_object) {

  EipStatus eip_status = kEipStatusOk;
  /*get pointer to the CPF data, currently we have just one global instance of the struct. This may change in the future*/
  CipCommonPacketFormatData *common_packet_format_data =
    &g_common_packet_format_data_item;

  ConnectionObjectConnectionType originator_to_target_connection_type =
    ConnectionObjectGetOToTConnectionType(connection_object);

  ConnectionObjectConnectionType target_to_originator_connection_type =
    ConnectionObjectGetTToOConnectionType(connection_object);

  /* open a connection "point to point" or "multicast" based on the ConnectionParameter */
  if (originator_to_target_connection_type ==
      kConnectionObjectConnectionTypeMulticast)                                         /* Multicast consuming */
  {
    if (OpenMulticastConnection(kUdpCommuncationDirectionConsuming,
                                connection_object, common_packet_format_data)
        == kEipStatusError) {
      OPENER_TRACE_ERR("error in OpenMulticast Connection\n");
      return kCipErrorConnectionFailure;
    }
  } else if (originator_to_target_connection_type ==
             kConnectionObjectConnectionTypePointToPoint)                                  /* Point to Point consuming */
  {
    if (OpenConsumingPointToPointConnection(connection_object,
                                            common_packet_format_data)
        == kEipStatusError) {
      OPENER_TRACE_ERR("error in PointToPoint consuming connection\n");
      return kCipErrorConnectionFailure;
    }
  }

  if (target_to_originator_connection_type ==
      kConnectionObjectConnectionTypeMulticast)                                         /* Multicast producing */
  {
    if (OpenProducingMulticastConnection(connection_object,
                                         common_packet_format_data)
        == kEipStatusError) {
      OPENER_TRACE_ERR("error in OpenMulticast Connection\n");
      return kCipErrorConnectionFailure;
    }
  } else if (target_to_originator_connection_type ==
             kConnectionObjectConnectionTypePointToPoint)                                  /* Point to Point producing */
  {

    if (OpenProducingPointToPointConnection(connection_object,
                                            common_packet_format_data)
        != kEipStatusOk) {
      OPENER_TRACE_ERR("error in PointToPoint producing connection\n");
      return kCipErrorConnectionFailure;
    }
  }
  return eip_status;
}

void CloseCommunicationChannelsAndRemoveFromActiveConnectionsList(
  CipConnectionObject *connection_object) {
  CloseUdpSocket(
    connection_object->socket[kUdpCommuncationDirectionConsuming]);

  connection_object->socket[kUdpCommuncationDirectionConsuming] =
    kEipInvalidSocket;

  CloseUdpSocket(
    connection_object->socket[kUdpCommuncationDirectionProducing]);

  connection_object->socket[kUdpCommuncationDirectionProducing] =
    kEipInvalidSocket;

  RemoveFromActiveConnections(connection_object);
  ConnectionObjectInitializeEmpty(connection_object);
}
