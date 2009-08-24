/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#ifndef CIP_API_H_
#define CIP_API_H_

#include "typedefs.h"
#include "ciptypes.h"
#include "ciperror.h"
#include <opener_user_conf.h>

/*! \mainpage OpENer - Open Source EtherNet/IP(TM) I/O Target Stack Documentation
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
 * \section install_sec Installation
 * How to compile, install and run OpENer on a specific platform.
 * 
 * \section porting_sec Porting OpENer
 * 
 * \section extetnding_sec Extending OpENer
 * 
 */  


/**  \defgroup CIP_API OpENer User interface
 * \brief This is the public interface of the OpENer. It provides all function needed to implement an EtherNet/IP enabled slave-device.
 */

/*! \ingroup CIP_API \brief configure the data of the network interface of the device
 * 
 *  This function setup the data of the network interface needed by OpENer.
 *  The multicast address is automatically calculated fromt he given data.
 * 
 *  @param pa_acIpAdress    the current ip address of the device
 *  @param pa_acSubNetMask  the subnetmask to be used
 *  @param pa_acGateway     the gateway address 
 *  @return EIP_OK if the configuring worked otherwise EIP_ERROR
 */
EIP_STATUS configureNetworkInterface(const char *pa_acIpAdress,
    const char *pa_acSubNetMask, const char *pa_acGateway);

/*! \ingroup CIP_API \brief configure the MAC address of the device
 * 
 *  @param pa_acMACAddress  the hardware MAC address of the network interface
 */
void configureMACAddress(const EIP_UINT8 *pa_acMACAddress);

/*! \ingroup CIP_API \brief configure the domain name of the device
 *  @param pa_acDomainName the domain name to be used
 */
void configureDomainName(const char *pa_acDomainName);

/*! \ingroup CIP_API \brief configure the host name of the device
 *  @param pa_acHostName the host name to be used
 */
void configureHostName(const char *pa_acHostName);

/** \ingroup CIP_API \brief Initialize and setup the CIP-stack
 * 
 */
void CIP_Init(void);

/** \ingroup CIP_API Get a pointer to a cip object with given class code
 * 
 * @param pa_nClassID class ID of the object to retrieve 
 * @return pointer to CIP Object
 *          0 if object is not present in the stack
 */
S_CIP_Class *getCIPClass(EIP_UINT32 pa_nClassID);

/** \ingroup CIP_API Get a pointer to an instance
 * 
 * @param pa_pstObject pointer to the object the instance belongs to
 * @param pa_nInstanceNr number of the instance to retrieve
 * @return pointer to CIP Instance
 *          0 if instance is not in the object
 */
S_CIP_Instance *getCIPInstance(S_CIP_Class *pa_pstObject,
    EIP_UINT16 pa_nInstanceNr);

/** \ingroup CIP_API Get a pointer to an instance's attribute
 * 
 * As instances and objects are selsimilar this function can also be used
 * to retrieve the attribute of an object. 
 * @param pa_pInstance  pointer to the instance the attribute belongs to
 * @param pa_nAttributeNr number of the attribute to retrieve
 * @return poitner to attribute
 *          0 if instance is not in the object
 */
S_CIP_attribute_struct *getAttribute(S_CIP_Instance * pa_pInstance,
    EIP_UINT8 pa_nAttributeNr);

/*! \ingroup CIP_API Allocate memory for new CIP Class and attributes;
 *  and register the new CIP class at the stack to be able
 *  for recieving ecplicit messages
 * 
 *  @param pa_nClassID class ID of the new class
 *  @param pa_nNr_of_ClassAttributes number of class attributes
 *  @param pa_nClassGetAttrAllMask mask of which attributes are included in the class getAttributeAll
 *  @param pa_nNr_of_ClassServices number of class services
 *  @param pa_nNr_of_InstanceAttributes number of attributes of each instance
 *  @param pa_nInstGetAttrAllMask  mask of which attributes are included in the instance getAttributeAll
 *  @param pa_nNr_of_InstanceServices number of instance services
 *  @param pa_nNr_of_Instances number of initial instances to creat
 *  @param pa_acName  class name (for debugging class structure)
 *  @param pa_nRevision class revision
 *  @return pointer to new class object
 *      0 on error
 */
S_CIP_Class *createCIPClass(EIP_UINT32 pa_nClassID, int pa_nNr_of_ClassAttributes, EIP_UINT32 pa_nClassGetAttrAllMask, int pa_nNr_of_ClassServices,
    int pa_nNr_of_InstanceAttributes, EIP_UINT32 pa_nInstGetAttrAllMask, int pa_nNr_of_InstanceServices, 
    int pa_nNr_of_Instances, char *pa_acName, EIP_UINT16 pa_nRevision);

/** \ingroup CIP_API \brief Add a number of cip instances to a given cip class
 *
 * the required number of instances are created in a block, but are attached to the class as a linked list.
 * the instances are numbered sequentially -- i.e. the first node in the chain is instance 1, the second is 2, etc.
 * you can add new instances at any time (you do not have to create all the instances of a class at the same time)
 * deleting instances once they have been created is not supported
 * out-of-order instance numbers are not supported
 * running out of memory while creating new instances causes an assert
 *
 * @param pa_pstCIPObject CIP object the instances should be added
 * @param pa_nNr_of_Instances number of instances to be generated.
 * @return return pointer to the first of the new instances
 * 	        0 on error
 */
S_CIP_Instance *addCIPInstances(S_CIP_Class *pa_pstCIPObject,
    int pa_nNr_of_Instances);

/** \ingroup CIP_API \brief Create one instance of a given class with a certain instance number
 *
 * This function can be used for creating out of order instance numbers
 * @param pa_pstCIPClass the class the instance should be created for
 * @param pa_nInstanceId the instance id of the created instance
 * @return pointer to the created instance, if an instance with the given id 
 *         already exists the exisiting is returned an no new instance is created
 * 
 */
S_CIP_Instance *addCIPInstance(S_CIP_Class * pa_pstCIPClass,
    EIP_UINT32 pa_nInstanceId);

/*! \ingroup CIP_API Insert an attribute in an instance of a CIP class
 *  @param pa_pInstance pointer to CIP class. (may be also instance 0)
 *  @param pa_nAttributeNr number of attribute to be inserted.
 *  @param pa_nCIP_Type type of attribute to be inserted.
 *  @param pa_pt2data pointer to data of attribute.
 */
void insertAttribute(S_CIP_Instance *pa_pInstance, EIP_UINT8 pa_nAttributeNr,
    EIP_UINT8 pa_nCIP_Type, void* pa_pt2data);

/** \ingroup CIP_API \brief Insert a service in an instance of a CIP object
 *  note that services are stored in an array pointer to by the class object
 *  the service array is not expandable if you insert a service that has 
 *  already been defined, the previous service will be replaced
 * 
 * @param pa_pClass pointer to CIP object. (may be also instance 0)
 * @param pa_nServiceNr servicecode of service to be inserted.
 * @param pa_ptfuncService pointer to function which represents the service.
 * @param name name of the service
 */
void insertService(S_CIP_Class *pa_pClass, EIP_UINT8 pa_nServiceNr,
    TCIPServiceFunc pa_ptfuncService, char *name);

/** \ingroup CIP_API \brief Create an instance of an assembly object
 * 
 * @param pa_nInstanceID  instance number of the assembly object to create 
 * @param pa_data         pointer to the data the assembly object should contain
 * @param pa_datalength   lenght of the assembly object's data
 * @return pointer to the instance of the created assembly object. NULL on error
 *
 * Assembly Objects for Configuration Data:
 *
 * The CIP stack treats configuration assembly objects the same way as any other assembly object. 
 * In order to support a configuration assembly object it has to be created with this function.
 * The notification on recieved configuration data is handled with the IApp_after_receive function.
 */
S_CIP_Instance *createAssemblyObject(EIP_UINT8 pa_nInstanceID,
    EIP_BYTE *pa_data, EIP_UINT16 pa_datalength);


/** \ingroup CIP_API 
 * Notify the encapsulation layer that an explicit message has been recieved via TCP or UDP.
 * 
 * @param pa_socket socket handle from which data is received.
 * @param pa_buf buffer to be read.
 * @param pa_length length of the data in pa_buf.
 * @param pa_nRemainingBytes return how many bytes of the input are left over after we're done here
 * @return length of reply that need to be sent back
 */
int handleReceivedExplictData(int pa_socket, EIP_UINT8* pa_buf, int pa_length,
    int *pa_nRemainingBytes);

/*! \ingroup CIP_API
 *  Notfiy the connection manager that data for a connection has been recieved.
 *  This function should be invoked by the network layer.
 *  @param pa_pnData pointer to the buffer of data that has been recieved 
 *  @param pa_nDataLength number of bytes in the data buffer
 *  @return EIP_OK on success
 */ 
EIP_STATUS handleReceivedConnectedData(EIP_UINT8 *pa_pnData, int pa_nDataLength);

/*! \ingroup CIP_API
 * Check if any of the connection timers (TransmissionTrigger or WarchdogTimeout) has timed out.
 * If yes the function performs the necessary action. This function should be called periodicaly once every
 * OPENER_TIMER_TICK ms. 
 * 
 * @return EIP_OK on success
 */
EIP_STATUS manageConnections(void); 

/**  \defgroup CIP_CALLBACK_API Callback function demanded by the CIP Stack
 * \ingroup CIP_API
 * 
 * \brief These functions have to implemented in order to give the CIP stack a method to inform the application on certain state changes
 */

/** \ingroup CIP_CALLBACK_API \brief Callback for the application initilisation
 *
 * This function will be called by the CIP stack after it has finished its initialisation. In this function the user can setup all CIP objects she likes to have.
 * TODO think if this is still the right way to do this. Maybe just after the CIP_INIT would be fine to. 
 *
 *  return status -1 .. error.
 */
EIP_STATUS IApp_Init(void);

/** \ingroup CIP_CALLBACK_API \brief Call back function to inform application on recieved data for an assembly object.
 * 
 * This function has to be implemented by the user of the CIP-stack.
 * @param pa_pstInstance pointer to the assembly object data was received for
 * @return Information if the data could be processed
 *     - EIP_OK the received data was ok 
 *     - EIP_ERROR the received data was wrong (especially needed for configuration data assembly
 *                 objects) 
 * 
 * Assembly Objects for Configuration Data:
 * The CIP-stack uses this function to inform on recieved configuration data. The length of the data
 * is already checked within the stack. Therefore the user only has to check if the data is valid.
 */
EIP_STATUS IApp_AfterAssemblyDataReceived(S_CIP_Instance *pa_pstInstance);

/** \ingroup CIP_CALLBACK_API \brief Inform the application that the data of an assembly
 * object will be sent.
 *
 * Within this function the user can update the data of the assembly object before it
 * gets sent. The application can inform the application if data has changed.
 * @param pa_pstInstance instance of assembly object that should send data.
 * @return data has changed:
 *          - true assembly data has changed
 *          - false assembly data has not changed
 */
EIP_BOOL8 IApp_BeforeAssemblyDataSend(S_CIP_Instance *pa_pstInstance);

/** \ingroup CIP_CALLBACK_API \brief Emulate as close a possible a power cycle of the device
 *  
 * @return if the service is supported the function will not return.
 *     EIP_ERROR if this service is not supported
 */
EIP_STATUS IApp_ResetDevice(void);

/**\ingroup CIP_CALLBACK_API \brief Reset the device to the initial configuration and emulate as close as possible a power cycle of the device
 * 
 * return if the service is supported the function will not return.
 *     EIP_ERROR if this service is not supported
 */
EIP_STATUS IApp_ResetDeviceToInitialConfiguration(void);

/**\ingroup CIP_CALLBACK_API \brief Allocate memory for the cip stack
 * 
 * emulate the common c-libary function calloc
 * In OpENer allocation only happens on application startup and on class/instance creation
 * and configuration not on during operation (processing messages)
 * @param pa_nNumberOfElements number of elements to allocate
 * @param pa_nSizeOfElement size in bytes of one element
 * return pointer to the allocated memory, 0 on error
 */
void *IApp_CipCalloc(unsigned pa_nNumberOfElements, unsigned pa_nSizeOfElement);

/**\ingroup CIP_CALLBACK_API \brief Inform the application that the Run/Idle State has been changed
 *  by the originator. 
 * 
 * 
 * @param pa_nRunIdleValue  the current value of the run/idle flag according to CIP spec Vol 1 3-6.5
 */
void IApp_RunIdleChanged(EIP_UINT32 pa_nRunIdleValue);

/**\ingroup CIP_CALLBACK_API \brief create a producing or consuming UDP socket
 * 
 * @param pa_nDirection PRODCUER or CONSUMER
 * @param pa_pstAddr pointer to the address holding structure
 * @return socket identfier on success
 *         -1 on error 
 */
int IApp_CreateUDPSocket(int pa_nDirection, struct sockaddr_in *pa_pstAddr); // direction: PRODUCER or CONSUMER

/**\ingroup CIP_CALLBACK_API \brief create a producing or consuming UDP socket
 * 
 * @param pa_pstAddr pointer to the sendto address
 * @param pa_nSockFd socket descriptor to send on
 * @param pa_acData pointer to the data to send
 * @param pa_nDataLength length of the data to send
 * @return  EIP_SUCCESS on success
 */
EIP_STATUS IApp_SendUDPData(struct sockaddr_in *pa_pstAddr, int pa_nSockFd,
    EIP_UINT8 *pa_acData, EIP_UINT16 pa_nDataLength);

/**\ingroup CIP_CALLBACK_API \brief Close the given socket and clean up the stack 
 * 
 * @param pa_nSockFd socket descriptor to close
 */
void IApp_CloseSocket(int pa_nSockFd);

#endif /*CIP_API_H_*/
