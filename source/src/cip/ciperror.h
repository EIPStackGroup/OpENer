/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef SRC_CIP_CIPERROR_H_
#define SRC_CIP_CIPERROR_H_

typedef enum {
  /// Service was successfully performed by the object specified.
  kCipErrorSuccess = 0x00U,
  /// A connection related service failed along the connection path.
  kCipErrorConnectionFailure = 0x01U,
  /// Resources needed for the object to perform the requested service were
  /// unavailable
  kCipErrorResourceUnavailable = 0x02U,
  /// See Status Code 0x20, which is the preferred value to use for this
  /// condition.
  kCipErrorInvalidParameterValue = 0x03U,
  /// The path segment identifier or the segment syntax was not understood by
  /// the processing node. Path processing shall stop when a path segment error
  /// is encountered.
  kCipErrorPathSegmentError = 0x04U,
  /// The path is referencing an object class, instance or structure element
  /// that is not known or is not contained in the processing node. Path
  /// processing shall stop when a path destination unknown error is
  /// encountered.
  kCipErrorPathDestinationUnknown = 0x05U,
  /// Only part of the expected data was transferred.
  kCipErrorPartialTransfer = 0x06U,
  /// The messaging connection was lost.
  kCipErrorConnectionLost = 0x07U,
  /// The requested service was not implemented or was not defined for this
  /// Object Class/Instance.
  kCipErrorServiceNotSupported = 0x08U,
  /// Invalid attribute data detected
  kCipErrorInvalidAttributeValue = 0x09U,
  /// An attribute in the Get_Attribute_List or Set_Attribute_List response has
  /// a non-zero status.
  kCipErrorAttributeListError = 0x0AU,
  /// The object is already in the mode/state being requested by the service
  kCipErrorAlreadyInRequestedMode = 0x0BU,
  /// The object cannot perform the requested service in its current mode/state
  kCipErrorObjectStateConflict = 0x0CU,
  /// The requested instance of object to be created already exists.
  kCipErrorObjectAlreadyExists = 0x0DU,
  /// A request to modify a non-modifiable attribute was received.
  kCipErrorAttributeNotSetable = 0x0EU,
  /// A permission/privilege check failed
  kCipErrorPrivilegeViolation = 0x0FU,
  /// The device's current mode/state prohibits the execution of the requested
  /// service.
  kCipErrorDeviceStateConflict = 0x10U,
  /// The data to be transmitted in the response buffer is larger than the
  /// allocated response buffer
  kCipErrorReplyDataTooLarge = 0x11U,
  /// The service specified an operation that is going to fragment a primitive
  /// data value, i.e. half a REAL data type.
  kCipErrorFragmentationOfAPrimitiveValue = 0x12U,
  /// The service did not supply enough data to perform the specified operation.
  kCipErrorNotEnoughData = 0x13U,
  /// The attribute specified in the request is not supported
  kCipErrorAttributeNotSupported = 0x14U,
  /// The service supplied more data than was expected
  kCipErrorTooMuchData = 0x15U,
  /// The object specified does not exist in the device.
  kCipErrorObjectDoesNotExist = 0x16U,
  /// The fragmentation sequence for this service is not currently active for
  /// this data.
  kCipErrorServiceFragmentationSequenceNotInProgress = 0x17U,
  /// The attribute data of this object was not saved prior to the requested
  /// service.
  kCipErrorNoStoredAttributeData = 0x18U,
  /// The attribute data of this object was not saved due to a failure during
  /// the attempt.
  kCipErrorStoreOperationFailure = 0x19U,
  /// The service request packet was too large for transmission on a network in
  /// the path to the destination. The routing device was forced to abort the
  /// service.
  kCipErrorRoutingFailureRequestPacketTooLarge = 0x1AU,
  /// The service response packet was too large for transmission on a network in
  /// the path from the destination. The routing device was forced to abort the
  /// service.
  kCipErrorRoutingFailureResponsePacketTooLarge = 0x1BU,
  /// The service did not supply an attribute in a list of attributes that was
  /// needed by the service to perform the requested behavior.
  kCipErrorMissingAttributeListEntry = 0x1CU,
  /// The service is returning the list of attributes supplied with status
  /// information for those attributes that were invalid.
  kCipErrorInvalidAttributeValueList = 0x1DU,
  /// An embedded service resulted in an error.
  kCipErrorEmbeddedServiceError = 0x1EU,
  /// A vendor specific error has been encountered. The Additional Code Field of
  /// the Error Response defines the particular error encountered. Use of this
  /// General Error Code should only be performed when none of the Error Codes
  /// presented in this table or within an Object Class definition accurately
  /// reflect the error.
  kCipErrorVendorSpecificError = 0x1FU,
  /// A parameter associated with the request was invalid. This code is used
  /// when a parameter does not meet the requirements of this specification
  /// and/or the requirements defined in an Application Object Specification.
  kCipErrorInvalidParameter = 0x20U,
  /// An attempt was made to write to a write-once medium (e.g. WORM drive,
  /// PROM) that has already been written, or to modify a value that cannot be
  /// changed once established.
  kCipErrorWriteonceValueOrMediumAlreadyWritten = 0x21U,
  /// An invalid reply is received (e.g. reply service code does not match the
  /// request service code, or reply message is shorter than the minimum
  /// expected reply size). This status code can serve for other causes of
  /// invalid replies.
  kCipErrorInvalidReplyReceived = 0x22U,
  /* 23-24 Reserved by CIP for future extensions */
  /// The Key Segment that was included as the first segment in the path does
  /// not match the destination module. The object specific status shall
  /// indicate which part of the key check failed.
  kCipErrorKeyFailureInPath = 0x25U,
  /// The size of the path which was sent with the Service Request is either not
  /// large enough to allow the Request to be routed to an object or too much
  /// routing data was included.
  kCipErrorPathSizeInvalid = 0x26U,
  /// An attempt was made to set an attribute that is not able to be set at this
  /// time.
  kCipErrorUnexpectedAttributeInList = 0x27U,
  /// The Member ID specified in the request does not exist in the specified
  /// Class/Instance/Attribute
  kCipErrorInvalidMemberId = 0x28U,
  /// A request to modify a non-modifiable member was received
  kCipErrorMemberNotSetable = 0x29U,
  /// This error code may only be reported by DeviceNet group 2 only servers
  /// with 4K or less code space and only in place of Service not supported,
  /// Attribute not supported and Attribute not setable.
  kCipErrorGroup2OnlyServerGeneralFailure = 0x2AU,
  /// A CIP to Modbus translator received an unknown Modbus Exception Code.
  kCipErrorUnknownModbusError = 0x2BU,
  /// A request to read a non-readable attribute was received.
  kCipErrorAttributeNotGettable = 0x2CU,
  /// The requested object instance cannot be deleted.
  kCipErrorInstanceNotDeletable = 0x2DU,
  /// The object supports the service, but not for the designated application
  /// path (e.g. attribute). NOTE: Not to be used when a more specific General
  /// Status Code applies, e.g. 0x0E (Attribute not settable) or 0x29 (Member
  /// not settable).
  kCipErrorServiceNotSupportedForSpecifiedPath = 0x2EU,
  /* 2F - CF Reserved by CIP for future extensions D0 - FF Reserved for Object
     Class and service errors*/
} CipError;
#endif  // SRC_CIP_CIPERROR_H_
