/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPCONNECTIONMANAGER_H_
#define OPENER_CIPCONNECTIONMANAGER_H_

#include "opener_user_conf.h"
#include "opener_api.h"
#include "typedefs.h"
#include "ciptypes.h"

/**
 * @brief Connection Type constants of the Forward Open service request
 *   Indicates either a
 * - Null Request
 * - Point-to-point connection request (unicast)
 * - Multicast connection request
 */
typedef enum {
  kForwardOpenConnectionTypeNull = 0x0000,
  kForwardOpenConnectionTypePointToPointConnection = 0x4000,
  kForwardOpenConnectionTypeMulticastConnection = 0x2000,
  kForwardOpenConnectionTypeReserved = 0x6000 /**< Reserved and therefore invalid type*/
} ForwardOpenConnectionType;

typedef enum {
  kConnectionManagerGeneralStatusSuccess = 0x00, /**< General Status - Everything is ok */
  kConnectionManagerGeneralStatusExtendedStatus = 0x01, /**< Indicates that extended status is set */
  kConnectionManagerGeneralStatusResourceUnavailableForUnconnectedSend = 0x02,
  kConnectionManagerGeneralStatusPathSegmentErrorInUnconnectedSend = 0x04,
  kConnectionManagerGeneralStatusErrorInDataSegment = 0x09,
  kConnectionManagerGeneralStatusObjectStateError = 0x0C,
  kConnectionManagerGeneralStatusDeviceStateError = 0x10,
  kConnectionManagerGeneralStatusNotEnoughData = 0x13,
  kConnectionManagerGeneralStatusTooMuchData = 0x15,
} ConnectionManagerGeneralStatus;
/** @brief Connection Manager Error codes */
typedef enum {
  kConnectionManagerExtendedStatusCodeSuccess = 0x00, /**< Obsolete code, should be General Status - Everything is ok */
  kConnectionManagerExtendedStatusCodeErrorConnectionInUseOrDuplicateForwardOpen
    = 0x0100,                                                                              /**< General Status has to be 0x01, Connection is already in use, or a duplicate Forward Open was received */
  kConnectionManagerExtendedStatusCodeErrorTransportClassAndTriggerCombinationNotSupported
    = 0x0103,                                                                                        /**< General Status has to be 0x01, A Transport class and trigger combination has been specified, which is not supported by the target application */
  kConnectionManagerExtendedStatusCodeErrorOwnershipConflict = 0x0106, /**< General Status has to be 0x01, Another connection has already reserved some needed resources */
  kConnectionManagerExtendedStatusCodeErrorConnectionTargetConnectionNotFound =
    0x0107,                                                                             /**< General Status has to be 0x01, Forward Close error message, if connection to be closed is not found at the target */
  kConnectionManagerExtendedStatusCodeErrorTargetForConnectionNotConfigured =
    0x0110,                                                                           /**< General Status has to be 0x01, Target application not configured and connection request does not contain data segment for configuration */
  kConnectionManagerExtendedStatusCodeErrorRpiValuesNotAcceptable = 0x0112, /**< General Status has to be 0x01, Requested RPI parameters outside of range, needs 6 16-bit extended status words, see Vol.1 Table 3-5.33 */
  kConnectionManagerExtendedStatusCodeErrorNoMoreConnectionsAvailable = 0x0113, /**< General Status has to be 0x01, No free connection slots available */
  kConnectionManagerExtendedStatusCodeErrorVendorIdOrProductcodeError = 0x0114, /**< General Status has to be 0x01, The Product Code or Vendor ID in the electronic key logical segment does not match the Product Code or Vendor ID of the device, or if the compatibility bit is set and one or both are zero, or cannot be emulated. */
  kConnectionManagerExtendedStatusCodeErrorDeviceTypeError = 0x0115, /**< General Status has to be 0x01, Device Type specified in the electronic key logical segment does not match the Device Type, or if the compatibility bit is set and Device Type is zero, or cannot be emulated. */
  kConnectionManagerExtendedStatusCodeErrorRevisionMismatch = 0x0116, /**< General Status has to be 0x01, Major and minor revision specified in the electronic key logical segment is not a valid revision of the device, or if the compatibility bit is set and the requested Major Revision and/or Minor Revision is 0 or the device cannot emulate the specified revision. */
  kConnectionManagerExtendedStatusCodeNonListenOnlyConnectionNotOpened = 0x0119, /**< General Status has to be 0x01, listen-only connection cannot be established, if no non-listen only connections are established  */
  kConnectionManagerExtendedStatusCodeTargetObjectOutOfConnections = 0x011A, /**< Maximum number of connections supported by the instance of the target object exceeded */
  kConnectionManagerExtendedStatusCodeProductionInhibitTimerGreaterThanRpi =
    0x011B,                                                                          /**< The Production Inhibit Time is greater than the Target to Originator RPI */
  kConnectionManagerExtendedStatusCodeTransportClassNotSupported = 0x011C, /**< The transport class requested in the Transport Type/Trigger parameter is not supported. */
  kConnectionManagerExtendedStatusCodeProductionTriggerNotSuppoerted = 0x011D, /**< The production trigger requested in the Transport Type/Trigger parameter is not supported. */
  kConnectionManagerExtendedStatusCodeDirectionNotSupported = 0x011E, /**< The direction requested in the Transport Type/Trigger parameter is not supported */
  kConnectionManagerExtendedStatusCodeInvalidOToTNetworkConnectionFixVar =
    0x011F,                                                                        /**< Shall be returned as the result of specifying an O->T fixed / variable flag that is not supported. */
  kConnectionManagerExtendedStatusCodeInvalidTToONetworkConnectionFixVar =
    0x0120,                                                                        /**< Shall be returned as the result of specifying an T->O fixed / variable flag that is not supported. */
  kConnectionManagerExtendedStatusCodeInvalidOToTNetworkConnectionPriority =
    0x0121,                                                                          /**< Shall be returned as the result of specifying an O->T priority code that is not supported. */
  kConnectionManagerExtendedStatusCodeInvalidTToONetworkConnectionPriority =
    0x0122,                                                                          /**< Shall be returned as the result of specifying an T->O priority code that is not supported. */
  kConnectionManagerExtendedStatusCodeErrorInvalidOToTConnectionType = 0x0123, /**< Shall be returned as the result of specifying an O->T connection type that is not supported */
  kConnectionManagerExtendedStatusCodeErrorInvalidTToOConnectionType = 0x0124, /**< Shall be returned as the result of specifying a T->O connection type that is not supported */
  kConnectionManagerExtendedStatusCodeInvalidOToTNetworkConnectionRedundantOwner
    = 0x0125,                                                                              /**< Shall be returned as the result of specifying an O->T Redundant Owner flag that is not supported. */
  kConnectionManagerExtendedStatusCodeInvalidConfigurationSize = 0x0126, /**< The data segment provided in the Connection_Path parameter did not contain an acceptable number of 16-bit words for the the configuration application path requested. Two additional status words shall follow, the error code plus the max size in words */
  kConnectionManagerExtendedStatusCodeErrorInvalidOToTConnectionSize = 0x0127, /**< The size of the consuming object declared in the Forward_Open request and available on the target does not match the size declared in the O->T Network Connection Parameter. Two additional status words shall follow, the error code plus the max size in words */
  kConnectionManagerExtendedStatusCodeErrorInvalidTToOConnectionSize = 0x0128, /**< The size of the consuming object declared in the Forward_Open request and available on the target does not match the size declared in the T->O Network Connection Parameter. Two additional status words shall follow, the error code plus the max size in words  */
  kConnectionManagerExtendedStatusCodeInvalidConfigurationApplicationPath =
    0x0129,                                                                         /**< Configuration application path specified does not correspond to a valid configuration application path within the target application. This error could also be returned if a configuration application path was required, but not provided by a connection request. */
  kConnectionManagerExtendedStatusCodeInvalidConsumingApplicationPath = 0x012A, /**< Consumed application path specified does not correspond to a valid consumed application path within the target application. This error could also be returned if a consumed application path was required, but not provided by a connection request. */
  kConnectionManagerExtendedStatusCodeInvalidProducingApplicationPath = 0x012B, /**< Produced application path specified does not correspond to a valid produced application path within the target application. This error could also be returned if a produced application path was required, but not provided by a connection request. */
  kConnectionManagerExtendedStatusCodeConfigurationSymbolDoesNotExist = 0x012C,
  kConnectionManagerExtendedStatusCodeConsumingSymbolDoesNotExist = 0x012D,
  kConnectionManagerExtendedStatusCodeProducingSymbolDoesNotExist = 0x012E,
  kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo = 0x012F, /**<  */
  kConnectionManagerExtendedStatusCodeInconsistentConsumeDataFormat = 0x0130,
  kConnectionManagerExtendedStatusCodeInconsistentProduceDataFormat = 0x0131,
  kConnectionManagerExtendedStatusCodeNullForwardOpenNotSupported = 0x0132,
  kConnectionManagerExtendedStatusCodeConnectionTimeoutMultiplierNotAcceptable =
    0x0133,
  kConnectionManagerExtendedStatusCodeConnectionTimedOut = 0x0203,
  kConnectionManagerExtendedStatusCodeUnconnectedRequestTimedOut = 0x0204,
  kConnectionManagerExtendedStatusCodeErrorParameterErrorInUnconnectedSendService
    = 0x0205,                                                                               /**<  */
  kConnectionManagerExtendedStatusCodeMessageToLargeForUnconnectedSendService =
    0x0206,
  kConnectionManagerExtendedStatusCodeUnconnectedAcknowledgeWithoutReply =
    0x0207,
  kConnectionManagerExtendedStatusCodeNoBufferMemoryAvailable = 0x0301,
  kConnectionManagerExtendedStatusCodeNetworkBandwithNotAvailableForData =
    0x0302,
  kConnectionManagerExtendedStatusCodeNoConsumedConnectionIdFilterAvailable =
    0x0303,
  kConnectionManagerExtendedStatusCodeNotConfiguredToSendScheduledPriorityData =
    0x0304,
  kConnectionManagerExtendedStatusCodeScheduleSignatureMismatch = 0x0305,
  kConnectionManagerExtendedStatusCodeScheduleSignatureValidationNotPossible =
    0x0306,
  kConnectionManagerExtendedStatusCodePortNotAvailable = 0x0311,
  kConnectionManagerExtendedStatusCodeLinkAddressNotValid = 0x0312,
  kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath = 0x0315, /**<  */
  kConnectionManagerExtendedStatusCodeForwardCloseServiceConnectionPathMismatch
    = 0x0316,
  kConnectionManagerExtendedStatusCodeSchedulingNotSpecified = 0x0317,
  kConnectionManagerExtendedStatusCodeLinkAddressToSelfInvalid = 0x0318,
  kConnectionManagerExtendedStatusCodeSecondaryResourcesUnavailable = 0x0319,
  kConnectionManagerExtendedStatusCodeRackConnectionAlreadyEstablished = 0x031A,
  kConnectionManagerExtendedStatusCodeModuleConnectionAlreadyEstablished =
    0x031B,
  kConnectionManagerExtendedStatusCodeMiscellaneous = 0x031C,
  kConnectionManagerExtendedStatusCodeRedundantConnectionMismatch = 0x031D,
  kConnectionManagerExtendedStatusCodeNoMoreUserConfigurableLinkConsumerResourcesAvailableInTheProducingModule
    = 0x031E,
  kConnectionManagerExtendedStatusCodeNoUserConfigurableLinkConsumerResourcesConfiguredInTheProducingModule
    = 0x031F,
  kConnectionManagerExtendedStatusCodeNetworkLinkOffline = 0x0800,
  kConnectionManagerExtendedStatusCodeNoTargetApplicationDataAvailable = 0x0810,
  kConnectionManagerExtendedStatusCodeNoOriginatorApplicationDataAvailable =
    0x0811,
  kConnectionManagerExtendedStatusCodeNodeAddressHasChangedSinceTheNetworkWasScheduled
    = 0x0812,
  kConnectionManagerExtendedStatusCodeNotConfiguredForOffSubnetMulticast =
    0x0813,
  kConnectionManagerExtendedStatusCodeInvalidProduceConsumeDataFormat = 0x0814,
  kConnectionManagerExtendedStatusWrongCloser
} ConnectionManagerExtendedStatusCode;

typedef enum {
  kConnectionTriggerTypeProductionTriggerMask = 0x70,
  kConnectionTriggerTypeCyclicConnection = 0x0,
  kConnectionTriggerTypeChangeOfStateTriggeredConnection = 0x10,
  kConnectionTriggerTypeApplicationTriggeredConnection = 0x20
} ConnectionTriggerType;

typedef enum ProductionTrigger {
  kProductionTriggerInvalid = -1,
  kProductionTriggerCyclic = 0,
  kProductionTriggerChangeOfState = 1,
  kProductionTriggerApplicationObjectTriggered = 2
} ProductionTrigger;

ProductionTrigger GetProductionTrigger(
  const ConnectionObject *const connection_object);

void SetProductionTrigger(const ProductionTrigger production_trigger,
                          ConnectionObject *connection_object);

CipUint GetProductionInhibitTime(const ConnectionObject *const connection_object);

void SetProductionInhibitTime(const EipUint16 production_inhibit_time,
                              ConnectionObject *const connection_object);

CipUdint GetTargetToOriginatorRequestedPackedInterval(
  const ConnectionObject *const connection_object);

/** @brief macros for comparing sequence numbers according to CIP spec vol
 * 2 3-4.2 for int type variables
 * @define SEQ_LEQ32(a, b) Checks if sequence number a is less or equal than b
 * @define SEQ_GEQ32(a, b) Checks if sequence number a is greater or equal than
 *  b
 *  @define SEQ_GT32(a, b) Checks if sequence number a is greater than b
 */
#define SEQ_LEQ32(a, b) ( (int)( (a) - (b) ) <= 0 )
#define SEQ_GEQ32(a, b) ( (int)( (a) - (b) ) >= 0 )
#define SEQ_GT32(a, b) ( (int)( (a) - (b) ) > 0 )

/** @brief similar macros for comparing 16 bit sequence numbers
 * @define SEQ_LEQ16(a, b) Checks if sequence number a is less or equal than b
 * @define SEQ_GEQ16(a, b) Checks if sequence number a is greater or equal than
 *  b
 */
#define SEQ_LEQ16(a, b) ( (short)( (a) - (b) ) <= 0 )
#define SEQ_GEQ16(a, b) ( (short)( (a) - (b) ) >= 0 )

/** @brief States of a connection */
typedef enum {
  kConnectionStateNonExistent = 0,
  kConnectionStateConfiguring = 1,
  kConnectionStateWaitingForConnectionId = 2 /**< only used in DeviceNet */,
  kConnectionStateEstablished = 3,
  kConnectionStateTimedOut = 4,
  kConnectionStateDeferredDelete = 5 /**< only used in DeviceNet */,
  kConnectionStateClosing
} ConnectionState;

/** @brief instance_type attributes */
typedef enum {
  kConnectionTypeExplicit = 0,
  kConnectionTypeIoExclusiveOwner = 0x01,
  kConnectionTypeIoInputOnly = 0x11,
  kConnectionTypeIoListenOnly = 0x21
} ConnectionType;

/** @brief Possible values for the watch dog time out action of a connection */
typedef enum {
  kWatchdogTimeoutActionTransitionToTimedOut = 0, /**< , invalid for explicit message connections */
  kWatchdogTimeoutActionAutoDelete = 1, /**< Default for explicit message connections,
                                           default for I/O connections on EIP */
  kWatchdogTimeoutActionAutoReset = 2, /**< Invalid for explicit message connections */
  kWatchdogTimeoutActionDeferredDelete = 3 /**< Only valid for DeviceNet, invalid for I/O connections */
} WatchdogTimeoutAction;

typedef struct {
  ConnectionState state;
  EipUint16 connection_id;
/*TODO think if this is needed anymore
   TCMReceiveDataFunc m_ptfuncReceiveData; */
} LinkConsumer;

typedef struct {
  ConnectionState state;
  EipUint16 connection_id;
} LinkProducer;

typedef struct {
  LinkConsumer consumer;
  LinkProducer producer;
} LinkObject;

/** The data needed for handling connections. This data is strongly related to
 * the connection object defined in the CIP-specification. However the full
 * functionality of the connection object is not implemented. Therefore this
 * data can not be accessed with CIP means.
 */
typedef struct connection_object {
  ConnectionState state; /**< state of the object */
  ConnectionType instance_type; /**< Indicates either I/O or Messaging connection */
  EipByte transport_type_class_trigger;
  /* conditional
     EipUint16 device_net_produced_connection_id;
     EipUint16 device_net_consumed_connection_id;
     EipByte device_net_initial_comm_characteristcs;
   */

  EipUint16 produced_connection_size;
  EipUint16 consumed_connection_size;
  EipUint16 expected_packet_rate;

  /*conditional*/
  EipUint32 cip_produced_connection_id;
  EipUint32 cip_consumed_connection_id;
  /**/
  EipUint8 watchdog_timeout_action; /**< see enum WatchdogTimeoutAction */
  EipUint16 produced_connection_path_length;
  CipEpath produced_connection_path; /**< Packed EPATH */
  EipUint16 consumed_connection_path_length;
  CipEpath consumed_connection_path; /**< Packed EPATH */
  /** @brief Minimal time between the production of two application triggered
   * or change of state triggered I/O connection messages
   */
  EipUint16 production_inhibit_time;
  EipUint16 connection_timeout_multiplier;
  /* Conditional
   * connection_binding_list < STRUCT OF UINT, Array of UINT
   */
  /* non CIP Attributes, only relevant for opened connections */
  EipByte priority_timetick;
  EipUint8 timeout_ticks;
  EipUint16 connection_serial_number;
  EipUint16 originator_vendor_id;
  EipUint32 originator_serial_number;

  EipUint32 o_to_t_requested_packet_interval;
  EipUint16 o_to_t_network_connection_parameter;
  EipUint32 t_to_o_requested_packet_interval;
  EipUint16 t_to_o_network_connection_parameter;

  EipUint8 connection_path_size;
  CipElectronicKey electronic_key;
  CipConnectionPath connection_path; /* padded EPATH*/
  LinkObject link_object;

  CipInstance *consuming_instance;
  /*S_CIP_CM_Object *p_stConsumingCMObject; */

  CipInstance *producing_instance;
  /*S_CIP_CM_Object *p_stProducingCMObject; */

  EipUint32 eip_level_sequence_count_producing; /* the EIP level sequence Count
                                                   for Class 0/1
                                                   Producing Connections may have a
                                                   different
                                                   value than SequenceCountProducing */
  EipUint32 eip_level_sequence_count_consuming; /* the EIP level sequence Count
                                                   for Class 0/1
                                                   Producing Connections may have a
                                                   different
                                                   value than SequenceCountProducing */

  EipUint16 sequence_count_producing; /* sequence Count for Class 1 Producing
                                         Connections */
  EipUint16 sequence_count_consuming; /* sequence Count for Class 1 Producing
                                         Connections */

  EipInt32 transmission_trigger_timer;
  uint64_t inactivity_watchdog_timer;



  /** @brief Timer for the production inhibition of application triggered or
   * change-of-state I/O connections.
   */
  EipInt32 production_inhibit_timer;

  struct sockaddr_in remote_address; /* socket address for produce */
  struct sockaddr_in originator_address; /* the address of the originator that
                                            established the connection. needed
                                            for scanning if the right packet is
                                            arriving */
  int socket[2]; /* socket handles, indexed by kConsuming or kProducing */

  /* pointers to connection handling functions */
  ConnectionCloseFunction connection_close_function;
  ConnectionTimeoutFunction connection_timeout_function;
  ConnectionSendDataFunction connection_send_data_function;
  ConnectionReceiveDataFunction connection_receive_data_function;

  /* pointers to be used in the active connection list */
  struct connection_object *next_connection_object;
  struct connection_object *first_connection_object;

  EipUint16 correct_originator_to_target_size;
  EipUint16 correct_target_to_originator_size;
  struct sockaddr_in original_opener_ip_address;
} ConnectionObject;

/** @brief Connection Manager class code */
static const int g_kCipConnectionManagerClassCode = 0x06;

/* public functions */

/** @brief Initialize the data of the connection manager object
 *
 *  @param A unique connection id
 *  @return kEipStatusOk if successful, otherwise kEipStatusError
 */
EipStatus ConnectionManagerInit(EipUint16 unique_connection_id);

/** @brief Get a connected object dependent on requested ConnectionID.
 *
 *   @param connection_id  requested @var connection_id of opened connection
 *   @return pointer to connected Object
 *           0 .. connection not present in device
 */
ConnectionObject *GetConnectedObject(EipUint32 connection_id);

/**  Get a connection object for a given output assembly.
 *
 *   @param output_assembly_id requested output assembly of requested
 *  connection
 *   @return pointer to connected Object
 *           0 .. connection not present in device
 */
ConnectionObject *GetConnectedOutputAssembly(EipUint32 output_assembly_id);

/** @brief Copy the given connection data from source to destination
 *
 * @param destination Destination of the copy operation
 * @param osurce Source of the copy operation
 */
void CopyConnectionData(ConnectionObject *RESTRICT destination,
                        const ConnectionObject *RESTRICT const source);

/** @brief Close the given connection
 *
 * This function will take the data form the connection and correctly closes the
 * connection (e.g., open sockets)
 * @param connection_object pointer to the connection object structure to be
 * closed
 */
void CloseConnection(ConnectionObject *RESTRICT connection_object);

/* TODO: Missing documentation */
EipBool8 IsConnectedOutputAssembly(const EipUint32 instance_number);

/** @brief Generate the ConnectionIDs and set the general configuration
 * parameter in the given connection object.
 *
 * @param connection_object pointer to the connection object that should be set
 * up.
 */
void GeneralConnectionConfiguration(ConnectionObject *connection_object);

/** @brief Insert the given connection object to the list of currently active
 *  and managed connections.
 *
 * By adding a connection to the active connection list the connection manager
 * will perform the supervision and handle the timing (e.g., timeout,
 * production inhibit, etc).
 *
 * @param connection_object pointer to the connection object to be added.
 */
void AddNewActiveConnection(ConnectionObject *connection_object);

/** @brief Removes connection from the list of active connections
 *
 * @param connection_object Connection object to be removed from the active connection list
 */
void RemoveFromActiveConnections(ConnectionObject *connection_object);

/** @brief returns the connection type of the supplied network connection parameter
 *
 */
ForwardOpenConnectionType GetConnectionType(
  EipUint16 network_connection_parameter);

#endif /* OPENER_CIPCONNECTIONMANAGER_H_ */
