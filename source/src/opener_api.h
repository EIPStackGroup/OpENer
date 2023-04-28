/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_OPENER_API_H_
#define OPENER_OPENER_API_H_

#include <assert.h>
#include <stdbool.h>

#include "typedefs.h"
#include "ciptypes.h"
#include "ciperror.h"

#if defined(STM32)	/** STM32 target -> uses a struct for the network interface */
#define TcpIpInterface struct netif
#else		/** other targets -> string for the network interface */
#define TcpIpInterface const char
#endif		/** other targets */

/**  @defgroup CIP_API OpENer User interface
 * @brief This is the public interface of the OpENer. It provides all function
 * needed to implement an EtherNet/IP enabled slave-device.
 */

/** @ingroup CIP_API
 * @brief Read network configuration data from specified hardware interface
 *
 * @param  iface     address specifying the network interface
 * @param  iface_cfg address of interface configuration structure
 * @return           kEipStatusOk on success,
 *                   kEipStatusError on error with @p errno set
 *
 * This function reads all information needed to fill the iface_cfg structure
 *  of type @ref CipTcpIpInterfaceConfiguration from the hardware interface
 *  specified by the iface string.
 *
 */
EipStatus IfaceGetConfiguration(TcpIpInterface *iface,
                                CipTcpIpInterfaceConfiguration *iface_cfg);

/** @ingroup CIP_API
 * @brief Read and return the MAC address of the Ethernet interface
 *
 * @param  iface             address specifying the network interface
 * @param  physical_address  hardware MAC address of the network interface
 * @return                   kEipStatusOk: all fine
 *                           kEipStatusError: failure, errno set
 */
EipStatus IfaceGetMacAddress(TcpIpInterface *iface,
                             uint8_t *const physical_address);

/** @ingroup CIP_API
 * @brief Wait for the network interface having an IP address
 *
 * @param  iface      address specifying the network interface
 * @param  timeout    in seconds; max: INT_MAX/10, -1: wait for ever
 * @param  abort_wait stop waiting if this parameter becomes zero
 * @return            kEipStatusOk on success,
 *                    kEipStatusError on error with @p errno set
 *
 * This function waits for the network interface getting an IP address but
 *  only @p timeout seconds (set to -1 to wait for ever).
 * The polling wait process can be aborted by setting @p abort_wait to
 *  a non zero value from another thread.
 */
EipStatus IfaceWaitForIp(TcpIpInterface *const iface,
                         int timeout,
                         volatile int *const abort_wait);

#if defined(STM32)  /** STM32 target, the hostname is linked to the network interface */
/** @ingroup CIP_API
 * @brief Get host name from platform
 *
 * @param  iface      address specifying the network interface
 * @param  hostname   address of CipString destination structure
 *
 * This function reads the host name from the platform and returns it
 *  via the hostname parameter.
 */
void GetHostName(TcpIpInterface *iface,
                 CipString *hostname);
#else   /** other targets */
/** @ingroup CIP_API
 * @brief Get host name from platform
 *
 * @param  hostname  address of CipString destination structure
 *
 * This function reads the host name from the platform and returns it
 *  via the hostname parameter.
 */
void GetHostName(CipString *hostname);
#endif    /** other targets */

/** @ingroup CIP_API
 * @brief Set the CIP revision of the device's identity object.
 *
 * @param major unsigned 8 bit major revision
 * @param minor unsigned 8 bit minor revision
 */
void SetDeviceRevision(EipUint8 major,
                       EipUint8 minor);

/** @ingroup CIP_API
 * @brief Set the serial number of the device's identity object.
 *
 * @param serial_number unique 32 bit number identifying the device
 */
void SetDeviceSerialNumber(const EipUint32 serial_number);

/** @ingroup CIP_API
 * @brief Set the DeviceType of the device's identity object.
 *
 * @param type 16 bit unsigned number representing the CIP device type
 */
void SetDeviceType(const EipUint16 type);

/** @ingroup CIP_API
 * @brief Set the ProductCode of the device's identity object.
 *
 * @param type 16 bit unsigned number representing the product code
 */
void SetDeviceProductCode(const EipUint16 code);

/** @ingroup CIP_API
 * @brief Set the device's Status word also updating the Extended Device Status
 *
 * @param status    complete Identity Object's Status word content
 *
 *  This function sets the status flags and the internal state of the Extended
 *  Device Status field in Identity object's ext_status member.
 */
void SetDeviceStatus(const CipWord status);

/** @ingroup CIP_API
 * @breif Set device's CIP VendorId
 *
 * @param vendor_id vendor ID, can be zero
 *
 * When OpENer is used as a library, multiple CIP adapters may use it
 * and may want to set the VendorId.  Note: some applications allow
 * the use of VendorId 0.
 */
void SetDeviceVendorId(CipUint vendor_id);

/** @ingroup CIP_API
 * @brief Get device's CIP VendorId
 *
 * @returns the currently used VendorId
 */
CipUint GetDeviceVendorId(void);

/** @ingroup CIP_API
 * @breif Set device's CIP ProductName
 *
 * @param product_name C-string to use as ProducName
 *
 * When OpENer is used as a library, multiple CIP adapters may use it
 * and will need to change the product name.
 */
void SetDeviceProductName(const char *product_name);

/** @ingroup CIP_API
 * @brief Get device's current CIP ProductName
 *
 * Hint, use GetCstrFromCipShortString() to get a printable/logable C
 * string, since CipShortString's aren't NUL terminated.
 *
 * @returns the CipShortString for the product name
 */
CipShortString *GetDeviceProductName(void);

/** @ingroup CIP_API
 * @brief Initialize and setup the CIP-stack
 *
 * @param unique_connection_id value passed to Connection_Manager_Init() to form
 * a "per boot" unique connection ID.
 */
EipStatus CipStackInit(const EipUint16 unique_connection_id);

/** @ingroup CIP_API
 * @brief Shutdown of the CIP stack
 *
 * This will
 *   - close all open I/O connections,
 *   - close all open explicit connections, and
 *   - free all memory allocated by the stack.
 *
 * Memory allocated by the application will not be freed. This has to be done
 * by the application!
 */
void ShutdownCipStack(void);

/** @ingroup CIP_API
 * @brief Enable the Run/Idle header for consumed (O->T) cyclic data
 * @param onoff if set (default), OpENer expects 4 byte Run/Idle header from scanner
 */
void CipRunIdleHeaderSetO2T(bool onoff);

/** @ingroup CIP_API
 * @brief Get current setting of the O->T Run/Idle header
 * @return current setting of the O->T Run/Idle header
 */
bool CipRunIdleHeaderGetO2T(void);

/** @ingroup CIP_API
 * @brief Enable the Run/Idle header for produced (T->O) cyclic data
 * @param onoff if set (not default), OpENer includes a 4 byte Run/Idle header in responses to scanner
 */
void CipRunIdleHeaderSetT2O(bool onoff);

/** @ingroup CIP_API
 * @brief Get current setting of the T->O Run/Idle header
 * @return current setting of the T->O Run/Idle header
 */
bool CipRunIdleHeaderGetT2O(void);

/** @ingroup CIP_API
 * @brief Get a pointer to a CIP object with given class code
 *
 * @param class_code class code of the object to retrieve
 * @return pointer to CIP Object
 *          0 if object is not present in the stack
 */
CipClass *GetCipClass(const CipUdint class_code);

/** @ingroup CIP_API
 * @brief Get a pointer to an instance
 *
 * @param cip_object pointer to the object the instance belongs to
 * @param instance_number number of the instance to retrieve
 * @return pointer to CIP Instance
 *          0 if instance is not in the object
 */
CipInstance *GetCipInstance(const CipClass *RESTRICT const cip_object,
                            const CipInstanceNum instance_number);

/** @ingroup CIP_API
 * @brief Get a pointer to an instance's attribute
 *
 * As instances and objects are selfsimilar this function can also be used
 * to retrieve the attribute of an object.
 * @param cip_instance  pointer to the instance the attribute belongs to
 * @param attribute_number number of the attribute to retrieve
 * @return pointer to attribute
 *          0 if instance is not in the object
 */
CipAttributeStruct *GetCipAttribute(const CipInstance *const cip_instance,
                                    const EipUint16 attribute_number);

typedef void (*InitializeCipClass)(CipClass *); /**< Initializer function for CIP class initialization */

/** @ingroup CIP_API
 * @brief Allocate memory for new CIP Class and attributes
 *
 *  The new CIP class will be registered at the stack to be able
 *  for receiving explicit messages.
 *
 *  @param class_code class code of the new class
 *  @param number_of_class_attributes number of class attributes
 *  @param highest_class_attribute_number Highest attribute number from the set of implemented class attributes
 *  @param number_of_class_services number of class services
 *  @param number_of_instance_attributes Number of implemented instance attributes
 *  @param highest_instance_attribute_number Highest attribute number from the set of implemented instance attributes
 *  @param number_of_instance_services number of instance services
 *  @param number_of_instances number of initial instances to create
 *  @param name class name (for debugging class structure)
 *  @param revision class revision
 *  @param initializer For non-standard implementation of
 *  class attributes, function realizes specific implementation
 *  @return pointer to new class object
 *      0 on error
 */
CipClass *CreateCipClass(const CipUdint class_code,
                         const int number_of_class_attributes,
                         const EipUint32 highest_class_attribute_number,
                         const int number_of_class_services,
                         const int number_of_instance_attributes,
                         const EipUint32 highest_instance_attribute_number,
                         const int number_of_instance_services,
                         const CipInstanceNum number_of_instances,
                         const char *const name,
                         const EipUint16 revision,
                         InitializeCipClass initializer);

/** @ingroup CIP_API
 * @brief Add a number of CIP instances to a given CIP class
 *
 * The required number of instances are attached to the class as a linked list.
 *
 * The instances are numbered sequentially -- i.e. the first node in the chain
 * is instance 1, the second is 2, and so on.
 * You can add new instances at any time (you do not have to create all the
 * instances of a class at the same time) deleting instances once they have
 * been created is not supported out-of-order instance numbers are not
 * supported running out of memory while creating new instances causes an
 * assert.
 *
 * @param cip_object_to_add_instances CIP object the instances should be added
 * @param number_of_instances number of instances to be generated.
 * @return pointer to the first of the new instances
 *              0 on error
 */
CipInstance *AddCipInstances(
  CipClass *RESTRICT const cip_object_to_add_instances,
  const CipInstanceNum number_of_instances);

/** @ingroup CIP_API
 * @brief Create one instance of a given class with a certain instance number
 *
 * This function can be used for creating out of order instance numbers
 * @param cip_class_to_add_instance the class the instance should be created for
 * @param instance_id the instance id of the created instance
 * @return pointer to the created instance, if an instance with the given id
 *         already exists the existing is returned an no new instance is created
 *
 */
CipInstance *AddCipInstance(CipClass *RESTRICT const cip_class_to_add_instance,
                            const CipInstanceNum instance_id);

/** @ingroup CIP_API
 * @brief Insert an attribute in an instance of a CIP class
 *
 *  Note that attributes are stored in an array pointer in the instance
 *  the attributes array is not expandable if you insert an attributes that has
 *  already been defined, the previous attributes will be replaced
 *
 *  @param cip_instance Pointer to CIP class instance (Instance 0)
 *  @param attribute_number Number of attribute to be inserted.
 *  @param cip_data_type Type of attribute to be inserted.
 *  @param encode_function Function pointer to the encoding function
 *  @param decode_function Function pointer to the decoding function
 *  @param cip_data Pointer to data of attribute.
 *  @param cip_flags Flags to indicate set-ability and get-ability of attribute.
 */
void InsertAttribute(CipInstance *const instance,
                     const EipUint16 attribute_number,
                     const EipUint8 cip_type,
                     CipAttributeEncodeInMessage encode_function,
                     CipAttributeDecodeFromMessage decode_function,
                     void *const data,
                     const EipByte cip_flags);

/** @ingroup CIP_API
 * @brief Allocates Attribute bitmasks
 *
 * @param target_class Class, in which the bitmasks will be inserted.
 *
 */
void AllocateAttributeMasks(CipClass *target_class);

/** @ingroup CIP_API
 * @brief Calculates Byte-Index of Attribute
 *
 * @param attribute_number Attribute number.
 *
 */
size_t CalculateIndex(EipUint16 attribute_number);

/** @ingroup CIP_API
 * @brief Insert a service in an instance of a CIP object
 *
 *  Note that services are stored in an array pointer in the class object
 *  the service array is not expandable if you insert a service that has
 *  already been defined, the previous service will be replaced
 *
 * @param cip_class pointer to CIP object. (may be also
 * instance# 0)
 * @param service_code service code of service to be inserted.
 * @param service_function pointer to function which represents the service.
 * @param service_name name of the service
 */
void InsertService(const CipClass *const cip_class,
                   const EipUint8 service_code,
                   const CipServiceFunction service_function,
                   char *const service_name);

/** @ingroup CIP_API
 * @brief Insert a Get or Set callback for a CIP class
 *
 * @param cip_class pointer to the target CIP object
 * @param callback_function the callback function to insert
 * @param callbacks_to_install  flags to select the affected callbacks
 *
 * This function inserts the provided @p callback_function into selected
 *  callback function entries of the CIP class @p cip_class.
 * The callback targets are selected by @p callbacks_to_install that may
 *  be an ORed mask of kPreGetFunc, kPostGetFunc, kPreSetFunc, kPostSetFunc
 *  and kNvDataFunc.
 * If either the kPostSetFunc or kNvDataFunc is set the same function
 *  pointer CipClass::PostSetCallback will be called.
 */
void InsertGetSetCallback(CipClass *const cip_class,
                          CipGetSetCallback callback_function,
                          CIPAttributeFlag callbacks_to_install);

//TODO: Update documentation
/** @ingroup CIP_API
 * @brief Produce the data according to CIP encoding onto the message buffer.
 *
 * This function may be used in own services for sending data back to the
 * requester (e.g., getAttributeSingle for special structs).
 *  @param cip_data_type the cip type to encode
 *  @param cip_data pointer to data value.
 *  @param message_router_response The message router response construct
 */

void EncodeCipBool(const void *const data,
                   ENIPMessage *const outgoing_message);

void EncodeCipByte(const void *const data,
                   ENIPMessage *const outgoing_message);

void EncodeCipWord(const void *const data,
                   ENIPMessage *const outgoing_message);

void EncodeCipDword(const void *const data,
                    ENIPMessage *const outgoing_message);

void EncodeCipLword(const void *const data,
                    ENIPMessage *const outgoing_message);

void EncodeCipUsint(const void *const data,
                    ENIPMessage *const outgoing_message);

void EncodeCipUint(const void *const data,
                   ENIPMessage *const outgoing_message);

void EncodeCipUdint(const void *const data,
                    ENIPMessage *const outgoing_message);

void EncodeCipUlint(const void *const data,
                    ENIPMessage *const outgoing_message);

void EncodeCipSint(const void *const data,
                   ENIPMessage *const outgoing_message);

void EncodeCipInt(const void *const data,
                  ENIPMessage *const outgoing_message);

void EncodeCipDint(const void *const data,
                   ENIPMessage *const outgoing_message);

void EncodeCipLint(const void *const data,
                   ENIPMessage *const outgoing_message);

void EncodeCipReal(const void *const data,
                   ENIPMessage *const outgoing_message);

void EncodeCipLreal(const void *const data,
                    ENIPMessage *const outgoing_message);

void EncodeCipShortString(const void *const data,
                          ENIPMessage *const outgoing_message);

void EncodeCipString(const void *const data,
                     ENIPMessage *const outgoing_message);

void EncodeCipString2(const void *const data,
                      ENIPMessage *const outgoing_message);

void EncodeCipStringN(const void *const data,
                      ENIPMessage *const outgoing_message);

void EncodeCipStringI(const void *const data,
                      ENIPMessage *const outgoing_message);

void EncodeCipByteArray(const void *const data,
                        ENIPMessage *const outgoing_message);

void EncodeCipEPath(const void *const data,
                    ENIPMessage *const outgoing_message); //path_size UINT

void EncodeEPath(const void *const data,
                 ENIPMessage *const outgoing_message); //path_size not encoded

void EncodeCipEthernetLinkPhyisicalAddress(const void *const data,
                                           ENIPMessage *const outgoing_message);


/** @ingroup CIP_API
 * @brief Retrieve the given data according to CIP encoding from the message
 * buffer.
 *
 * This function may be used in own services for handling data from the
 * requester (e.g., setAttributeSingle).
 *  @param data pointer to value to be written.
 *  @param message_router_request pointer to the request where the data should be taken from
 *  @param message_router_response pointer to the response where status should be set
 *  @return length of taken bytes
 *          -1 .. error
 */
int DecodeCipBool(CipBool *const data,
                  CipMessageRouterRequest *const message_router_request,
                  CipMessageRouterResponse *const message_router_response);

int DecodeCipByte(CipByte *const data,
                  CipMessageRouterRequest *const message_router_request,
                  CipMessageRouterResponse *const message_router_response);

int DecodeCipByteArray(CipByteArray *const data,
                       const CipMessageRouterRequest *const message_router_request,
                       CipMessageRouterResponse *const message_router_response);

int DecodeCipWord(CipWord *const data,
                  CipMessageRouterRequest *const message_router_request,
                  CipMessageRouterResponse *const message_router_response);

int DecodeCipDword(CipDword *const data,
                   CipMessageRouterRequest *const message_router_request,
                   CipMessageRouterResponse *const message_router_response);

int DecodeCipLword(CipLword *const data,
                   CipMessageRouterRequest *const message_router_request,
                   CipMessageRouterResponse *const message_router_response);

int DecodeCipUsint(CipUsint *const data,
                   CipMessageRouterRequest *const message_router_request,
                   CipMessageRouterResponse *const message_router_response);

int DecodeCipUint(CipUint *const data,
                  CipMessageRouterRequest *const message_router_request,
                  CipMessageRouterResponse *const message_router_response);

int DecodeCipUdint(CipUdint *const data,
                   CipMessageRouterRequest *const message_router_request,
                   CipMessageRouterResponse *const message_router_response);

int DecodeCipUlint(CipUlint *const data,
                   CipMessageRouterRequest *const message_router_request,
                   CipMessageRouterResponse *const message_router_response);

int DecodeCipSint(CipSint *const data,
                  CipMessageRouterRequest *const message_router_request,
                  CipMessageRouterResponse *const message_router_response);

int DecodeCipInt(CipInt *const data,
                 CipMessageRouterRequest *const message_router_request,
                 CipMessageRouterResponse *const message_router_response);

int DecodeCipDint(CipDint *const data,
                  CipMessageRouterRequest *const message_router_request,
                  CipMessageRouterResponse *const message_router_response);

int DecodeCipLint(CipLint *const data,
                  CipMessageRouterRequest *const message_router_request,
                  CipMessageRouterResponse *const message_router_response);

int DecodeCipReal(CipReal *const data,
                  CipMessageRouterRequest *const message_router_request,
                  CipMessageRouterResponse *const message_router_response);

int DecodeCipLreal(CipLreal *const data,
                   CipMessageRouterRequest *const message_router_request,
                   CipMessageRouterResponse *const message_router_response);

int DecodeCipString(CipString *const data,
                    CipMessageRouterRequest *const message_router_request,
                    CipMessageRouterResponse *const message_router_response);

int DecodeCipShortString(CipShortString *const data,
                         CipMessageRouterRequest *const message_router_request,
                         CipMessageRouterResponse *const message_router_response);

/** @ingroup CIP_API
 * @brief Create an instance of an assembly object
 *
 * @param instance_number  instance number of the assembly object to create
 * @param data         pointer to the data the assembly object should contain
 * @param data_length   length of the assembly object's data
 * @return pointer to the instance of the created assembly object. NULL on error
 *
 * Assembly Objects for Configuration Data:
 *
 * The CIP stack treats configuration assembly objects the same way as any other
 * assembly object.
 * In order to support a configuration assembly object it has to be created with
 * this function.
 * The notification on received configuration data is handled with the
 * AfterAssemblyDataReceived.
 */
CipInstance *CreateAssemblyObject(const CipInstanceNum instance_number,
                                  EipByte *const data,
                                  const EipUint16 data_length);

typedef struct cip_connection_object CipConnectionObject;

/** @ingroup CIP_API
 * @brief Function prototype for handling the opening of connections
 *
 * @param connection_object The connection object which is opening the
 * connection
 * @param extended_error_code The returned error code of the connection object
 *
 * @return CIP error code
 */
typedef CipError (*OpenConnectionFunction)(
  CipConnectionObject *RESTRICT const connection_object,
  EipUint16 *const extended_error_code);

/** @ingroup CIP_API
 * @brief Function prototype for handling the closing of connections
 *
 * @param connection_object The connection object which is closing the
 * connection
 */
typedef void (*ConnectionCloseFunction)(CipConnectionObject *RESTRICT
                                        connection_object);

/** @ingroup CIP_API
 * @brief Function prototype for handling the timeout of connections
 *
 * @param connection_object The connection object which connection timed out
 */
typedef void (*ConnectionTimeoutFunction)(
  CipConnectionObject *connection_object);

/** @ingroup CIP_API
 * @brief Function prototype for sending data via a connection
 *
 * @param connection_object The connection object which connection timed out
 *
 * @return EIP stack status
 */
typedef EipStatus (*ConnectionSendDataFunction)(CipConnectionObject *
                                                connection_object);

/** @ingroup CIP_API
 * @brief Function prototype for receiving data via a connection
 *
 * @param connection_object The connection object which connection timed out
 * @param data The payload of the CIP message
 * @param data_length Length of the payload
 *
 * @return Stack status
 */
typedef EipStatus (*ConnectionReceiveDataFunction)(CipConnectionObject *
                                                   connection_object,
                                                   const EipUint8 *data,
                                                   const EipUint16 data_length);

/** @ingroup CIP_API
 * @brief Function pointer for timeout checker functions
 *
 * @param elapsed_time elapsed time since last check
 */
typedef void (*TimeoutCheckerFunction)(const MilliSeconds elapsed_time);

/** @ingroup CIP_API
 * @brief register open functions for an specific object.
 *
 * With this function any object can be enabled to be a target for forward
 * open/close request.
 * @param class_code The class code
 * @param open_connection_function Pointer to the function handling the open
 * process
 * @return EIP_OK on success
 */
EipStatus
AddConnectableObject(const CipUdint class_code,
                     OpenConnectionFunction open_connection_function);

/** @ingroup CIP_API
 * @brief Configures the connection point for an exclusive owner connection.
 *
 * @param connection_number The number of the exclusive owner connection. The
 *        enumeration starts with 0. Has to be smaller than
 *        OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS.
 * @param output_assembly_id ID of the O-to-T point to be used for this
 * connection
 * @param input_assembly_id ID of the T-to-O point to be used for this
 * connection
 * @param configuration_assembly_id ID of the configuration point to be used for
 * this connection
 */
void ConfigureExclusiveOwnerConnectionPoint(
  const unsigned int connection_number,
  const unsigned int output_assembly_id,
  const unsigned int input_assembly_id,
  const unsigned int configuration_assembly_id);

/** @ingroup CIP_API
 * @brief Configures the connection point for an input only connection.
 *
 * @param connection_number The number of the input only connection. The
 *        enumeration starts with 0. Has to be smaller than
 *        OPENER_CIP_NUM_INPUT_ONLY_CONNS.
 * @param output_assembly_id ID of the O-to-T point to be used for this
 * connection
 * @param input_assembly_id ID of the T-to-O point to be used for this
 * connection
 * @param configuration_assembly_id ID of the configuration point to be used for
 * this connection
 */
void ConfigureInputOnlyConnectionPoint(const unsigned int connection_number,
                                       const unsigned int output_assembly_id,
                                       const unsigned int input_assembly_id,
                                       const unsigned int configuration_assembly_id);

/** \ingroup CIP_API
 * \brief Configures the connection point for a listen only connection.
 *
 * @param connection_number The number of the input only connection. The
 *        enumeration starts with 0. Has to be smaller than
 *        OPENER_CIP_NUM_LISTEN_ONLY_CONNS.
 * @param output_assembly_id ID of the O-to-T point to be used for this
 * connection
 * @param input_assembly_id ID of the T-to-O point to be used for this
 * connection
 * @param configuration_assembly_id ID of the configuration point to be used for
 * this connection
 */
void ConfigureListenOnlyConnectionPoint(const unsigned int connection_number,
                                        const unsigned int output_assembly_id,
                                        const unsigned int input_assembly_id,
                                        const unsigned int configuration_assembly_id);

/** @ingroup CIP_API
 * @brief Notify the encapsulation layer that an explicit message has been
 * received via TCP.
 *
 * @param socket_handle Socket handle from which data is received.
 * @param buffer Buffer that contains the received data. This buffer will also
 * contain the response if one is to be sent.
 * @param length Length of the data in buffer.
 * @param number_of_remaining_bytes Return how many bytes of the input are left
 * over after we're done here
 * @param originator_address Address struct of the message originator
 * @param outgoing_message The outgoing ENIP message
 * @return kEipStatusOkSend: a response needs to be sent, others: EIP stack status
 */
EipStatus HandleReceivedExplictTcpData(int socket_handle,
                                       EipUint8 *buffer,
                                       size_t length,
                                       int *number_of_remaining_bytes,
                                       struct sockaddr *originator_address,
                                       ENIPMessage *const outgoing_message);

/** @ingroup CIP_API
 * @brief Notify the encapsulation layer that an explicit message has been
 * received via UDP.
 *
 * @param socket_handle socket handle from which data is received.
 * @param from_address remote address from which the data is received.
 * @param buffer buffer that contains the received data. This buffer will also
 * contain the response if one is to be sent.
 * @param buffer_length length of the data in buffer.
 * @param number_of_remaining_bytes return how many bytes of the input are left
 * over after we're done here
 * @param unicast Was the data received as unicast message?
 * @param outgoing_message Outgoing ENIP message
 * @return kEipStatusOkSend: a response needs to be sent, others: EIP stack status
 */
EipStatus HandleReceivedExplictUdpData(const int socket_handle,
                                       const struct sockaddr_in *from_address,
                                       const EipUint8 *buffer,
                                       const size_t buffer_length,
                                       int *number_of_remaining_bytes,
                                       bool unicast,
                                       ENIPMessage *const outgoing_message);

/** @ingroup CIP_API
 *  @brief Notify the connection manager that data for a connection has been
 *  received.
 *
 *  This function should be invoked by the network layer.
 *  @param received_data pointer to the buffer of data that has been received
 *  @param received_data_length number of bytes in the data buffer
 *  @param from_address address from which the data has been received. Only
 *           data from the connections originator may be accepted. Avoids
 *           connection hijacking
 *  @return EIP_OK on success
 */
EipStatus HandleReceivedConnectedData(const EipUint8 *const received_data,
                                      int received_data_length,
                                      struct sockaddr_in *from_address);

/** @ingroup CIP_API
 * @brief Check if any of the connection timers (TransmissionTrigger or
 * WatchdogTimeout) have timed out.
 *
 * If the a timeout occurs the function performs the necessary action. This
 * function should be called periodically once every @ref kOpenerTimerTickInMilliSeconds
 * milliseconds. In order to simplify the algorithm if more time was lapsed, the elapsed
 * time since the last call of the function is given as a parameter.
 *
 * @param elapsed_time Elapsed time in milliseconds since the last call of ManageConnections
 *
 * @return EIP_OK on success
 */
EipStatus ManageConnections(MilliSeconds elapsed_time);

/** @ingroup CIP_API
 * @brief Trigger the production of an application triggered connection.
 *
 * This will issue the production of the specified connection at the next
 * possible occasion. Depending on the values for the RPI and the production
 * inhibit timer. The application is informed via the
 * EIP_BOOL8 BeforeAssemblyDataSend(S_CIP_Instance *pa_pstInstance)
 * callback function when the production will happen. This function should only
 * be invoked from void HandleApplication(void).
 *
 * The connection can only be triggered if the application is established and it
 * is of application application triggered type.
 *
 * @param output_assembly_id the output assembly connection point of the
 * connection
 * @param input_assembly_id the input assembly connection point of the
 * connection
 * @return EIP_OK on success
 */
EipStatus TriggerConnections(unsigned int output_assembly_id,
                             unsigned int input_assembly_id);

/** @ingroup CIP_API
 * @brief Inform the encapsulation layer that the remote host has closed the
 * connection.
 *
 * According to the specifications that will clean up and close the session in
 * the encapsulation layer.
 * @param socket_handle the handler to the socket of the closed connection
 */
void CloseSession(int socket_handle);

/**  @defgroup CIP_CALLBACK_API Callback Functions Demanded by OpENer
 * @ingroup CIP_API
 *
 * @brief These functions have to implemented in order to give the OpENer a
 * method to inform the application on certain state changes.
 */

/** @ingroup CIP_CALLBACK_API
 * @brief Callback for the application initialization
 *
 * This function will be called by the CIP stack after it has finished its
 * initialization. In this function the user can setup all CIP objects she
 * likes to have.
 *
 * This function is provided for convenience reasons. After the void
 * CipStackInit(void)
 * function has finished it is okay to also generate your CIP objects.
 *  return status EIP_ERROR .. error
 *                EIP_OK ... successful finish
 */
EipStatus ApplicationInitialization(void);

/** @ingroup CIP_CALLBACK_API
 * @brief Allow the device specific application to perform its execution
 *
 * This function will be executed by the stack at the beginning of each
 * execution of EIP_STATUS ManageConnections(void). It allows to implement
 * device specific application functions. Execution within this function should
 * be short.
 */
void HandleApplication(void);

/** @ingroup CIP_CALLBACK_API
 * @brief Inform the application on changes occurred for a connection
 *
 * @param output_assembly_id the output assembly connection point of the
 * connection
 * @param input_assembly_id the input assembly connection point of the
 * connection
 * @param io_connection_event information on the change occurred
 */
void CheckIoConnectionEvent(unsigned int output_assembly_id,
                            unsigned int input_assembly_id,
                            IoConnectionEvent io_connection_event);

/** @ingroup CIP_CALLBACK_API
 * @brief Call back function to inform application on received data for an
 * assembly object.
 *
 * This function has to be implemented by the user of the CIP-stack.
 * @param instance pointer to the assembly object data was received for
 * @return Information if the data could be processed
 *     - EIP_OK the received data was ok
 *     - EIP_ERROR the received data was wrong (especially needed for
 * configuration data assembly objects)
 *
 * Assembly Objects for Configuration Data:
 * The CIP-stack uses this function to inform on received configuration data.
 * The length of the data is already checked within the stack. Therefore the
 * user only has to check if the data is valid.
 */
EipStatus AfterAssemblyDataReceived(CipInstance *instance);

/** @ingroup CIP_CALLBACK_API
 * @brief Inform the application that the data of an assembly
 * object will be sent.
 *
 * Within this function the user can update the data of the assembly object
 * before it gets sent. The application can inform the application if data has
 * changed.
 * @param instance instance of assembly object that should send data.
 * @return data has changed:
 *          - true assembly data has changed
 *          - false assembly data has not changed
 */
EipBool8 BeforeAssemblyDataSend(CipInstance *instance);

/** @ingroup CIP_CALLBACK_API
 * @brief Emulate as close a possible a power cycle of the device
 *
 * @return if the service is supported the function will not return.
 *     EIP_ERROR if this service is not supported
 */
EipStatus ResetDevice(void);

/** @ingroup CIP_CALLBACK_API
 * @brief Reset the device to the initial configuration and emulate as close as
 * possible a power cycle of the device
 *
 * @return if the service is supported the function will not return.
 *     EIP_ERROR if this service is not supported
 */
EipStatus ResetDeviceToInitialConfiguration(void);

/** @ingroup CIP_CALLBACK_API
 * @brief Allocate memory for the CIP stack
 *
 * emulate the common c-library function calloc
 * In OpENer allocation only happens on application startup and on
 * class/instance creation and configuration not on during operation
 * (processing messages).
 * @param number_of_elements number of elements to allocate
 * @param size_of_element size in bytes of one element
 * @return pointer to the allocated memory, 0 on error
 */
void *CipCalloc(size_t number_of_elements,
                size_t size_of_element);

/** @ingroup CIP_CALLBACK_API
 * @brief Free memory allocated by the OpENer
 *
 * emulate the common c-library function free
 * @param data pointer to the allocated memory
 */
void CipFree(void *data);

/** @ingroup CIP_CALLBACK_API
 * @brief Inform the application that the Run/Idle State has been changed
 * by the originator.
 *
 * @param run_idle_value the current value of the run/idle flag according to CIP
 * spec Vol 1 3-6.5
 */
void RunIdleChanged(EipUint32 run_idle_value);

/** @ingroup CIP_CALLBACK_API
 * @brief Create the UDP socket for the implicit IO messaging,
 * one socket handles all connections
 * @return the socket handle if successful, else kEipInvalidSocket
 */
int CreateUdpSocket(void);

/** @ingroup CIP_CALLBACK_API
 * @brief Sends the data for the implicit IO messaging via UDP socket
 * @param socket_data Address message to be sent
 * @param outgoing message The constructed outgoing message
 * @return kEipStatusOk on success
 */
EipStatus SendUdpData(const struct sockaddr_in *const socket_data,
                      const ENIPMessage *const outgoing_message);

/** @ingroup CIP_CALLBACK_API
 * @brief Close the given socket and clean up the stack
 *
 * @param socket_handle socket descriptor to close
 */
void CloseSocket(const int socket_handle);

/** @ingroup CIP_CALLBACK_API
 * @brief Register function pointer in timeout_checker_array
 *
 * @param timeout_checker_function pointer to object specific timeout checker function
 */
void RegisterTimeoutChecker(TimeoutCheckerFunction timeout_checker_function);

/** @mainpage OpENer - Open Source EtherNet/IP(TM) Communication Stack
 * Documentation
 *
 * EtherNet/IP stack for adapter devices (connection target); supports multiple
 * I/O and explicit connections; includes features and objects required by the
 * CIP specification to enable devices to comply with ODVA's conformance/
 * interoperability tests.
 *
 * @section intro_sec Introduction
 *
 * This is the introduction.
 *
 * @section install_sec Building
 * How to compile, install and run OpENer on a specific platform.
 *
 * @subsection build_req_sec Requirements
 * OpENer has been developed to be highly portable. The default version targets
 * PCs with a POSIX operating system and a BSD-socket network interface. To
 * test this version we recommend a Linux PC or Windows with Cygwin installed.
 *  You will need to have the following installed:
 *   - gcc, make, binutils, etc.
 *
 * for normal building. These should be installed on most Linux installations
 * and are part of the development packages of Cygwin.
 *
 * For the development itself we recommend the use of Eclipse with the CDT
 * plugin. For your convenience OpENer already comes with an Eclipse project
 * file. This allows to just import the OpENer source tree into Eclipse.
 *
 * @subsection compile_pcs_sec Compile for PCs
 *   -# Directly in the shell
 *       -# Go into the bin/pc directory
 *       -# Invoke make
 *       -# For invoking opener type:\n
 *          ./opener ipaddress subnetmask gateway domainname hostaddress
 * macaddress\n
 *          e.g., ./opener 192.168.0.2 255.255.255.0 192.168.0.1 test.com
 * testdevice 00 15 C5 BF D0 87
 *   -# Within Eclipse
 *       -# Import the project
 *       -# Go to the bin/pc folder in the make targets view
 *       -# Choose all from the make targets
 *       -# The resulting executable will be in the directory
 *           ./bin/pc
 *       -# The command line parameters can be set in the run configuration
 * dialog of Eclipse
 *
 * @section further_reading_sec Further Topics
 *   - @ref porting
 *   - @ref extending
 *   - @ref license
 *
 * @page porting Porting OpENer
 * @section gen_config_section General Stack Configuration
 * The general stack properties have to be defined prior to building your
 * production. This is done by providing a file called opener_user_conf.h. An
 * example file can be found in the src/ports/POSIX or src/ports/WIN32 directory.
 * The documentation of the example file for the necessary configuration options:
 * opener_user_conf.h
 *
 * @copydoc ./ports/POSIX/sample_application/opener_user_conf.h
 *
 * @section startup_sec Startup Sequence
 * During startup of your EtherNet/IP(TM) device the following steps have to be
 * performed:
 *   -# Configure the network properties:\n
 *       With the following functions the network interface of OpENer is
 *       configured:
 *        - EIP_STATUS ConfigureNetworkInterface(const char *ip_address,
 *        const char *subnet_mask, const char *gateway_address)
 *        - void ConfigureMACAddress(const EIP_UINT8 *mac_address)
 *        - void ConfigureDomainName(const char *domain_name)
 *        - void ConfigureHostName(const char *host_name)
 *        .
 *       Depending on your platform these data can come from a configuration
 *       file or from operating system functions. If these values should be
 *       setable remotely via explicit messages the SetAttributeSingle functions
 *       of the EtherNetLink and the TCPIPInterface object have to be adapted.
 *   -# Set the device's serial number\n
 *      According to the CIP specification a device vendor has to ensure that
 *      each of its devices has a unique 32Bit device id. You can set it with
 *      the function:
 *       - void setDeviceSerialNumber(EIP_UINT32 serial_number)
 *   -# Initialize OpENer: \n
 *      With the function CipStackInit(EIP_UINT16 unique_connection_id) the
 *      internal data structures of opener are correctly setup. After this
 *      step own CIP objects and Assembly objects instances may be created. For
 *      your convenience we provide the call-back function
 *      ApplicationInitialization. This call back function is called when the
 * stack is ready to receive application specific CIP objects.
 *   -# Create Application Specific CIP Objects:\n
 *      Within the call-back function ApplicationInitialization(void) or
 *      after CipStackInit(void) has finished you may create and configure any
 *      CIP object or Assembly object instances. See the module @ref CIP_API
 *      for available functions. Currently no functions are available to
 *      remove any created objects or instances. This is planned
 *      for future versions.
 *   -# Setup the listening TCP and UDP port:\n
 *      THE ETHERNET/IP SPECIFICATION demands from devices to listen to TCP
 *      connections and UDP datagrams on the port AF12hex for explicit messages.
 *      Therefore before going into normal operation you need to configure your
 *      network library so that TCP and UDP messages on this port will be
 *      received and can be hand over to the Ethernet encapsulation layer.
 *
 * @section normal_op_sec Normal Operation
 * During normal operation the following tasks have to be done by the platform
 * specific code:
 *   - Establish connections requested on TCP port AF12hex
 *   - Receive explicit message data on connected TCP sockets and the UPD socket
 *     for port AF12hex. The received data has to be hand over to Ethernet
 *     encapsulation layer with the functions: \n
 *      int HandleReceivedExplictTCPData(int socket_handle, EIP_UINT8* buffer, int
 * buffer_length, int *number_of_remaining_bytes),\n
 *      int HandleReceivedExplictUDPData(int socket_handle, struct sockaddr_in
 * *from_address, EIP_UINT8* buffer, unsigned int buffer_length, int
 * *number_of_remaining_bytes).\n
 *     Depending if the data has been received from a TCP or from a UDP socket.
 *     As a result of this function a response may have to be sent. The data to
 *     be sent is in the given buffer pa_buf.
 *   - Create UDP sending and receiving sockets for implicit connected
 * messages\n
 *     OpENer will use to call-back function int CreateUdpSocket(
 *     UdpCommuncationDirection connection_direction,
 *     struct sockaddr_in *pa_pstAddr)
 *     for informing the platform specific code that a new connection is
 *     established and new sockets are necessary
 *   - Receive implicit connected data on a receiving UDP socket\n
 *     The received data has to be hand over to the Connection Manager Object
 *     with the function EIP_STATUS HandleReceivedConnectedData(EIP_UINT8
 * *data, int data_length)
 *   - Close UDP and TCP sockets:
 *      -# Requested by OpENer through the call back function: void
 * CloseSocket(int socket_handle)
 *      -# For TCP connection when the peer closed the connection OpENer needs
 *         to be informed to clean up internal data structures. This is done
 * with
 *         the function void CloseSession(int socket_handle).
 *      .
 *   - Cyclically update the connection status:\n
 *     In order that OpENer can determine when to produce new data on
 *     connections or that a connection timed out every @ref kOpenerTimerTickInMilliSeconds
 * milliseconds the
 *     function EIP_STATUS ManageConnections(void) has to be called.
 *
 * @section callback_funcs_sec Callback Functions
 * In order to make OpENer more platform independent and in order to inform the
 * application on certain state changes and actions within the stack a set of
 * call-back functions is provided. These call-back functions are declared in
 * the file opener_api.h and have to be implemented by the application specific
 * code. An overview and explanation of OpENer's call-back API may be found in
 * the module @ref CIP_CALLBACK_API.
 *
 * @page extending Extending OpENer
 * OpENer provides an API for adding own CIP objects and instances with
 * specific services and attributes. Therefore OpENer can be easily adapted to
 * support different device profiles and specific CIP objects needed for your
 * device. The functions to be used are:
 *   - CipClass *CreateCipClass( const CipUdint class_code,
   const int number_of_class_attributes,
   const EipUint32 highest_class_attribute_number,
   const int number_of_class_services,
   const int number_of_instance_attributes,
   const EipUint32 highest_instance_attribute_number,
   const int number_of_instance_services,
   const int number_of_instances,
   char *name,
   const EipUint16 revision,
   InitializeCipClass initializer );
 *   - CipInstance *AddCipInstances(CipClass *RESTRICT const cip_class,
   const int number_of_instances)
 *   - CipInstance *AddCipInstance(CipClass *RESTRICT const class,
   const EipUint32 instance_id)
 *   - void InsertAttribute(CipInstance *const cip_instance,
   const EipUint16 attribute_number,
   const EipUint8 cip_data_type,
   void *const cip_data,
   const EipByte cip_flags);
 *   - void InsertService(const CipClass *const cip_class_to_add_service,
   const EipUint8 service_code,
   const CipServiceFunction service_function,
   char *const service_name);
 *
 * @page license OpENer Open Source License
 * The OpENer Open Source License is an adapted BSD style license. The
 * adaptations include the use of the term EtherNet/IP(TM) and the necessary
 * guarding conditions for using OpENer in own products. For this please look
 * in license text as shown below:
 *
 * @include "../license.txt"
 *
 */

#endif /*OPENER_OPENER_API_H_*/
