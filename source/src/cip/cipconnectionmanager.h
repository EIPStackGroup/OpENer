/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef SRC_CIP_CIPCONNECTIONMANAGER_H_
#define SRC_CIP_CIPCONNECTIONMANAGER_H_

#include "api/opener_api.h"
#include "cip/cipconnectionobject.h"
#include "cip/ciptypes.h"
#include "core/typedefs.h"
#include "opener_user_conf.h"  // NOLINT(build/include_subdir)  // NOLINT(build/include_subdir)

/**
 * @brief Connection Type constants of the Forward Open service request
 *   Indicates either a
 * - Null Request
 * - Point-to-point connection request (unicast)
 * - Multicast connection request
 */

typedef enum {
  kConnectionManagerGeneralStatusSuccess =
    0x00U, /**< General Status - Everything is ok */
  kConnectionManagerGeneralStatusExtendedStatus =
    0x01U, /**< Indicates that extended status is set */
  kConnectionManagerGeneralStatusResourceUnavailableForUnconnectedSend = 0x02U,
  kConnectionManagerGeneralStatusPathSegmentErrorInUnconnectedSend     = 0x04U,
  kConnectionManagerGeneralStatusErrorInDataSegment                    = 0x09U,
  kConnectionManagerGeneralStatusObjectStateError                      = 0x0CU,
  kConnectionManagerGeneralStatusDeviceStateError                      = 0x10U,
  kConnectionManagerGeneralStatusNotEnoughData                         = 0x13U,
  kConnectionManagerGeneralStatusTooMuchData                           = 0x15U,
} ConnectionManagerGeneralStatus;

/** @brief Connection Manager Error codes */
typedef enum {
  kConnectionManagerExtendedStatusCodeSuccess =
    0x00U, /**< Obsolete code, should be General Status - Everything is ok */
  kConnectionManagerExtendedStatusCodeErrorConnectionInUseOrDuplicateForwardOpen =
    0x0100U,  ///< General Status has to be 0x01, Connection is already in
              ///< use, or a duplicate Forward Open was received
  kConnectionManagerExtendedStatusCodeErrorTransportClassAndTriggerCombinationNotSupported =
    0x0103U,  ///< General Status has to be 0x01, A Transport class and
              ///< trigger combination has been specified, which is not
              ///< supported by the target application
  kConnectionManagerExtendedStatusCodeErrorOwnershipConflict =
    0x0106U,  ///< General Status has to be 0x01, Another connection has
              ///< already reserved some needed resources */
  kConnectionManagerExtendedStatusCodeErrorConnectionTargetConnectionNotFound =
    0x0107U,  ///< General Status has to be 0x01, Forward Close error message,
              ///< if connection to be closed is not found at the target */
  kConnectionManagerExtendedStatusCodeErrorTargetForConnectionNotConfigured =
    0x0110U,  ///< General Status has to be 0x01, Target application not
              ///< configured and connection request does not contain data
              ///< segment for configuration
  kConnectionManagerExtendedStatusCodeRpiNotSupported =
    0x0111U,  ///< General Status has
              ///< to be 0x01, Requested RPI not supported by target device
  kConnectionManagerExtendedStatusCodeErrorRpiValuesNotAcceptable =
    0x0112U,  ///< General Status has to be 0x01, Requested RPI parameters
              ///< outside of range, needs 6 16-bit extended status words, see
              ///< Vol.1 Table 3-5.33
  kConnectionManagerExtendedStatusCodeErrorNoMoreConnectionsAvailable =
    0x0113U,  ///< General Status has to be 0x01, No free connection slots
              ///< available
  kConnectionManagerExtendedStatusCodeErrorVendorIdOrProductcodeError =
    0x0114U,  ///< General Status has to be 0x01, The Product Code or Vendor
              ///< ID in the electronic key logical segment does not match the
              ///< Product Code or Vendor ID of the device, or if the
              ///< compatibility bit is set and one or both are zero, or
              ///< cannot be emulated.
  kConnectionManagerExtendedStatusCodeErrorDeviceTypeError =
    0x0115U,  ///< General Status has to be 0x01, Device Type specified in the
              ///< electronic key logical segment does not match the Device
              ///< Type, or if the compatibility bit is set and Device Type is
              ///< zero, or cannot be emulated.
  kConnectionManagerExtendedStatusCodeErrorRevisionMismatch =
    0x0116U,  ///< General Status has to be 0x01, Major and minor revision
              ///< specified in the electronic key logical segment is not a
              ///< valid revision of the device, or if the compatibility bit
              ///< is set and the requested Major Revision and/or Minor
              ///< Revision is 0 or the device cannot emulate the specified
              ///< revision.
  kConnectionManagerExtendedStatusCodeNonListenOnlyConnectionNotOpened =
    0x0119U,  ///< General Status has to be 0x01, listen-only connection
              ///< cannot be established, if no non-listen only connections
              ///< are established
  kConnectionManagerExtendedStatusCodeTargetObjectOutOfConnections =
    0x011AU,  ///< Maximum number of connections supported by the instance of
              ///< the target object exceeded
  kConnectionManagerExtendedStatusCodeProductionInhibitTimerGreaterThanRpi =
    0x011BU,  ///< The Production Inhibit Time is greater than the Target to
              ///< Originator RPI
  kConnectionManagerExtendedStatusCodeTransportClassNotSupported =
    0x011CU,  ///< The transport class requested in the Transport Type/Trigger
              ///< parameter is not supported.
  kConnectionManagerExtendedStatusCodeProductionTriggerNotSuppoerted =
    0x011DU,  ///< The production trigger requested in the Transport
              ///< Type/Trigger parameter is not supported.
  kConnectionManagerExtendedStatusCodeDirectionNotSupported =
    0x011EU,  ///< The direction requested in the Transport Type/Trigger
              ///< parameter is not supported
  kConnectionManagerExtendedStatusCodeInvalidOToTNetworkConnectionFixVar =
    0x011FU,  ///< Shall be returned as the result of specifying an O->T fixed
              ///< / variable flag that is not supported.
  kConnectionManagerExtendedStatusCodeInvalidTToONetworkConnectionFixVar =
    0x0120U,  ///< Shall be returned as the result of specifying an T->O fixed
              ///< / variable flag that is not supported.
  kConnectionManagerExtendedStatusCodeInvalidOToTNetworkConnectionPriority =
    0x0121U,  ///< Shall be returned as the result of specifying an O->T
              ///< priority code that is not supported.
  kConnectionManagerExtendedStatusCodeInvalidTToONetworkConnectionPriority =
    0x0122U,  ///< Shall be returned as the result of specifying an T->O
              ///< priority code that is not supported.
  kConnectionManagerExtendedStatusCodeErrorInvalidOToTConnectionType =
    0x0123U,  ///< Shall be returned as the result of specifying an O->T
              ///< connection type that is not supported
  kConnectionManagerExtendedStatusCodeErrorInvalidTToOConnectionType =
    0x0124U,  ///< Shall be returned as the result of specifying a T->O
              ///< connection type that is not supported
  kConnectionManagerExtendedStatusCodeInvalidOToTNetworkConnectionRedundantOwner =
    0x0125U,  ///< Shall be returned as the result of specifying an O->T
              ///< Redundant Owner flag that is not supported.
  kConnectionManagerExtendedStatusCodeInvalidConfigurationSize =
    0x0126U,  ///< The data segment provided in the Connection_Path parameter
              ///< did not contain an acceptable number of 16-bit words for
              ///< the the configuration application path requested. Two
              ///< additional status words shall follow, the error code plus
              ///< the max size in words
  kConnectionManagerExtendedStatusCodeErrorInvalidOToTConnectionSize =
    0x0127U,  ///< The size of the consuming object declared in the
              ///< Forward_Open request and available on the target does not
              ///< match the size declared in the O->T Network Connection
              ///< Parameter. Two additional status words shall follow, the
              ///< error code plus the
              ///<  max size in words
  kConnectionManagerExtendedStatusCodeErrorInvalidTToOConnectionSize =
    0x0128U,  ///< The size of the consuming object declared in the
              ///< Forward_Open request and available on the target does not
              ///< match the size declared in the T->O Network Connection
              ///< Parameter. Two additional status words shall follow, the
              ///< error code plus the max size in words
  kConnectionManagerExtendedStatusCodeInvalidConfigurationApplicationPath =
    0x0129U,  ///< Configuration application path specified does not
              ///< correspond
              ///<  to a valid configuration application path within the
              ///<  target
              ///< configuration application path was required, but not
              ///< provided
              ///<  application. This error could also be returned if a
              ///<  by a connection request.
  kConnectionManagerExtendedStatusCodeInvalidConsumingApplicationPath =
    0x012AU,  ///< Consumed application path specified does not correspond to
              ///< a valid consumed application path within the target
              ///< application. This error could also be returned if a
              ///< consumed application path was required, but not provided by
              ///< a connection request.
  kConnectionManagerExtendedStatusCodeInvalidProducingApplicationPath =
    0x012BU,  ///< Produced application path specified does not correspond to
              ///< a valid produced application path within the target
              ///< application. This error could also be returned if a
              ///< produced application path was required, but not provided by
              ///< a connection request.
  kConnectionManagerExtendedStatusCodeConfigurationSymbolDoesNotExist = 0x012CU,
  kConnectionManagerExtendedStatusCodeConsumingSymbolDoesNotExist     = 0x012DU,
  kConnectionManagerExtendedStatusCodeProducingSymbolDoesNotExist     = 0x012EU,
  kConnectionManagerExtendedStatusCodeInconsistentApplicationPathCombo =
    0x012FU,
  kConnectionManagerExtendedStatusCodeInconsistentConsumeDataFormat = 0x0130U,
  kConnectionManagerExtendedStatusCodeInconsistentProduceDataFormat = 0x0131U,
  kConnectionManagerExtendedStatusCodeNullForwardOpenNotSupported   = 0x0132U,
  kConnectionManagerExtendedStatusCodeConnectionTimeoutMultiplierNotAcceptable =
    0x0133U,
  kConnectionManagerExtendedStatusCodeMismatchedTToONetworkConnectionFixVar =
    0x0135U,
  kConnectionManagerExtendedStatusCodeMismatchedTToONetworkConnectionPriority =
    0x0136U,
  kConnectionManagerExtendedStatusCodeMismatchedTransportClass        = 0x0137U,
  kConnectionManagerExtendedStatusCodeMismatchedTToOProductionTrigger = 0x0138U,
  kConnectionManagerExtendedStatusCodeMismatchedTToOProductionInhibitTimeSegment =
    0x0139U,
  kConnectionManagerExtendedStatusCodeConnectionTimedOut         = 0x0203U,
  kConnectionManagerExtendedStatusCodeUnconnectedRequestTimedOut = 0x0204U,
  kConnectionManagerExtendedStatusCodeErrorParameterErrorInUnconnectedSendService =
    0x0205U,
  kConnectionManagerExtendedStatusCodeMessageToLargeForUnconnectedSendService =
    0x0206U,
  kConnectionManagerExtendedStatusCodeUnconnectedAcknowledgeWithoutReply =
    0x0207U,
  kConnectionManagerExtendedStatusCodeNoBufferMemoryAvailable = 0x0301U,
  kConnectionManagerExtendedStatusCodeNetworkBandwithNotAvailableForData =
    0x0302U,
  kConnectionManagerExtendedStatusCodeNoConsumedConnectionIdFilterAvailable =
    0x0303U,
  kConnectionManagerExtendedStatusCodeNotConfiguredToSendScheduledPriorityData =
    0x0304U,
  kConnectionManagerExtendedStatusCodeScheduleSignatureMismatch = 0x0305U,
  kConnectionManagerExtendedStatusCodeScheduleSignatureValidationNotPossible =
    0x0306U,
  kConnectionManagerExtendedStatusCodePortNotAvailable              = 0x0311U,
  kConnectionManagerExtendedStatusCodeLinkAddressNotValid           = 0x0312U,
  kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath = 0x0315U,
  kConnectionManagerExtendedStatusCodeForwardCloseServiceConnectionPathMismatch =
    0x0316U,
  kConnectionManagerExtendedStatusCodeSchedulingNotSpecified        = 0x0317U,
  kConnectionManagerExtendedStatusCodeLinkAddressToSelfInvalid      = 0x0318U,
  kConnectionManagerExtendedStatusCodeSecondaryResourcesUnavailable = 0x0319U,
  kConnectionManagerExtendedStatusCodeRackConnectionAlreadyEstablished =
    0x031AU,
  kConnectionManagerExtendedStatusCodeModuleConnectionAlreadyEstablished =
    0x031BU,
  kConnectionManagerExtendedStatusCodeMiscellaneous               = 0x031CU,
  kConnectionManagerExtendedStatusCodeRedundantConnectionMismatch = 0x031DU,
  kConnectionManagerExtendedStatusCodeNoMoreUserConfigurableLinkConsumerResourcesAvailableInTheProducingModule =
    0x031EU,
  kConnectionManagerExtendedStatusCodeNoUserConfigurableLinkConsumerResourcesConfiguredInTheProducingModule =
    0x031FU,
  kConnectionManagerExtendedStatusCodeNetworkLinkOffline = 0x0800U,
  kConnectionManagerExtendedStatusCodeNoTargetApplicationDataAvailable =
    0x0810U,
  kConnectionManagerExtendedStatusCodeNoOriginatorApplicationDataAvailable =
    0x0811U,
  kConnectionManagerExtendedStatusCodeNodeAddressHasChangedSinceTheNetworkWasScheduled =
    0x0812U,
  kConnectionManagerExtendedStatusCodeNotConfiguredForOffSubnetMulticast =
    0x0813U,
  kConnectionManagerExtendedStatusCodeInvalidProduceConsumeDataFormat = 0x0814U,
  kConnectionManagerExtendedStatusWrongCloser =
    0xFFFFU  ///< Not an official extended error code - used for Forward Close
             ///< indicating that not the original originator is trying to
             ///< close the connection
} ConnectionManagerExtendedStatusCode;

/** @brief macros for comparing sequence numbers according to CIP spec vol
 * 2 3-4.2 for int type variables
 * @def SEQ_LEQ32(a, b) Checks if sequence number a is less or equal than b
 * @def SEQ_GEQ32(a, b) Checks if sequence number a is greater or equal than
 *  b
 *  @def SEQ_GT32(a, b) Checks if sequence number a is greater than b
 */
#define SEQ_LEQ32(a, b) ((int_fast32_t)((a) - (b)) <= 0)
#define SEQ_GEQ32(a, b) ((int_fast32_t)((a) - (b)) >= 0)
#define SEQ_GT32(a, b) ((int_fast32_t)((a) - (b)) > 0)

/** @brief similar macros for comparing 16 bit sequence numbers
 * @def SEQ_LEQ16(a, b) Checks if sequence number a is less or equal than b
 * @def SEQ_GEQ16(a, b) Checks if sequence number a is greater or equal than
 *  b
 */
#define SEQ_LEQ16(a, b) ((int_fast16_t)((a) - (b)) <= 0)
#define SEQ_GEQ16(a, b) ((int_fast16_t)((a) - (b)) >= 0)

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
CipConnectionObject* GetConnectedObject(const EipUint32 connection_id);

/**  Get a connection object for a given output assembly.
 *
 *   @param output_assembly_id requested output assembly of requested
 *  connection
 *   @return pointer to connected Object
 *           0 .. connection not present in device
 */
CipConnectionObject* GetConnectedOutputAssembly(
  const EipUint32 output_assembly_id);

/** @brief Close the given connection
 *
 * This function will take the data form the connection and correctly closes the
 * connection (e.g., open sockets)
 * @param connection_object pointer to the connection object structure to be
 * closed
 */
void CloseConnection(CipConnectionObject* RESTRICT connection_object);

/* TODO(MelikMerkumians): Missing documentation */
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
void AddNewActiveConnection(CipConnectionObject* const connection_object);

/** @brief Removes connection from the list of active connections
 *
 * @param connection_object Connection object to be removed from the active
 * connection list
 */
void RemoveFromActiveConnections(CipConnectionObject* const connection_object);

CipUdint GetConnectionId(void);

typedef void (*CloseSessionFunction)(
  const CipConnectionObject* const connection_object);

void CheckForTimedOutConnectionsAndCloseTCPConnections(
  const CipConnectionObject* const connection_object,
  CloseSessionFunction CloseSessions);

#endif  // SRC_CIP_CIPCONNECTIONMANAGER_H_
