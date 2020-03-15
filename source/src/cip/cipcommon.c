/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <string.h>
#include <stdio.h>

#include "cipcommon.h"

#include "opener_user_conf.h"
#include "opener_api.h"
#include "cipidentity.h"
#include "ciptcpipinterface.h"
#include "cipethernetlink.h"
#include "cipconnectionmanager.h"
#include "endianconv.h"
#include "encap.h"
#include "ciperror.h"
#include "cipassembly.h"
#include "cipmessagerouter.h"
#if defined(OPENER_IS_DLR_DEVICE) && 0 != OPENER_IS_DLR_DEVICE
  #include "cipdlr.h"
#endif
#include "cipqos.h"
#include "cpf.h"
#include "trace.h"
#include "appcontype.h"
#include "cipepath.h"
#include "stdlib.h"

/* global public variables */
EipUint8 g_message_data_reply_buffer[OPENER_MESSAGE_DATA_REPLY_BUFFER]; /**< Reply buffer */

/* private functions*/
int EncodeEPath(CipEpath *epath,
                EipUint8 **message);

void CipStackInit(const EipUint16 unique_connection_id) {
  /* The message router is the first CIP object be initialized!!! */
  EipStatus eip_status = CipMessageRouterInit();
  OPENER_ASSERT(kEipStatusOk == eip_status)
  eip_status = CipIdentityInit();
  OPENER_ASSERT(kEipStatusOk == eip_status)
  eip_status = CipTcpIpInterfaceInit();
  OPENER_ASSERT(kEipStatusOk == eip_status)
  eip_status = CipEthernetLinkInit();
  OPENER_ASSERT(kEipStatusOk == eip_status)
  eip_status = ConnectionManagerInit(unique_connection_id);
  OPENER_ASSERT(kEipStatusOk == eip_status)
  eip_status = CipAssemblyInitialize();
  OPENER_ASSERT(kEipStatusOk == eip_status)
#if defined(OPENER_IS_DLR_DEVICE) && 0 != OPENER_IS_DLR_DEVICE
  eip_status = CipDlrInit();
  OPENER_ASSERT(kEipStatusOk == eip_status);
#endif
  eip_status = CipQoSInit();
  OPENER_ASSERT(kEipStatusOk == eip_status)
  /* the application has to be initialized at last */
  eip_status = ApplicationInitialization();
  OPENER_ASSERT(kEipStatusOk == eip_status)

  /* Shut up compiler warning with traces disabled */
    (void) eip_status;
}

void ShutdownCipStack(void) {
  /* First close all connections */
  CloseAllConnections();
  /* Than free the sockets of currently active encapsulation sessions */
  EncapsulationShutDown();
  /*clean the data needed for the assembly object's attribute 3*/
  ShutdownAssemblies();

  ShutdownTcpIpInterface();

  /*no clear all the instances and classes */
  DeleteAllClasses();
}

EipStatus NotifyClass(const CipClass *RESTRICT const cip_class,
                      CipMessageRouterRequest *const message_router_request,
                      CipMessageRouterResponse *const message_router_response,
                      const struct sockaddr *originator_address,
                      const int encapsulation_session) {

  /* find the instance: if instNr==0, the class is addressed, else find the instance */
  EipUint16 instance_number =
    message_router_request->request_path.instance_number;                     /* get the instance number */
  CipInstance *instance = GetCipInstance(cip_class, instance_number);       /* look up the instance (note that if inst==0 this will be the class itself) */
  if (instance)       /* if instance is found */
  {
    OPENER_TRACE_INFO("notify: found instance %d%s\n", instance_number,
                      instance_number == 0 ? " (class object)" : "");

    CipServiceStruct *service = instance->cip_class->services;             /* get pointer to array of services */
    if (NULL != service)             /* if services are defined */
    {
      for (size_t i = 0; i < instance->cip_class->number_of_services; i++)                   /* seach the services list */
      {
        if (message_router_request->service == service->service_number)                         /* if match is found */
        {
          /* call the service, and return what it returns */
          OPENER_TRACE_INFO("notify: calling %s service\n",
                            service->name);
          OPENER_ASSERT(NULL != service->service_function)
          return service->service_function(instance,
                                           message_router_request,
                                           message_router_response,
                                           originator_address,
                                           encapsulation_session);
        } else {
          service++;
        }
      }
    }
    OPENER_TRACE_WARN("notify: service 0x%x not supported\n",
                      message_router_request->service);
    message_router_response->general_status = kCipErrorServiceNotSupported;             /* if no services or service not found, return an error reply*/
  } else {
    OPENER_TRACE_WARN("notify: instance number %d unknown\n",
                      instance_number);
    /* if instance not found, return an error reply */
    message_router_response->general_status =
      kCipErrorPathDestinationUnknown;
    /* according to the test tool this is the correct error flag instead of CIP_ERROR_OBJECT_DOES_NOT_EXIST */
  }

  /* handle error replies*/
  message_router_response->size_of_additional_status = 0;       /* fill in the rest of the reply with not much of anything*/
  message_router_response->data_length = 0;
  message_router_response->reply_service = (0x80
                                            | message_router_request->service); /* except the reply code is an echo of the command + the reply flag */

  return kEipStatusOkSend;
}

CipInstance *AddCipInstances(CipClass *RESTRICT const cip_class,
                             const int number_of_instances) {
  CipInstance **next_instance = NULL;
  CipInstance *first_instance = NULL; /* Initialize to error result */
  EipUint32 instance_number = 1;      /* the first instance is number 1 */
  int new_instances   = 0;

  OPENER_TRACE_INFO("adding %d instances to class %s\n", number_of_instances,
                    cip_class->class_name);

  next_instance = &cip_class->instances;       /* get address of pointer to head of chain */
  while (*next_instance)       /* as long as what pp points to is not zero */
  {
    next_instance = &(*next_instance)->next;              /* follow the chain until pp points to pointer that contains a zero */
    instance_number++;                                    /* keep track of what the first new instance number will be */
  }

  /* Allocate and initialize all needed instances one by one. */
  for (new_instances = 0; new_instances < number_of_instances; new_instances++)
  {
    CipInstance *current_instance =
      (CipInstance *) CipCalloc(1, sizeof(CipInstance) );
    OPENER_ASSERT(NULL != current_instance);              /* fail if run out of memory */
    if (NULL == current_instance) {break;}
    if (NULL == first_instance)
    {
      first_instance = current_instance;                  /* remember the first allocated instance */
    }

    current_instance->instance_number = instance_number;  /* assign the next sequential instance number */
    current_instance->cip_class = cip_class;              /* point each instance to its class */

    if (cip_class->number_of_attributes)                  /* if the class calls for instance attributes */
    {                                                     /* then allocate storage for the attribute array */
      current_instance->attributes = (CipAttributeStruct *) CipCalloc(
        cip_class->number_of_attributes, sizeof(CipAttributeStruct) );
      OPENER_ASSERT(NULL != current_instance->attributes);/* fail if run out of memory */
      if (NULL == current_instance->attributes) {break;}
    }

    *next_instance = current_instance;        /* link the previous pointer to this new node */
    next_instance = &current_instance->next;  /* update pp to point to the next link of the current node */
    cip_class->number_of_instances += 1;      /* update the total number of instances recorded by the class */
    instance_number++;                        /* update to the number of the next node*/
  }

  if (new_instances != number_of_instances)
  {
    /* TODO: Free again all attributes and instances allocated so far in this call. */
    OPENER_TRACE_ERR(
      "ERROR: Allocated only %d instances of requested %d for class %s\n",
      new_instances,
      number_of_instances,
      cip_class->class_name);
    first_instance = NULL;  /* failed to allocate all instances / attributes */
  }
  return first_instance;
}

CipInstance *AddCipInstance(CipClass *RESTRICT const cip_class,
                            const EipUint32 instance_id) {
  CipInstance *instance = GetCipInstance(cip_class, instance_id);

  if (NULL == instance) {       /*we have no instance with given id*/
    instance = AddCipInstances(cip_class, 1);
    instance->instance_number = instance_id;
  }
  return instance;
}

CipClass *CreateCipClass(const CipUdint class_code,
                         const EipUint16 number_of_class_attributes,
                         const EipUint16 highest_class_attribute_number,
                         const EipUint16 number_of_class_services,
                         const EipUint16 number_of_instance_attributes,
                         const EipUint16 highest_instance_attribute_number,
                         const EipUint16 number_of_instance_services,
                         const int number_of_instances,
                         char *name,
                         const EipUint16 revision,
                         InitializeCipClass initializer) {

  OPENER_TRACE_INFO("creating class '%s' with code: 0x%" PRIX32 "\n", name,
                    class_code);

  OPENER_ASSERT(NULL == GetCipClass(class_code))  /* check if an class with the ClassID already exists */
  /* should never try to redefine a class*/

  /* a metaClass is a class that holds the class attributes and services
     CIP can talk to an instance, therefore an instance has a pointer to its class
     CIP can talk to a class, therefore a class struct is a subclass of the instance struct,
     and contains a pointer to a metaclass
     CIP never explicitly addresses a metaclass*/

  CipClass *const cip_class = (CipClass *) CipCalloc(1, sizeof(CipClass) );       /* create the class object*/
  CipClass *const meta_class = (CipClass *) CipCalloc(1, sizeof(CipClass) );       /* create the metaclass object*/

  /* initialize the class-specific fields of the Class struct*/
  cip_class->class_code = class_code;       /* the class remembers the class ID */
  cip_class->revision = revision;       /* the class remembers the class ID */
  cip_class->number_of_instances = 0;       /* the number of instances initially zero (more created below) */
  cip_class->instances = 0;
  cip_class->number_of_attributes = number_of_instance_attributes;       /* the class remembers the number of instances of that class */
  cip_class->highest_attribute_number = highest_instance_attribute_number;       /* indicate which attributes are included in instance getAttributeAll */
  cip_class->number_of_services = number_of_instance_services;       /* the class manages the behavior of the instances */
  cip_class->services = 0;
  cip_class->class_name = name;       /* initialize the class-specific fields of the metaClass struct */
  meta_class->class_code = 0xffffffff;       /* set metaclass ID (this should never be referenced) */
  meta_class->number_of_instances = 1;       /* the class object is the only instance of the metaclass */
  meta_class->instances = (CipInstance *) cip_class;
  meta_class->number_of_attributes = number_of_class_attributes + 7;      /* the metaclass remembers how many class attributes exist*/
  meta_class->highest_attribute_number = highest_class_attribute_number;  /* indicate which attributes are included in class getAttributeAll*/
  meta_class->number_of_services = number_of_class_services;              /* the metaclass manages the behavior of the class itself */
  meta_class->class_name = (char *) CipCalloc(1, strlen(name) + 6);       /* fabricate the name "meta<classname>"*/
  snprintf(meta_class->class_name, strlen(name) + 6, "meta-%s", name);

  /* initialize the instance-specific fields of the Class struct*/
  cip_class->class_instance.instance_number = 0;    /* the class object is instance zero of the class it describes (weird, but that's the spec)*/
  cip_class->class_instance.attributes = 0;         /* this will later point to the class attibutes*/
  cip_class->class_instance.cip_class = meta_class; /* the class's class is the metaclass (like SmallTalk)*/
  cip_class->class_instance.next = 0;       /* the next link will always be zero, since there is only one instance of any particular class object */

  meta_class->class_instance.instance_number = 0xffffffff;  /*the metaclass object does not really have a valid instance number*/
  meta_class->class_instance.attributes = NULL;/* the metaclass has no attributes*/
  meta_class->class_instance.cip_class = NULL; /* the metaclass has no class*/
  meta_class->class_instance.next = NULL;      /* the next link will always be zero, since there is only one instance of any particular metaclass object*/

  /* further initialization of the class object*/

  cip_class->class_instance.attributes = (CipAttributeStruct *) CipCalloc(
    meta_class->number_of_attributes, sizeof(CipAttributeStruct) );
  /* TODO -- check that we didn't run out of memory?*/

  meta_class->services = (CipServiceStruct *) CipCalloc(
    meta_class->number_of_services, sizeof(CipServiceStruct) );

  cip_class->services = (CipServiceStruct *) CipCalloc(cip_class->number_of_services,
                                                   sizeof(CipServiceStruct) );

  if (number_of_instances > 0) {
    AddCipInstances(cip_class, number_of_instances);  /*TODO handle return value and clean up if necessary*/
  }

  if (RegisterCipClass(cip_class) == kEipStatusError) {/* no memory to register class in Message Router */
    return 0;             /*TODO handle return value and clean up if necessary*/
  }

  AllocateAttributeMasks(meta_class); /* Allocation of bitmasks for Class Attributes */
  AllocateAttributeMasks(cip_class);  /* Allocation of bitmasks for Instance Attributes */

  if (NULL == initializer) {
    InsertAttribute( (CipInstance *) cip_class, 1, kCipUint,
                     (void *) &cip_class->revision, kGetableSingleAndAll ); /* revision */
    InsertAttribute( (CipInstance *) cip_class, 2, kCipUint,
                     (void *) &cip_class->number_of_instances,
                     kGetableSingleAndAll );                        /*  largest instance number */
    InsertAttribute( (CipInstance *) cip_class, 3, kCipUint,
                     (void *) &cip_class->number_of_instances,
                     kGetableSingleAndAll );                        /* number of instances currently existing*/
    InsertAttribute( (CipInstance *) cip_class, 4, kCipUint,
                     (void *) &kCipUintZero, kGetableAll );         /* optional attribute list - default = 0 */
    InsertAttribute( (CipInstance *) cip_class, 5, kCipUint,
                     (void *) &kCipUintZero, kNotSetOrGetable );    /* optional service list - default = 0 */
    InsertAttribute( (CipInstance *) cip_class, 6, kCipUint,
                     (void *) &meta_class->highest_attribute_number,
                     kGetableSingle );                              /* max class attribute number*/
    InsertAttribute( (CipInstance *) cip_class, 7, kCipUint,
                     (void *) &cip_class->highest_attribute_number,
                     kGetableSingle );                              /* max instance attribute number*/
  } else {
    initializer(cip_class);
  }

  /* create the standard class services*/
  if (number_of_class_services > 0) {
    if (number_of_class_services > 1) {             /*only if the mask has values add the get_attribute_all service */
      InsertService(meta_class, kGetAttributeAll, &GetAttributeAll,
                    "GetAttributeAll");                     /* bind instance services to the metaclass*/
    }
    InsertService(meta_class, kGetAttributeSingle, &GetAttributeSingle,
                  "GetAttributeSingle");
  }
  return cip_class;
}

void InsertAttribute(CipInstance *const instance,
                     const EipUint16 attribute_number,
                     const EipUint8 cip_type,
                     void *const data,
                     const EipByte cip_flags) {

  CipAttributeStruct *attribute = instance->attributes;
  CipClass *cip_class = instance->cip_class;

  OPENER_ASSERT(NULL != attribute)
  /* adding a attribute to a class that was not declared to have any attributes is not allowed */
  for (int i = 0; i < instance->cip_class->number_of_attributes; i++) {
    if (attribute->data == NULL) {             /* found non set attribute */
      attribute->attribute_number = attribute_number;
      attribute->type = cip_type;
      attribute->attribute_flags = cip_flags;
      attribute->data = data;

      OPENER_ASSERT(attribute_number <= cip_class->highest_attribute_number)

      size_t index = CalculateIndex(attribute_number);

      cip_class->get_single_bit_mask[index] |=
        (cip_flags & kGetableSingle) ?
        1 << (attribute_number) % 8 : 0;
      cip_class->get_all_bit_mask[index] |=
        (cip_flags & kGetableAll) ? 1 << (attribute_number) % 8 : 0;
      cip_class->set_bit_mask[index] |=
        ( (cip_flags & kSetable) ? 1 : 0 ) << ( (attribute_number) % 8 );

      return;
    }
    attribute++;
  }
  OPENER_TRACE_ERR(
    "Tried to insert too many attributes into class: %" PRIu32 " '%s', instance %" PRIu32 "\n",
    cip_class->class_code, cip_class->class_name, instance->instance_number);
  OPENER_ASSERT(0)
  /* trying to insert too many attributes*/
}

void InsertService(const CipClass *const cip_class,
                   const EipUint8 service_number,
                   const CipServiceFunction service_function,
                   char *const service_name) {

  CipServiceStruct *service = cip_class->services;  /* get a pointer to the service array*/
  OPENER_TRACE_INFO("%s, number of services:%d, service number:%d\n",
                    cip_class->class_name, cip_class->number_of_services,
                    service_number);
  OPENER_ASSERT(service != NULL)
  /* adding a service to a class that was not declared to have services is not allowed*/
  for (int i = 0; i < cip_class->number_of_services; i++) /* Iterate over all service slots attached to the class */
  {
    if (service->service_number == service_number||
        service->service_function == NULL)         /* found undefined service slot*/
    {
      service->service_number = service_number;                   /* fill in service number*/
      service->service_function = service_function;                   /* fill in function address*/
      service->name = service_name;
      return;
    }
    ++service;
  }
  OPENER_ASSERT(0)
  /* adding more services than were declared is a no-no*/
}

void InsertGetSetCallback
(
  CipClass *const cip_class,
  CipGetSetCallback callback_function,
  CIPAttributeFlag  callbacks_to_install
) {
  if (0 != (kPreGetFunc & callbacks_to_install)) {
    cip_class->PreGetCallback = callback_function;
  }
  if (0 != (kPostGetFunc & callbacks_to_install)) {
    cip_class->PostGetCallback = callback_function;
  }
  if (0 != (kPreSetFunc & callbacks_to_install)) {
    cip_class->PreSetCallback = callback_function;
  }
  /* The PostSetCallback is used for both, the after set action and the storage
   * of non volatile data. Therefore check for both flags set. */
  if (0 != ((kPostSetFunc | kNvDataFunc) & callbacks_to_install)) {
    cip_class->PostSetCallback = callback_function;
  }
}

CipAttributeStruct *GetCipAttribute(const CipInstance *const instance,
                                    const EipUint16 attribute_number) {

  CipAttributeStruct *attribute = instance->attributes;       /* init pointer to array of attributes*/
  for (int i = 0; i < instance->cip_class->number_of_attributes; i++) {
    if (attribute_number == attribute->attribute_number) {
      return attribute;
    } else {
      ++attribute;
    }
  }

  OPENER_TRACE_WARN("attribute %d not defined\n", attribute_number);

  return NULL;
}

/* TODO this needs to check for buffer overflow*/
EipStatus GetAttributeSingle(CipInstance *RESTRICT const instance,
                             CipMessageRouterRequest *const message_router_request,
                             CipMessageRouterResponse *const message_router_response,
                             const struct sockaddr *originator_address,
                             const int encapsulation_session) {
  (void) originator_address;
  (void) encapsulation_session;

  /* Mask for filtering get-ability */

  CipAttributeStruct *attribute = GetCipAttribute(instance,
                                                  message_router_request->request_path.attribute_number);
  EipByte *message = message_router_response->data;

  message_router_response->data_length = 0;
  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->general_status = kCipErrorAttributeNotSupported;
  message_router_response->size_of_additional_status = 0;

  EipUint16 attribute_number =
    message_router_request->request_path.attribute_number;

  if ( (NULL != attribute) && (NULL != attribute->data) ) {
    uint8_t get_bit_mask = 0;
    if (kGetAttributeAll == message_router_request->service) {
      get_bit_mask =
        (instance->cip_class->get_all_bit_mask[CalculateIndex(
                                                 attribute_number)]);
      message_router_response->general_status = kCipErrorSuccess;
    } else {
      get_bit_mask =
        (instance->cip_class->get_single_bit_mask[CalculateIndex(
                                                    attribute_number)]);
    }
    if (0 != (get_bit_mask & (1 << (attribute_number % 8) ) ) ) {
      OPENER_TRACE_INFO("getAttribute %d\n",
                        message_router_request->request_path.attribute_number);                 /* create a reply message containing the data*/

      /* Call the PreGetCallback if enabled for this attribute and the class provides one. */
      if (attribute->attribute_flags & kPreGetFunc && NULL != instance->cip_class->PreGetCallback) {
        instance->cip_class->PreGetCallback(instance, attribute, message_router_request->service);
      }

      OPENER_ASSERT(NULL != attribute)
      message_router_response->data_length = EncodeData(attribute->type,
                                                        attribute->data,
                                                        &message);
      message_router_response->general_status = kCipErrorSuccess;

      /* Call the PostGetCallback if enabled for this attribute and the class provides one. */
      if (attribute->attribute_flags & kPostGetFunc && NULL != instance->cip_class->PostGetCallback) {
        instance->cip_class->PostGetCallback(instance, attribute, message_router_request->service);
      }
    }
  }

  return kEipStatusOkSend;
}

int EncodeData(const EipUint8 cip_type,
               const void *const cip_data,
               EipUint8 **cip_message) {
  int counter = 0;

  switch (cip_type)
  /* check the data type of attribute */
  {
    case (kCipBool):
    case (kCipSint):
    case (kCipUsint):
    case (kCipByte):
      counter = AddSintToMessage(*(EipUint8 *) (cip_data), cip_message);
      break;

    case (kCipInt):
    case (kCipUint):
    case (kCipWord):
      counter = AddIntToMessage(*(EipUint16 *) (cip_data), cip_message);
      break;

    case (kCipDint):
    case (kCipUdint):
    case (kCipDword):
    case (kCipReal):
      counter = AddDintToMessage(*(EipUint32 *) (cip_data), cip_message);
      break;

#ifdef OPENER_SUPPORT_64BIT_DATATYPES
    case (kCipLint):
    case (kCipUlint):
    case (kCipLword):
    case (kCipLreal):
      counter = AddLintToMessage(*(EipUint64 *) (cip_data), cip_message);
      break;
#endif

    case (kCipStime):
    case (kCipDate):
    case (kCipTimeOfDay):
    case (kCipDateAndTime):
      break;
    case (kCipString): {
      CipString *const string = (CipString *) cip_data;

      AddIntToMessage(*(EipUint16 *) &(string->length), cip_message);
      memcpy(*cip_message, string->string, string->length);
      *cip_message += string->length;

      counter = string->length + 2;           /* we have a two byte length field */
      if (counter & 0x01) {
        /* we have an odd byte count */
        **cip_message = 0;
        ++(*cip_message);
        counter++;
      }
      break;
    }
    case (kCipString2):
    case (kCipFtime):
    case (kCipLtime):
    case (kCipItime):
    case (kCipStringN):
      break;

    case (kCipShortString): {
      CipShortString *const short_string = (CipShortString *) cip_data;

      **cip_message = short_string->length;
      ++(*cip_message);

      memcpy(*cip_message, short_string->string, short_string->length);
      *cip_message += short_string->length;

      counter = short_string->length + 1;
      break;
    }

    case (kCipTime):
      break;

    case (kCipEpath):
      counter = EncodeEPath( (CipEpath *) cip_data, cip_message );
      break;

    case (kCipEngUnit):
      break;

    case (kCipUsintUsint): {
      CipRevision *revision = (CipRevision *) cip_data;

      **cip_message = revision->major_revision;
      ++(*cip_message);
      **cip_message = revision->minor_revision;
      ++(*cip_message);
      counter = 2;
      break;
    }

    case (kCipUdintUdintUdintUdintUdintString): {
      /* TCP/IP attribute 5 */
      CipTcpIpInterfaceConfiguration *
        tcp_ip_network_interface_configuration =
        (CipTcpIpInterfaceConfiguration *) cip_data;
      counter += AddDintToMessage(
        ntohl(tcp_ip_network_interface_configuration->ip_address),
        cip_message);
      counter += AddDintToMessage(
        ntohl(tcp_ip_network_interface_configuration->network_mask),
        cip_message);
      counter += AddDintToMessage(
        ntohl(tcp_ip_network_interface_configuration->gateway),
        cip_message);
      counter += AddDintToMessage(
        ntohl(tcp_ip_network_interface_configuration->name_server),
        cip_message);
      counter += AddDintToMessage(
        ntohl(tcp_ip_network_interface_configuration->name_server_2),
        cip_message);
      counter += EncodeData(kCipString,
                            &(tcp_ip_network_interface_configuration->
                              domain_name),
                            cip_message);
      break;
    }

    case (kCip6Usint): {
      EipUint8 *p = (EipUint8 *) cip_data;
      memcpy(*cip_message, p, 6);
      counter = 6;
      break;
    }

    case (kCipMemberList):
      break;

    case (kCipByteArray): {
      OPENER_TRACE_INFO(" -> get attribute byte array\r\n");
      CipByteArray *cip_byte_array = (CipByteArray *) cip_data;
      memcpy(*cip_message, cip_byte_array->data, cip_byte_array->length);
      *cip_message += cip_byte_array->length;
      counter = cip_byte_array->length;
    }
    break;

    case (kInternalUint6):     /* TODO for port class attribute 9, hopefully we can find a better way to do this*/
    {
      EipUint16 *internal_unit16_6 = (EipUint16 *) cip_data;

      AddIntToMessage(internal_unit16_6[0], cip_message);
      AddIntToMessage(internal_unit16_6[1], cip_message);
      AddIntToMessage(internal_unit16_6[2], cip_message);
      AddIntToMessage(internal_unit16_6[3], cip_message);
      AddIntToMessage(internal_unit16_6[4], cip_message);
      AddIntToMessage(internal_unit16_6[5], cip_message);
      counter = 12;
      break;
    }
    default:
      break;

  }

  return counter;
}

int DecodeData(const EipUint8 cip_data_type,
               void *const cip_data,
               const EipUint8 **const cip_message) {
  int number_of_decoded_bytes = -1;

  switch (cip_data_type)
  /* check the data type of attribute */
  {
    case (kCipBool):
    case (kCipSint):
    case (kCipUsint):
    case (kCipByte):
      *(EipUint8 *) (cip_data) = **cip_message;
      ++(*cip_message);
      number_of_decoded_bytes = 1;
      break;

    case (kCipInt):
    case (kCipUint):
    case (kCipWord):
      (*(EipUint16 *) (cip_data) ) = GetIntFromMessage(cip_message);
      number_of_decoded_bytes = 2;
      break;

    case (kCipDint):
    case (kCipUdint):
    case (kCipDword):
    case (kCipReal):
      (*(EipUint32 *) (cip_data) ) = GetDintFromMessage(cip_message);
      number_of_decoded_bytes = 4;
      break;

#ifdef OPENER_SUPPORT_64BIT_DATATYPES
    case (kCipLint):
    case (kCipUlint):
    case (kCipLword): {
      (*(EipUint64 *) (cip_data) ) = GetLintFromMessage(cip_message);
      number_of_decoded_bytes = 8;
    }
    break;
#endif

    case (kCipString): {
      CipString *string = (CipString *) cip_data;
      string->length = GetIntFromMessage(cip_message);
      memcpy(string->string, *cip_message, string->length);
      *cip_message += string->length;

      number_of_decoded_bytes = string->length + 2;           /* we have a two byte length field */
      if (number_of_decoded_bytes & 0x01) {
        /* we have an odd byte count */
        ++(*cip_message);
        number_of_decoded_bytes++;
      }
    }
    break;
    case (kCipShortString): {
      CipShortString *short_string = (CipShortString *) cip_data;

      short_string->length = **cip_message;
      ++(*cip_message);

      memcpy(short_string->string, *cip_message, short_string->length);
      *cip_message += short_string->length;

      number_of_decoded_bytes = short_string->length + 1;
      break;
    }

    default:
      break;
  }

  return number_of_decoded_bytes;
}

CipServiceStruct *GetCipService(const CipInstance *const instance,
                                CipUsint service_number) {
  CipServiceStruct *service = instance->cip_class->services;
  for (size_t i = 0; i < instance->cip_class->number_of_services; i++)       /* hunt for the GET_ATTRIBUTE_SINGLE service*/
  {
    if (service->service_number == service_number) {
      return service;   /* found the service */
    }
    service++;
  }
  return NULL; /* didn't find the service */
}

EipStatus GetAttributeAll(CipInstance *instance,
                          CipMessageRouterRequest *message_router_request,
                          CipMessageRouterResponse *message_router_response,
                          const struct sockaddr *originator_address,
                          const int encapsulation_session) {

  EipUint8 *reply = message_router_response->data;       /* pointer into the reply */
  CipAttributeStruct *attribute = instance->attributes;       /* pointer to list of attributes*/
  CipServiceStruct *service = GetCipService(instance, kGetAttributeSingle);       /* pointer to list of services*/

  if(NULL == service) {
    /* GetAttributeAll service not found */
    /* Return kEipStatusOk if cannot find GET_ATTRIBUTE_SINGLE service*/
    return kEipStatusOk;
  }

  if (0 == instance->cip_class->number_of_attributes) {
    message_router_response->data_length = 0;                         /*there are no attributes to be sent back*/
    message_router_response->reply_service =
      (0x80 | message_router_request->service);
    message_router_response->general_status = kCipErrorServiceNotSupported;
    message_router_response->size_of_additional_status = 0;
  } else {
    for (size_t j = 0; j < instance->cip_class->number_of_attributes; j++) {
      /* for each instance attribute of this class */
      int attribute_number = attribute->attribute_number;
      if ( (instance->cip_class->get_all_bit_mask[CalculateIndex(
                                                    attribute_number)]) &
           (1 << (attribute_number % 8) ) ) {
        /* only return attributes that are flagged as being part of GetAttributeALl */
        message_router_request->request_path.attribute_number =
          attribute_number;
        if (kEipStatusOkSend !=
            service->service_function(instance, message_router_request,
                                      message_router_response,
                                      originator_address,
                                      encapsulation_session) ) {
          message_router_response->data = reply;
          return kEipStatusError;
        }
        message_router_response->data += message_router_response->data_length;
      }
      attribute++;
    }
    message_router_response->data_length = message_router_response->data -
                                           reply;
    message_router_response->data = reply;
  }
  return kEipStatusOkSend;
}

int EncodeEPath(CipEpath *epath,
                EipUint8 **message) {
  unsigned int length = epath->path_size;
  AddIntToMessage(epath->path_size, message);

  if (epath->class_id < 256) {
    **message = 0x20;             /*8Bit Class Id */
    ++(*message);
    **message = (EipUint8) epath->class_id;
    ++(*message);
    length -= 1;
  } else {
    **message = 0x21;             /*16Bit Class Id */
    ++(*message);
    **message = 0;             /*pad byte */
    ++(*message);
    AddIntToMessage(epath->class_id, message);
    length -= 2;
  }

  if (0 < length) {
    if (epath->instance_number < 256) {
      **message = 0x24;                   /*8Bit Instance Id */
      ++(*message);
      **message = (EipUint8) epath->instance_number;
      ++(*message);
      length -= 1;
    } else {
      **message = 0x25;                   /*16Bit Instance Id */
      ++(*message);
      **message = 0;                   /*padd byte */
      ++(*message);
      AddIntToMessage(epath->instance_number, message);
      length -= 2;
    }

    if (0 < length) {
      if (epath->attribute_number < 256) {
        **message = 0x30;                         /*8Bit Attribute Id */
        ++(*message);
        **message = (EipUint8) epath->attribute_number;
        ++(*message);
        length -= 1;
      } else {
        **message = 0x31;                         /*16Bit Attribute Id */
        ++(*message);
        **message = 0;                         /*pad byte */
        ++(*message);
        AddIntToMessage(epath->attribute_number, message);
        length -= 2;
      }
    }
  }

  return 2 + epath->path_size * 2;       /* path size is in 16 bit chunks according to the specification */
}

int DecodePaddedEPath(CipEpath *epath,
                      const EipUint8 **message) {
  unsigned int number_of_decoded_elements = 0;
  const EipUint8 *message_runner = *message;

  epath->path_size = *message_runner;
  message_runner++;
  /* copy path to structure, in version 0.1 only 8 bit for Class,Instance and Attribute, need to be replaced with function */
  epath->class_id = 0;
  epath->instance_number = 0;
  epath->attribute_number = 0;

  for (number_of_decoded_elements = 0;
       number_of_decoded_elements < epath->path_size;
       number_of_decoded_elements++) {
    if (kSegmentTypeReserved
        == ( (*message_runner) & kSegmentTypeReserved ) ) {
      /* If invalid/reserved segment type, segment type greater than 0xE0 */
      return kEipStatusError;
    }

    switch (*message_runner) {
      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_CLASS_ID +
        LOGICAL_SEGMENT_FORMAT_EIGHT_BIT:
        epath->class_id = *(EipUint8 *) (message_runner + 1);
        message_runner += 2;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_CLASS_ID +
        LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT:
        message_runner += 2;
        epath->class_id = GetIntFromMessage(&(message_runner) );
        number_of_decoded_elements++;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_INSTANCE_ID +
        LOGICAL_SEGMENT_FORMAT_EIGHT_BIT:
        epath->instance_number = *(EipUint8 *) (message_runner + 1);
        message_runner += 2;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_INSTANCE_ID +
        LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT:
        message_runner += 2;
        epath->instance_number = GetIntFromMessage(&(message_runner) );
        number_of_decoded_elements++;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID +
        LOGICAL_SEGMENT_FORMAT_EIGHT_BIT:
        epath->attribute_number = *(EipUint8 *) (message_runner + 1);
        message_runner += 2;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_ATTRIBUTE_ID +
        LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT:
        message_runner += 2;
        epath->attribute_number = GetIntFromMessage(&(message_runner) );
        number_of_decoded_elements++;
        break;

      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_MEMBER_ID
        + LOGICAL_SEGMENT_FORMAT_EIGHT_BIT:
        message_runner += 2;
        break;
      case SEGMENT_TYPE_LOGICAL_SEGMENT + LOGICAL_SEGMENT_TYPE_MEMBER_ID +
        LOGICAL_SEGMENT_FORMAT_SIXTEEN_BIT:
        message_runner += 2;
        number_of_decoded_elements++;
        break;

      default:
        OPENER_TRACE_ERR("wrong path requested\n");
        return kEipStatusError;
    }
  }

  *message = message_runner;
  return number_of_decoded_elements * 2 + 1;       /* number_of_decoded_elements times 2 as every encoding uses 2 bytes */
}

void AllocateAttributeMasks(CipClass *target_class) {
  unsigned size = 1 + CalculateIndex(target_class->highest_attribute_number);
  OPENER_TRACE_INFO(
    ">>> Allocate memory for %s %u bytes times 3 for masks\n",
    target_class->class_name, size);
  target_class->get_single_bit_mask = CipCalloc(size, sizeof(uint8_t) );
  target_class->set_bit_mask = CipCalloc(size, sizeof(uint8_t) );
  target_class->get_all_bit_mask = CipCalloc(size, sizeof(uint8_t) );
}

size_t CalculateIndex(EipUint16 attribute_number) {
  size_t index = attribute_number / 8;
  return index;
}

size_t GetSizeOfAttribute(const CipAttributeStruct *const attribute_struct) {
  size_t data_type_size = 0;
  switch (attribute_struct->type) {
    case (kCipBool):
      data_type_size = sizeof(CipBool);
      break;
    case (kCipSint):
      data_type_size = sizeof(CipSint);
      break;
    case (kCipInt):
      data_type_size = sizeof(CipInt);
      break;
    case (kCipDint):
      data_type_size = sizeof(CipDint);
      break;
    case (kCipUsint):
      data_type_size = sizeof(CipUsint);
      break;
    case (kCipUint):
      data_type_size = sizeof(CipUint);
      break;
    case (kCipUdint):
      data_type_size = sizeof(CipUdint);
      break;
    case (kCipReal):
      data_type_size = sizeof(CipReal);
      break;
#ifdef OPENER_SUPPORT_64BIT_DATATYPES
    case (kCipLreal):
      data_type_size = sizeof(CipLreal);
      break;
    case (kCipUlint):
      data_type_size = sizeof(CipUlint);
      break;
    case (kCipLint):
      data_type_size = sizeof(CipLint);
      break;
    case (kCipLword):
      data_type_size = sizeof(CipLword);
      break;
    case (kCipLtime):
      data_type_size = sizeof(CipLint);
      break;
#endif /* OPENER_SUPPORT_64BIT_DATATYPES */

    case (kCipStime):
      data_type_size = sizeof(CipDint);
      break;
    case (kCipDate):
      data_type_size = sizeof(CipUint);
      break;
    case (kCipTimeOfDay):
      data_type_size = sizeof(CipUdint);
      break;
    case (kCipDateAndTime):
      data_type_size = sizeof(CipUdint) + sizeof(CipUint);
      break;
    case (kCipString): {
      CipString *data = (CipString *) attribute_struct->data;
      data_type_size = sizeof(CipUint) + (data->length) * sizeof(CipOctet);
    }
    break;
    case (kCipByte):
      data_type_size = sizeof(CipByte);
      break;
    case (kCipWord):
      data_type_size = sizeof(CipWord);
      break;
    case (kCipDword):
      data_type_size = sizeof(CipDword);
      break;
    case (kCipString2): {
      CipString *data = (CipString *) attribute_struct->data;
      data_type_size = sizeof(CipUint) + 2 * (data->length) * sizeof(CipOctet);
    }
    break;
    case (kCipFtime):
      data_type_size = sizeof(CipDint);
      break;
    case (kCipItime):
      data_type_size = sizeof(CipInt);
      break;
    case (kCipStringN): {
      CipStringN *data = (CipStringN *) attribute_struct->data;
      data_type_size = sizeof(CipUint) + sizeof(CipUint)
                       + (size_t) (data->length) * (size_t) (data->size);
    }
    break;
    case (kCipShortString): {
      CipShortString *data = (CipShortString *) attribute_struct->data;
      data_type_size = sizeof(CipUsint) + (data->length) * sizeof(CipOctet);
    }
    break;
    case (kCipTime):
      data_type_size = sizeof(CipDint);
      break;
    case (kCipEpath): {
      CipEpath *data = (CipEpath *) attribute_struct->data;
      data_type_size = 2 * (data->path_size);
    }
    break;
    case (kCipEngUnit):
      data_type_size = sizeof(CipUint);
      break;
    case (kCipUsintUsint):
      data_type_size = 2 * sizeof(CipUsint);
      break;
    case (kCipUdintUdintUdintUdintUdintString): {
      CipTcpIpInterfaceConfiguration *data =
        (CipTcpIpInterfaceConfiguration *) attribute_struct->data;
      data_type_size = 5 * sizeof(CipUdint) + sizeof(CipUint)
                       + (data->domain_name.length) * sizeof(EipByte);
    }
    break;
    case (kCip6Usint):
      data_type_size = 6 * sizeof(CipUsint);
      break;
    case (kCipMemberList):
      data_type_size = 0;
      break;
    case (kCipByteArray): {
      CipByteArray *data = (CipByteArray *) attribute_struct->data;
      data_type_size = sizeof(CipUint) + (data->length) * sizeof(CipOctet);
    }
    break;
    case (kInternalUint6):
      data_type_size = 6 * sizeof(CipUint);
      break;
    default:
      data_type_size = 0;
      break;
  }
  return data_type_size;
}
