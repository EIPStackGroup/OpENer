/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPERROR_H_
#define OPENER_CIPERROR_H_

typedef enum {
  kCipErrorSuccess = 0x00, /**< Service was successfully performed by the object specified. */
  kCipErrorConnectionFailure = 0x01, /**< A connection related service failed along the connection path. */
  kCipErrorResourceUnavailable = 0x02, /**< Resources needed for the object to perform the requested service were unavailable */
  kCipErrorInvalidParameterValue = 0x03, /**< See Status Code 0x20, which is the preferred value to use for this condition. */
  kCipErrorPathSegmentError = 0x04, /**< The path segment identifier or the segment syntax was not understood by the processing node. Path processing shall stop when a path segment error is encountered. */
  kCipErrorPathDestinationUnknown = 0x05, /**< The path is referencing an object class, instance or structure element that is not known or is not contained in the processing node. Path processing shall stop when a path destination unknown error is encountered. */
  kCipErrorPartialTransfer = 0x06, /**< Only part of the expected data was transferred. */
  kCipErrorConnectionLost = 0x07, /**< The messaging connection was lost. */
  kCipErrorServiceNotSupported = 0x08, /**< The requested service was not implemented or was not defined for this Object Class/Instance. */
  kCipErrorInvalidAttributeValue = 0x09, /**< Invalid attribute data detected */
  kCipErrorAttributeListError = 0x0A, /**< An attribute in the Get_Attribute_List or Set_Attribute_List response has a non-zero status. */
  kCipErrorAlreadyInRequestedMode = 0x0B, /**< The object is already in the mode/state being requested by the service */
  kCipErrorObjectStateConflict = 0x0C, /**< The object cannot perform the requested service in its current mode/state */
  kCipErrorObjectAlreadyExists = 0x0D, /**< The requested instance of object to be created already exists.*/
  kCipErrorAttributeNotSetable = 0x0E, /**< A request to modify a non-modifiable attribute was received. */
  kCipErrorPrivilegeViolation = 0x0F, /**< A permission/privilege check failed */
  kCipErrorDeviceStateConflict = 0x10, /**< The device's current mode/state prohibits the execution of the requested service. */
  kCipErrorReplyDataTooLarge = 0x11, /**< The data to be transmitted in the response buffer is larger than the allocated response buffer */
  kCipErrorFragmentationOfAPrimitiveValue = 0x12, /**< The service specified an operation that is going to fragment a primitive data value, i.e. half a REAL data type. */
  kCipErrorNotEnoughData = 0x13, /**< The service did not supply enough data to perform the specified operation. */
  kCipErrorAttributeNotSupported = 0x14, /**< The attribute specified in the request is not supported */
  kCipErrorTooMuchData = 0x15, /**< The service supplied more data than was expected */
  kCipErrorObjectDoesNotExist = 0x16, /**< The object specified does not exist in the device. */
  kCipErrorServiceFragmentationSequenceNotInProgress = 0x17, /**< The fragmentation sequence for this service is not currently active for this data. */
  kCipErrorNoStoredAttributeData = 0x18, /**< The attribute data of this object was not saved prior to the requested service. */
  kCipErrorStoreOperationFailure = 0x19, /**< The attribute data of this object was not saved due to a failure during the attempt. */
  kCipErrorRoutingFailureRequestPacketTooLarge = 0x1A, /**< The service request packet was too large for transmission on a network in the path to the destination. The routing device was forced to abort the service. */
  kCipErrorRoutingFailureResponsePacketTooLarge = 0x1B, /**< The service response packet was too large for transmission on a network in the path from the destination. The routing device was forced to abort the service. */
  kCipErrorMissingAttributeListEntry = 0x1C, /**< The service did not supply an attribute in a list of attributes that was needed by the service to perform the requested behavior. */
  kCipErrorInvalidAttributeValueList = 0x1D, /**< The service is returning the list of attributes supplied with status information for those attributes that were invalid. */
  kCipErrorEmbeddedServiceError = 0x1E, /**< An embedded service resulted in an error. */
  kCipErrorVendorSpecificError = 0x1F, /**< A vendor specific error has been encountered. The Additional Code Field of the Error Response defines the particular error encountered. Use of this General Error Code should only be performed when none of the Error Codes presented in this table or within an Object Class definition accurately reflect the error. */
  kCipErrorInvalidParameter = 0x20, /**< A parameter associated with the request was invalid. This code is used when a parameter does not meet the requirements of this specification and/or the requirements defined in an Application Object Specification. */
  kCipErrorWriteonceValueOrMediumAlreadyWritten = 0x21, /**< An attempt was made to write to a write-once medium (e.g. WORM drive, PROM) that has already been written, or to modify a value that cannot be changed once established. */
  kCipErrorInvalidReplyReceived = 0x22, /**< An invalid reply is received (e.g. reply service code does not match the request service code, or reply message is shorter than the minimum expected reply size). This status code can serve for other causes of invalid replies. */
  /* 23-24 Reserved by CIP for future extensions */
  kCipErrorKeyFailureInPath = 0x25, /**< The Key Segment that was included as the first segment in the path does not match the destination module. The object specific status shall indicate which part of the key check failed. */
  kCipErrorPathSizeInvalid = 0x26, /**< The size of the path which was sent with the Service Request is either not large enough to allow the Request to be routed to an object or too much routing data was included. */
  kCipErrorUnexpectedAttributeInList = 0x27, /**< An attempt was made to set an attribute that is not able to be set at this time. */
  kCipErrorInvalidMemberId = 0x28, /**< The Member ID specified in the request does not exist in the specified Class/Instance/Attribute */
  kCipErrorMemberNotSetable = 0x29, /**< A request to modify a non-modifiable member was received */
  kCipErrorGroup2OnlyServerGeneralFailure = 0x2A, /**< This error code may only be reported by DeviceNet group 2 only servers with 4K or less code space and only in place of Service not supported, Attribute not supported and Attribute not setable. */
  kCipErrorUnknownModbusError = 0x2B, /**< A CIP to Modbus translator received an unknown Modbus Exception Code. */
  kCipErrorAttributeNotGettable = 0x2C, /**< A request to read a non-readable attribute was received. */
  kCipErrorInstanceNotDeletable = 0x2D, /**< The requested object instance cannot be deleted. */
  kCipErrorServiceNotSupportedForSpecifiedPath = 0x2E,  /**< The object supports the service, but not for the designated application path (e.g. attribute). NOTE: Not to be used when a more specific General Status Code applies,
                                                              e.g. 0x0E (Attribute not settable) or 0x29 (Member not settable).*/
/* 2F - CF Reserved by CIP for future extensions D0 - FF Reserved for Object Class and service errors*/
} CipError;
#endif /* OPENER_CIPERROR_H_ */
