/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#ifndef CIP_API_H_
#define CIP_API_H_

#include "typedefs.h"
#include "ciptypes.h"
#include "ciperror.h"
#include <opener_user_conf.h>

/**  \defgroup CIP_API OpENer User interface
 * \brief This is the public interface of the OpENer. It provides all function needed to implement an EtherNet/IP enabled slave-device.
 */

/*! \ingroup CIP_API
 * \brief Configure the data of the network interface of the device
 * 
 *  This function setup the data of the network interface needed by OpENer.
 *  The multicast address is automatically calculated from he given data.
 * 
 *  @param pa_acIpAdress    the current ip address of the device
 *  @param pa_acSubNetMask  the subnetmask to be used
 *  @param pa_acGateway     the gateway address 
 *  @return EIP_OK if the configuring worked otherwise EIP_ERROR
 */
EIP_STATUS
configureNetworkInterface(const char *pa_acIpAdress,
    const char *pa_acSubNetMask, const char *pa_acGateway);

/*! \ingroup CIP_API 
 * \brief Configure the MAC address of the device
 * 
 *  @param pa_acMACAddress  the hardware MAC address of the network interface
 */
void
configureMACAddress(const EIP_UINT8 *pa_acMACAddress);

/*! \ingroup CIP_API 
 * \brief Configure the domain name of the device
 *  @param pa_acDomainName the domain name to be used
 */
void
configureDomainName(const char *pa_acDomainName);

/*! \ingroup CIP_API 
 * \brief Configure the host name of the device
 *  @param pa_acHostName the host name to be used
 */
void
configureHostName(const char *pa_acHostName);

/*! \ingroup CIP_API
 * Set the serial number of the device's identity object.
 * 
 * @param pa_nSerialNumber unique 32 bit number identifying the device
 */
void
setDeviceSerialNumber(EIP_UINT32 pa_nSerialNumber);

/** \ingroup CIP_API 
 * \brief Initialize and setup the CIP-stack
 * 
 * @param pa_nUniqueConnID value passed to Connection_Manager_Init()to form a
 *                      "per boot" unique connection ID.
 *
 */
void
CIP_Init(EIP_UINT16 pa_nUniqueConnID);


/** \ingroup CIP_API
 * \brief Shutdown the CIP-stack
 *
 * This will
 *   - close all open I/O connections,
 *   - close all open explicit connections, and
 *   - free all memory allocated by the stack.
 *
 * Memory allocated by the application will not be freed. This has to be done
 * by the application!
 */
void shutdownCIP(void);

/** \ingroup CIP_API 
 * \brief Get a pointer to a CIP object with given class code
 * 
 * @param pa_nClassID class ID of the object to retrieve 
 * @return pointer to CIP Object
 *          0 if object is not present in the stack
 */
S_CIP_Class *
getCIPClass(EIP_UINT32 pa_nClassID);

/** \ingroup CIP_API 
 * \brief Get a pointer to an instance
 * 
 * @param pa_pstObject pointer to the object the instance belongs to
 * @param pa_nInstanceNr number of the instance to retrieve
 * @return pointer to CIP Instance
 *          0 if instance is not in the object
 */
S_CIP_Instance *
getCIPInstance(S_CIP_Class *pa_pstObject, EIP_UINT32 pa_nInstanceNr);

/** \ingroup CIP_API 
 * \brief Get a pointer to an instance's attribute
 * 
 * As instances and objects are selfsimilar this function can also be used
 * to retrieve the attribute of an object. 
 * @param pa_pInstance  pointer to the instance the attribute belongs to
 * @param pa_nAttributeNr number of the attribute to retrieve
 * @return pointer to attribute
 *          0 if instance is not in the object
 */
S_CIP_attribute_struct *
getAttribute(S_CIP_Instance * pa_pInstance, EIP_UINT8 pa_nAttributeNr);

/*! \ingroup CIP_API 
 * \brief Allocate memory for new CIP Class and attributes
 *  The new CIP class will be registered at the stack to be able
 *  for receiving explicit messages.
 * 
 *  @param pa_nClassID class ID of the new class
 *  @param pa_nNr_of_ClassAttributes number of class attributes
 *  @param pa_nClassGetAttrAllMask mask of which attributes are included in the class getAttributeAll
 *  @param pa_nNr_of_ClassServices number of class services
 *  @param pa_nNr_of_InstanceAttributes number of attributes of each instance
 *  @param pa_nInstGetAttrAllMask  mask of which attributes are included in the instance getAttributeAll
 *  @param pa_nNr_of_InstanceServices number of instance services
 *  @param pa_nNr_of_Instances number of initial instances to create
 *  @param pa_acName  class name (for debugging class structure)
 *  @param pa_nRevision class revision
 *  @return pointer to new class object
 *      0 on error
 */
S_CIP_Class *
createCIPClass(EIP_UINT32 pa_nClassID, int pa_nNr_of_ClassAttributes,
    EIP_UINT32 pa_nClassGetAttrAllMask, int pa_nNr_of_ClassServices,
    int pa_nNr_of_InstanceAttributes, EIP_UINT32 pa_nInstGetAttrAllMask,
    int pa_nNr_of_InstanceServices, int pa_nNr_of_Instances, char *pa_acName,
    EIP_UINT16 pa_nRevision);

/** \ingroup CIP_API 
 * \brief Add a number of CIP instances to a given CIP class
 *
 * The required number of instances are created in a block, but are attached to the class as a linked list.
 * the instances are numbered sequentially -- i.e. the first node in the chain is instance 1, the second is 2, etc.
 * you can add new instances at any time (you do not have to create all the instances of a class at the same time)
 * deleting instances once they have been created is not supported
 * out-of-order instance numbers are not supported
 * running out of memory while creating new instances causes an assert
 *
 * @param pa_pstCIPObject CIP object the instances should be added
 * @param pa_nNr_of_Instances number of instances to be generated.
 * @return pointer to the first of the new instances
 *              0 on error
 */
S_CIP_Instance *
addCIPInstances(S_CIP_Class *pa_pstCIPObject, int pa_nNr_of_Instances);

/** \ingroup CIP_API 
 * \brief Create one instance of a given class with a certain instance number
 *
 * This function can be used for creating out of order instance numbers
 * @param pa_pstCIPClass the class the instance should be created for
 * @param pa_nInstanceId the instance id of the created instance
 * @return pointer to the created instance, if an instance with the given id 
 *         already exists the existing is returned an no new instance is created
 * 
 */
S_CIP_Instance *
addCIPInstance(S_CIP_Class * pa_pstCIPClass, EIP_UINT32 pa_nInstanceId);

/*! \ingroup CIP_API 
 * Insert an attribute in an instance of a CIP class
 *  @param pa_pInstance pointer to CIP class. (may be also instance 0)
 *  @param pa_nAttributeNr number of attribute to be inserted.
 *  @param pa_nCIP_Type type of attribute to be inserted.
 *  @param pa_pt2data pointer to data of attribute.
 */
void
insertAttribute(S_CIP_Instance *pa_pInstance, EIP_UINT32 pa_nAttributeNr,
    EIP_UINT8 pa_nCIP_Type, void* pa_pt2data);

/** \ingroup CIP_API 
 * \brief Insert a service in an instance of a CIP object
 *  Note that services are stored in an array pointer to by the class object
 *  the service array is not expandable if you insert a service that has 
 *  already been defined, the previous service will be replaced
 * 
 * @param pa_pClass pointer to CIP object. (may be also instance 0)
 * @param pa_nServiceNr servicecode of service to be inserted.
 * @param pa_ptfuncService pointer to function which represents the service.
 * @param name name of the service
 */
void
insertService(S_CIP_Class *pa_pClass, EIP_UINT8 pa_nServiceNr,
    TCIPServiceFunc pa_ptfuncService, char *name);

/** \ingroup CIP_API 
 * \brief Create an instance of an assembly object
 * 
 * @param pa_nInstanceID  instance number of the assembly object to create 
 * @param pa_data         pointer to the data the assembly object should contain
 * @param pa_datalength   length of the assembly object's data
 * @return pointer to the instance of the created assembly object. NULL on error
 *
 * Assembly Objects for Configuration Data:
 *
 * The CIP stack treats configuration assembly objects the same way as any other assembly object. 
 * In order to support a configuration assembly object it has to be created with this function.
 * The notification on received configuration data is handled with the IApp_after_receive function.
 */
S_CIP_Instance *
createAssemblyObject(EIP_UINT32 pa_nInstanceID, EIP_BYTE *pa_data,
    EIP_UINT16 pa_datalength);

/** \ingroup CIP_API
 * Configures the connection point for an exclusive owner connection.
 *
 * @param pa_unConnNum the number of the exclusive owner connection. The
 *        enumeration starts with 0. Has to be smaller than
 *        OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS.
 * @param pa_unOutputAssembly the O-to-T point to be used for this connection
 * @param pa_unInputAssembly the T-to-O point to be used for this connection
 * @param pa_unConfigAssembly the config point to be used for this connection
 */
void
configureExclusiveOwnerConnectionPoint(unsigned int pa_unConnNum,
    unsigned int pa_unOutputAssembly, unsigned int pa_unInputAssembly,
    unsigned int pa_unConfigAssembly);

/** \ingroup CIP_API
 * Configures the connection point for an input only connection.
 *
 * @param pa_unConnNum the number of the input only connection. The
 *        enumeration starts with 0. Has to be smaller than
 *        OPENER_CIP_NUM_INPUT_ONLY_CONNS.
 * @param pa_unOutputAssembly the O-to-T point to be used for this connection
 * @param pa_unInputAssembly the T-to-O point to be used for this connection
 * @param pa_unConfigAssembly the config point to be used for this connection
 */
void
configureInputOnlyConnectionPoint(unsigned int pa_unConnNum,
    unsigned int pa_unOutputAssembly, unsigned int pa_unInputAssembly,
    unsigned int pa_unConfigAssembly);

/** \ingroup CIP_API
 * Configures the connection point for a listen only connection.
 *
 * @param pa_unConnNum the number of the input only connection. The
 *        enumeration starts with 0. Has to be smaller than
 *        OPENER_CIP_NUM_LISTEN_ONLY_CONNS.
 * @param pa_unOutputAssembly the O-to-T point to be used for this connection
 * @param pa_unInputAssembly the T-to-O point to be used for this connection
 * @param pa_unConfigAssembly the config point to be used for this connection
 */
void
configureListenOnlyConnectionPoint(unsigned int pa_unConnNum,
    unsigned int pa_unOutputAssembly, unsigned int pa_unInputAssembly,
    unsigned int pa_unConfigAssembly);

/** \ingroup CIP_API 
 * Notify the encapsulation layer that an explicit message has been received via TCP or UDP.
 * 
 * @param pa_socket socket handle from which data is received.
 * @param pa_buf buffer that contains the recieved data. This buffer will also contain the 
 *       response if one is to be sent.  
 * @param pa_length length of the data in pa_buf.
 * @param pa_nRemainingBytes return how many bytes of the input are left over after we're done here
 * @return length of reply that need to be sent back
 */
int
handleReceivedExplictData(int pa_socket, EIP_UINT8* pa_buf, int pa_length,
    int *pa_nRemainingBytes);

/*! \ingroup CIP_API
 *  Notfiy the connection manager that data for a connection has been received.
 *  This function should be invoked by the network layer.
 *  @param pa_pnData pointer to the buffer of data that has been recieved 
 *  @param pa_nDataLength number of bytes in the data buffer
 *  @return EIP_OK on success
 */
EIP_STATUS
handleReceivedConnectedData(EIP_UINT8 *pa_pnData, int pa_nDataLength);

/*! \ingroup CIP_API
 * Check if any of the connection timers (TransmissionTrigger or WarchdogTimeout) has timed out.
 * If yes the function performs the necessary action. This function should be called periodically once every
 * OPENER_TIMER_TICK ms. 
 * 
 * @return EIP_OK on success
 */
EIP_STATUS
manageConnections(void);

/*! \ingroup CIP_API
 * Inform the encapsulation layer that the remote host has closed the connection.
 * According to the specifciations that will clean up and close the session in the encapsulation layer.
 * @param pa_nSocket the handler to the socket of the closed connection
 */
void
closeSession(int pa_nSocket);

/**  \defgroup CIP_CALLBACK_API Callback Functions Demanded by OpENer
 * \ingroup CIP_API
 * 
 * \brief These functions have to implemented in order to give the OpENer a method to inform the application on certain state changes
 */

/** \ingroup CIP_CALLBACK_API 
 * \brief Callback for the application initialization
 *
 * This function will be called by the CIP stack after it has finished its
 * initialization. In this function the user can setup all CIP objects she
 * likes to have.
 *
 * This function is provided for convenience reasons. After the void CIP_Init(void)
 * function has finished it is okay to also generate your CIP objects.
 *  return status EIP_ERROR .. error
 *                EIP_OK ... successful finish
 */
EIP_STATUS
IApp_Init(void);


/** \ingroup CIP_CALLBACK_API
 * \brief Inform the application on changes occurred for a connection
 *
 * @param pa_unOutputAssembly the output assembly connection point of the connection
 * @param pa_unInputAssembly the input assembly connection point of the connection
 * @param pa_eIOConnectionEvent information on the change occurred
 */
void
IApp_IOConnectionEvent(unsigned int pa_unOutputAssembly,
    unsigned int pa_unInputAssembly, EIOConnectionEvent pa_eIOConnectionEvent);

/** \ingroup CIP_CALLBACK_API 
 * \brief Call back function to inform application on received data for an assembly object.
 * 
 * This function has to be implemented by the user of the CIP-stack.
 * @param pa_pstInstance pointer to the assembly object data was received for
 * @return Information if the data could be processed
 *     - EIP_OK the received data was ok 
 *     - EIP_ERROR the received data was wrong (especially needed for configuration data assembly
 *                 objects) 
 * 
 * Assembly Objects for Configuration Data:
 * The CIP-stack uses this function to inform on received configuration data. The length of the data
 * is already checked within the stack. Therefore the user only has to check if the data is valid.
 */
EIP_STATUS
IApp_AfterAssemblyDataReceived(S_CIP_Instance *pa_pstInstance);
/** \ingroup CIP_CALLBACK_API 
 * \brief Inform the application that the data of an assembly
 * object will be sent.
 *
 * Within this function the user can update the data of the assembly object before it
 * gets sent. The application can inform the application if data has changed.
 * @param pa_pstInstance instance of assembly object that should send data.
 * @return data has changed:
 *          - true assembly data has changed
 *          - false assembly data has not changed
 */
EIP_BOOL8
IApp_BeforeAssemblyDataSend(S_CIP_Instance *pa_pstInstance);

/** \ingroup CIP_CALLBACK_API 
 * \brief Emulate as close a possible a power cycle of the device
 *  
 * @return if the service is supported the function will not return.
 *     EIP_ERROR if this service is not supported
 */
EIP_STATUS
IApp_ResetDevice(void);

/**\ingroup CIP_CALLBACK_API 
 * \brief Reset the device to the initial configuration and emulate as close as possible a power cycle of the device
 * 
 * return if the service is supported the function will not return.
 *     EIP_ERROR if this service is not supported
 */
EIP_STATUS
IApp_ResetDeviceToInitialConfiguration(void);

/**\ingroup CIP_CALLBACK_API 
 * \brief Allocate memory for the cip stack
 * 
 * emulate the common c-library function calloc
 * In OpENer allocation only happens on application startup and on class/instance creation
 * and configuration not on during operation (processing messages)
 * @param pa_nNumberOfElements number of elements to allocate
 * @param pa_nSizeOfElement size in bytes of one element
 * return pointer to the allocated memory, 0 on error
 */
void *
IApp_CipCalloc(unsigned pa_nNumberOfElements, unsigned pa_nSizeOfElement);

/**\ingroup CIP_CALLBACK_API 
 * \brief Free memory allocated by the OpENer
 *
 * emulate the common c-library function free
 * @param pointer to the allocated memory
 * return
 */
void IApp_CipFree(void *pa_poData);

/**\ingroup CIP_CALLBACK_API
 * \brief Inform the application that the Run/Idle State has been changed
 *  by the originator. 
 * 
 * 
 * @param pa_nRunIdleValue  the current value of the run/idle flag according to CIP spec Vol 1 3-6.5
 */
void
IApp_RunIdleChanged(EIP_UINT32 pa_nRunIdleValue);

/**\ingroup CIP_CALLBACK_API 
 * \brief create a producing or consuming UDP socket
 * 
 * @param pa_nDirection PRODCUER or CONSUMER
 * @param pa_pstAddr pointer to the address holding structure
 *     Attention: For producing point-to-point connection the pa_pstAddr->sin_addr.s_addr
 *         member is set to 0 by OpENer. The network layer of the application has
 *         to set the correct address of the originator.
 *     Attention: For consuming connection the network layer has to set the pa_pstAddr->sin_addr.s_addr
 *         to the correct address of the originator.
 * FIXME add an additional parameter that can be used by the CIP stack to request the originators sockaddr_in
 * data.
 * @return socket identifier on success
 *         -1 on error 
 */
int
IApp_CreateUDPSocket(int pa_nDirection, struct sockaddr_in *pa_pstAddr);

/**\ingroup CIP_CALLBACK_API 
 * \brief create a producing or consuming UDP socket
 * 
 * @param pa_pstAddr pointer to the sendto address
 * @param pa_nSockFd socket descriptor to send on
 * @param pa_acData pointer to the data to send
 * @param pa_nDataLength length of the data to send
 * @return  EIP_SUCCESS on success
 */
EIP_STATUS
IApp_SendUDPData(struct sockaddr_in *pa_pstAddr, int pa_nSockFd,
    EIP_UINT8 *pa_acData, EIP_UINT16 pa_nDataLength);

/**\ingroup CIP_CALLBACK_API 
 * \brief Close the given socket and clean up the stack 
 * 
 * @param pa_nSockFd socket descriptor to close
 */
void
IApp_CloseSocket(int pa_nSockFd);

/*! \mainpage OpENer - Open Source EtherNet/IP(TM) Communication Stack Documentation
 *
 * EtherNet/IP stack for adapter devices (connection target); supports multiple
 * I/O and explicit connections; includes features and objects required by the
 * CIP specification to enable devices to comply with ODVA's conformance/
 * interoperability tests.
 * 
 * \section intro_sec Introduction
 *
 * This is the introduction.
 *
 * \section install_sec Building
 * How to compile, install and run OpENer on a specific platform.
 * \subsection build_req_sec Requirements
 * OpENer has been developed to be highly portable. The default version targets PCs
 * with a POSIX operating system and a BSD-socket network interface. To test this
 * version we recommend a Linux PC or Windows with Cygwin (www.cygwin.com)
 * installed. You will need to have the following installed:
 *   - gcc, make, binutils, etc.
 * 
 * for normal building. These should be installed on most Linux installations and
 * are part of the development packages of cygwin.
 * 
 * For the development itself we recommend the use of Eclipse with the CTD plugin
 * (www.eclipse.org). For your convenience OpENer already comes with an Eclipse
 * project file. This allows to just import the OpENer source tree into Eclipse.
 * 
 * \subsection compile_pcs_sec Compile for PCs
 *   -# Directly in the shell
 *       -# Go into the bin/pc directory
 *       -# Invoke make
 *       -# For invoking opener type:\n
 *          ./opener ipaddress subnetmask gateway domainname hostaddress macaddress\n
 *          e.g., ./opener 192.168.0.2 255.255.255.0 192.168.0.1 test.com testdevice 00 15 C5 BF D0 87 
 *   -# Within Eclipse
 *       -# Import the project
 *       -# Go to the bin/pc folder in the make targets view
 *       -# Choose all from the make targets
 *       -# The resulting executable will be in the directory
 *           ./bin/pc
 *       -# The command line parameters can be set in the run configuration dialog of Eclipse
 * 
 * \section further_reading_sec Further Topics
 *   - \ref porting
 *   - \ref extending
 *   - \ref license 
 * 
 * \page porting Porting OpENer
 * \section gen_config_section General Stack Configuration
 * The general stack properties have to be defined prior to building your 
 * production. This is done by providing a file called opener_user_conf.h. An 
 * example file can be found in the src/ports/platform-pc directory. The 
 * documentation of the example file for the necessary configuration options: 
 * opener_user_conf.h
 * 
 * \copydoc opener_user_conf.h
 * 
 * \section startup_sec Startup Sequence
 * During startup of your EtherNet/IP(TM) device the following steps have to be 
 * performed:
 *   -# Configure the network properties:\n
 *       With the following functions the network interface of OpENer is 
 *       configured:
 *        - EIP_STATUS configureNetworkInterface(const char *pa_acIpAdress, const char *pa_acSubNetMask, const char *pa_acGateway)
 *        - void configureMACAddress(const EIP_UINT8 *pa_acMACAddress)
 *        - void configureDomainName(const char *pa_acDomainName)
 *        - void configureHostName(const char *pa_acHostName)
 *        .
 *       Depending on your platform these data can come from a configuration 
 *       file or from operating system functions. If these values should be 
 *       settable remotely via explicit messages the SetAttributeSingle functions
 *       of the EtherNetLink and the TCPIPInterface object have to be adapted.
 *   -# Set the device's serial number\n
 *      According to the CIP specification a device vendor has to ensure that
 *      each of its devices has a unique 32Bit device id. You can set it with
 *      the function:
 *       - void setDeviceSerialNumber(EIP_UINT32 pa_nSerialNumber)
 *   -# Initialize OpENer: \n
 *      With the function CIP_Init(void) the internal data structures of opener are 
 *      correctly setup. After this step own CIP objects and Assembly objects 
 *      instances may be created. For your convenience we provide the call-back 
 *      function IApp_Init(void). This call back function is called when the stack is
 *      ready to receive application specific CIP objects.
 *   -# Create Application Specific CIP Objects:\n
 *      Within the call-back function IApp_Init(void) or after CIP_Init(void) 
 *      has finished you may create and configure any CIP object or Assembly object instances. See 
 *      the module \ref CIP_API for available functions. Currently no functions 
 *      are available to remove any created objects or instances. This is planned
 *      for future versions.
 *   -# Setup the listening TCP and UDP port:\n
 *      THE ETHERNET/IP SPECIFICATION demands from devices to listen to TCP 
 *      connections and UDP datagrams on the port AF12hex for explicit messages.
 *      Therefore before going into normal operation you need to configure your 
 *      network library so that TCP and UDP messages on this port will be 
 *      received and can be hand over to the Ethernet encapsulation layer.
 * 
 * \section normal_op_sec Normal Operation
 * During normal operation the following tasks have to be done by the platform
 * specific code:
 *   - Establish connections requested on TCP port AF12hex
 *   - Receive explicit message data on connected TCP sockets and the UPD socket
 *     for port AF12hex. The received data has to be hand over to Ethernet
 *     encapsulation layer with the function int handleReceivedExplictData(int pa_socket, EIP_UINT8* pa_buf, int pa_length, int *pa_nRemainingBytes).\n
 *     As a result of this function a response may have to be sent. The data to
 *     be sent is in the given buffer pa_buf.
 *   - Create UDP sending and receiving sockets for implicit connected messages\n
 *     OpENer will use to call-back function int IApp_CreateUDPSocket(int pa_nDirection, struct sockaddr_in *pa_pstAddr)
 *     for informing the platform specific code that a new connection is 
 *     established and new sockets are necessary
 *   - Receive implicit connected data on a receiving UDP socket\n
 *     The received data has to be hand over to the Connection Manager Object 
 *     with the function EIP_STATUS handleReceivedConnectedData(EIP_UINT8 *pa_pnData, int pa_nDataLength)
 *   - Close UDP and TCP sockets:
 *      -# Requested by OpENer through the call back function: void IApp_CloseSocket(int pa_nSockFd)
 *      -# For TCP connection when the peer closed the connection OpENer needs 
 *         to be informed to clean up internal data structures. This is done with
 *         the function void closeSession(int pa_nSocket).
 *      .
 *   - Cyclically update the connection status:\n
 *     In order that OpENer can determine when to produce new data on 
 *     connections or that a connection timed out every \ref OPENER_TIMER_TICK ms the 
 *     function EIP_STATUS manageConnections(void) has to be called.
 * 
 * \section callback_funcs_sec Callback Functions
 * In order to make OpENer more platform independent and in order to inform the 
 * application on certain state changes and actions within the stack a set of 
 * call-back functions is provided. These call-back functions are declared in 
 * the file opener_api.h and have to be implemented by the application specific 
 * code. An overview and explanation of OpENer's call-back API may be found in 
 * the module \ref CIP_CALLBACK_API.
 *  
 * \page extending Extending OpENer
 * OpENer provides an API for adding own CIP objects and instances with
 * specific services and attributes. Therefore OpENer can be easily adapted to
 * support different device profiles and specific CIP objects needed for your
 * device. The functions to be used are:
 *   - S_CIP_Class * createCIPClass(EIP_UINT32 pa_nClassID, int pa_nNr_of_ClassAttributes, EIP_UINT32 pa_nClassGetAttrAllMask, int pa_nNr_of_ClassServices, int pa_nNr_of_InstanceAttributes, EIP_UINT32 pa_nInstGetAttrAllMask, int pa_nNr_of_InstanceServices, int pa_nNr_of_Instances, char *pa_acName, EIP_UINT16 pa_nRevision);
 *   - S_CIP_Instance * addCIPInstances(S_CIP_Class *pa_pstCIPObject, int pa_nNr_of_Instances);
 *   - S_CIP_Instance * addCIPInstance(S_CIP_Class * pa_pstCIPClass, EIP_UINT32 pa_nInstanceId);
 *   - void insertAttribute(S_CIP_Instance *pa_pInstance, EIP_UINT8 pa_nAttributeNr, EIP_UINT8 pa_nCIP_Type, void* pa_pt2data);
 *   - void insertService(S_CIP_Class *pa_pClass, EIP_UINT8 pa_nServiceNr, TCIPServiceFunc pa_ptfuncService, char *name);
 * 
 * \page license OpENer Open Source License
 * The OpENer Open Source License is an adapted BSD style license. The 
 * adaptations include the use of the term EtherNet/IP(TM) and the necessary 
 * guarding conditions for using OpENer in own products. For this please look 
 * in license text as shown below:
 * 
 * \include "license.txt"  
 * 
 */

#endif /*CIP_API_H_*/

