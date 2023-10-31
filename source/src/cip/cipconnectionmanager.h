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
#include "cipconnectionobject.h"

/**
 * @brief Connection Type constants of the Forward Open service request
 *   Indicates either a
 * - Null Request
 * - Point-to-point connection request (unicast)
 * - Multicast connection request
 */

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
  kConnectionManagerExtendedStatusCodeRpiNotSupported = 0x0111,
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
  kConnectionManagerExtendedStatusCodeConnectionTimeoutMultiplierNotAcceptable
    =
      0x0133,
  kConnectionManagerExtendedStatusCodeMismatchedTToONetworkConnectionFixVar =
    0x0135,
  kConnectionManagerExtendedStatusCodeMismatchedTToONetworkConnectionPriority =
    0x0136,
  kConnectionManagerExtendedStatusCodeMismatchedTransportClass = 0x0137,
  kConnectionManagerExtendedStatusCodeMismatchedTToOProductionTrigger = 0x0138,
  kConnectionManagerExtendedStatusCodeMismatchedTToOProductionInhibitTimeSegment
    = 0x0139,
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
  kConnectionManagerExtendedStatusCodeNotConfiguredToSendScheduledPriorityData
    = 0x0304,
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
  kConnectionManagerExtendedStatusWrongCloser = 0xFFFF /* No a real extended error code, but needed for forward close */
} ConnectionManagerExtendedStatusCode;

/** @brief macros for comparing sequence numbers according to CIP spec vol
 * 2 3-4.2 for int type variables
 * @def SEQ_LEQ32(a, b) Checks if sequence number a is less or equal than b
 * @def SEQ_GEQ32(a, b) Checks if sequence number a is greater or equal than
 *  b
 *  @def SEQ_GT32(a, b) Checks if sequence number a is greater than b
 */
#define SEQ_LEQ32(a, b) ( (int)( (a) - (b) ) <= 0 )
#define SEQ_GEQ32(a, b) ( (int)( (a) - (b) ) >= 0 )
#define SEQ_GT32(a, b) ( (int)( (a) - (b) ) > 0 )

/** @brief similar macros for comparing 16 bit sequence numbers
 * @def SEQ_LEQ16(a, b) Checks if sequence number a is less or equal than b
 * @def SEQ_GEQ16(a, b) Checks if sequence number a is greater or equal than
 *  b
 */
#define SEQ_LEQ16(a, b) ( (short)( (a) - (b) ) <= 0 )
#define SEQ_GEQ16(a, b) ( (short)( (a) - (b) ) >= 0 )

/** @brief Connection Manager class code */
static const CipUint kCipConnectionManagerClassCode = 0x06U;

/* public functions */

/** @brief Initialize the data of the connection manager object
 *
 *  @param unique_connection_id A unique connection id
 *  @return kEipStatusOk if successful, otherwise kEipStatusError
 */
EipStatus ConnectionManagerInit(EipUint16 unique_connection_id);

/** @brief Get a connected object dependent on requested ConnectionID.
 *
 *   @param connection_id Connection ID of the Connection Object to get
 *   @return pointer to connected Object
 *           NULL .. connection not present in device
 */
CipConnectionObject *GetConnectedObject(const EipUint32 connection_id);

/**  Get a connection object for a given output assembly.
 *
 *   @param output_assembly_id requested output assembly of requested
 *  connection
 *   @return pointer to connected Object
 *           0 .. connection not present in device
 */
CipConnectionObject *GetConnectedOutputAssembly(
  const EipUint32 output_assembly_id);

/** @brief Close the given connection
 *
 * This function will take the data form the connection and correctly closes the
 * connection (e.g., open sockets)
 * @param connection_object pointer to the connection object structure to be
 * closed
 */
void CloseConnection(CipConnectionObject *RESTRICT connection_object);

/* TODO: Missing documentation */
EipBool8 IsConnectedOutputAssembly(const CipInstanceNum instance_number);

/** @brief Insert the given connection object to the list of currently active
 *  and managed connections.
 *
 * By adding a connection to the active connection list the connection manager
 * will perform the supervision and handle the timing (e.g., timeout,
 * production inhibit, etc).
 *
 * @param connection_object pointer to the connection object to be added.
 */
void AddNewActiveConnection(CipConnectionObject *const connection_object);

/** @brief Removes connection from the list of active connections
 *
 * @param connection_object Connection object to be removed from the active connection list
 */
void RemoveFromActiveConnections(CipConnectionObject *const connection_object);


CipUdint GetConnectionId(void);

typedef void (*CloseSessionFunction)(const CipConnectionObject *const
                                     connection_object);

void CheckForTimedOutConnectionsAndCloseTCPConnections(
  const CipConnectionObject *const connection_object,
  CloseSessionFunction CloseSessions);

#endif /* OPENER_CIPCONNECTIONMANAGER_H_ */
