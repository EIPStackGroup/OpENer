/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <string.h>

#include "opener_user_conf.h"
#include "opener_api.h"
#include "cipcommon.h"
#include "cipidentity.h"
#include "ciptcpipinterface.h"
#include "cipethernetlink.h"
#include "cipconnectionmanager.h"
#include "endianconv.h"
#include "encap.h"
#include "ciperror.h"
#include "cipassembly.h"
#include "cipmessagerouter.h"
#include "cpf.h"
#include "trace.h"
#include "appcontype.h"

/* global public variables */EIP_UINT8
    g_acMessageDataReplyBuffer[OPENER_MESSAGE_DATA_REPLY_BUFFER];

/* private functions*/
int
encodeEPath(S_CIP_EPATH *pa_pstEPath, EIP_UINT8 **pa_pnMsg);

void
CIP_Init(EIP_UINT16 pa_nUniqueConnID)
{
  EIP_STATUS nRetVal;
  encapInit();
  /* The message router is the first CIP object that has to be initialized first!!! */
  nRetVal = CIP_MessageRouter_Init();
  OPENER_ASSERT(EIP_OK == nRetVal);
  nRetVal = CIP_Identity_Init();
  OPENER_ASSERT(EIP_OK == nRetVal);
  nRetVal = CIP_TCPIP_Interface_Init();
  OPENER_ASSERT(EIP_OK == nRetVal);
  nRetVal = CIP_Ethernet_Link_Init();
  OPENER_ASSERT(EIP_OK == nRetVal);
  nRetVal = Connection_Manager_Init(pa_nUniqueConnID);
  OPENER_ASSERT(EIP_OK == nRetVal);
  nRetVal = CIP_Assembly_Init();
  OPENER_ASSERT(EIP_OK == nRetVal);
  /* the application has to be initialized at last */
  nRetVal = IApp_Init();
  OPENER_ASSERT(EIP_OK == nRetVal);
}

void
shutdownCIP(void)
{
  /* First close all connections */
  closeAllConnections();
  /* Than free the sockets of currently active encapsulation sessions */
  encapShutDown();
  /*clean the data needed for the assembly object's attribute 3*/
  shutdownAssemblies();

  shutdownTCPIP_Interface();

  /*no clear all the instances and classes */
  deleteAllClasses();
}

EIP_STATUS
notifyClass(S_CIP_Class * pt2Class, S_CIP_MR_Request * pa_MRRequest,
    S_CIP_MR_Response * pa_MRResponse)
{
  int i;
  S_CIP_Instance *pstInstance;
  S_CIP_service_struct *p;
  unsigned instNr; /* my instance number */

  /* find the instance: if instNr==0, the class is addressed, else find the instance */
  instNr = pa_MRRequest->RequestPath.InstanceNr; /* get the instance number */
  pstInstance = getCIPInstance(pt2Class, instNr); /* look up the instance (note that if inst==0 this will be the class itself) */
  if (pstInstance) /* if instance is found */
    {
      OPENER_TRACE_INFO("notify: found instance %d%s\n", instNr,
          instNr == 0 ? " (class object)" : "");

      p = pstInstance->pstClass->pstServices; /* get pointer to array of services */
      if (p) /* if services are defined */
        {
          for (i = 0; i < pstInstance->pstClass->nNr_of_Services; i++) /* seach the services list */
            {
              if (pa_MRRequest->Service == p->CIP_ServiceNr) /* if match is found */
                {
                  pa_MRResponse->Data = &g_acMessageDataReplyBuffer[0]; /* set reply buffer, using a fixed buffer (about 100 bytes) */
                  /* call the service, and return what it returns */
                  OPENER_TRACE_INFO("notify: calling %s service\n", p->name);
                  OPENER_ASSERT(NULL != p->m_ptfuncService);
                  return p->m_ptfuncService(pstInstance, pa_MRRequest,
                      pa_MRResponse, &g_acMessageDataReplyBuffer[0]);
                }
              else
                p++;
            }
        }OPENER_TRACE_WARN("notify: service 0x%x not supported\n",
          pa_MRRequest->Service);
      pa_MRResponse->GeneralStatus = CIP_ERROR_SERVICE_NOT_SUPPORTED; /* if no services or service not found, return an error reply*/
    }
  else
    {
      OPENER_TRACE_WARN("notify: instance number %d unknown\n", instNr);
      /* if instance not found, return an error reply*/
      pa_MRResponse->GeneralStatus = CIP_ERROR_PATH_DESTINATION_UNKNOWN; /*according to the test tool this should be the correct error flag instead of CIP_ERROR_OBJECT_DOES_NOT_EXIST;*/
    }

  /* handle error replies*/
  pa_MRResponse->SizeofAdditionalStatus = 0; /* fill in the rest of the reply with not much of anything*/
  pa_MRResponse->DataLength = 0;
  pa_MRResponse->ReplyService = (0x80 | pa_MRRequest->Service); /* except the reply code is an echo of the command + the reply flag */

  return EIP_OK_SEND;
}

S_CIP_Instance *
addCIPInstances(S_CIP_Class * pa_pstCIPClass, int pa_nNr_of_Instances)
{
  S_CIP_Instance *first, *p, **pp;
  int i;
  int inst = 1; /* the first instance is number 1 */

  OPENER_TRACE_INFO("adding %d instances to class %s\n", pa_nNr_of_Instances,
      pa_pstCIPClass->acName);

  pp = &pa_pstCIPClass->pstInstances; /* get address of pointer to head of chain */
  while (*pp) /* as long as what pp points to is not zero */
    {
      pp = &(*pp)->pstNext; /*    follow the chain until pp points to pointer that contains a zero */
      inst++; /*    keep track of what the first new instance number will be */
    }

  first = p = (S_CIP_Instance *) IApp_CipCalloc(pa_nNr_of_Instances,
      sizeof(S_CIP_Instance)); /* allocate a block of memory for all created instances*/
  OPENER_ASSERT(NULL != p);
  /* fail if run out of memory */

  pa_pstCIPClass->nNr_of_Instances += pa_nNr_of_Instances; /* add the number of instances just created to the total recorded by the class */

  for (i = 0; i < pa_nNr_of_Instances; i++) /* initialize all the new instances */
    {
      *pp = p; /* link the previous pointer to this new node */

      p->nInstanceNr = inst; /* assign the next sequential instance number */
      p->pstClass = pa_pstCIPClass; /* point each instance to its class */

      if (pa_pstCIPClass->nNr_of_Attributes) /* if the class calls for instance attributes */
        { /* then allocate storage for the attribute array */
          p->pstAttributes
              = (S_CIP_attribute_struct*) IApp_CipCalloc(
                  pa_pstCIPClass->nNr_of_Attributes,
                  sizeof(S_CIP_attribute_struct));
        }

      pp = &p->pstNext; /* update pp to point to the next link of the current node */
      inst++; /* update to the number of the next node*/
      p++; /* point to the next node in the calloc'ed array*/
    }

  return first;
}

S_CIP_Instance *
addCIPInstance(S_CIP_Class * pa_pstCIPClass, EIP_UINT32 pa_nInstanceId)
{
  S_CIP_Instance *pstInstance = getCIPInstance(pa_pstCIPClass, pa_nInstanceId);

  if (0 == pstInstance)
    { /*we have no instance with given id*/
      pstInstance = addCIPInstances(pa_pstCIPClass, 1);
      pstInstance->nInstanceNr = pa_nInstanceId;
    }
  return pstInstance;
}

S_CIP_Class *
createCIPClass(EIP_UINT32 pa_nClassID, int pa_nNr_of_ClassAttributes,
    EIP_UINT32 pa_nClassGetAttrAllMask, int pa_nNr_of_ClassServices,
    int pa_nNr_of_InstanceAttributes, EIP_UINT32 pa_nInstGetAttrAllMask,
    int pa_nNr_of_InstanceServices, int pa_nNr_of_Instances, char *pa_acName,
    EIP_UINT16 pa_nRevision)
{
  S_CIP_Class *pt2Class; /* pointer to the class struct */
  S_CIP_Class *pt2MetaClass; /* pointer to the metaclass struct */

  OPENER_TRACE_INFO("creating class '%s' with id: 0x%lx\n", pa_acName, pa_nClassID);

  pt2Class = getCIPClass(pa_nClassID); /* check if an class with the ClassID already exists */
  OPENER_ASSERT(NULL == pt2Class);
  /* should never try to redefine a class*/

  /* a metaClass is a class that holds the class attributes and services
   CIP can talk to an instance, therefore an instance has a pointer to its class
   CIP can talk to a class, therefore a class struct is a subclass of the instance struct,
   and contains a pointer to a metaclass
   CIP never explicitly addresses a metaclass*/

  pt2Class = (S_CIP_Class*) IApp_CipCalloc(1, sizeof(S_CIP_Class)); /* create the class object*/
  pt2MetaClass = (S_CIP_Class*) IApp_CipCalloc(1, sizeof(S_CIP_Class)); /* create the metaclass object*/

  /* initialize the class-specific fields of the Class struct*/
  pt2Class->nClassID = pa_nClassID; /* the class remembers the class ID */
  pt2Class->nRevision = pa_nRevision; /* the class remembers the class ID */
  pt2Class->nNr_of_Instances = 0; /* the number of instances initially zero (more created below) */
  pt2Class->pstInstances = 0;
  pt2Class->nNr_of_Attributes = pa_nNr_of_InstanceAttributes; /* the class remembers the number of instances of that class */
  pt2Class->nGetAttrAllMask = pa_nInstGetAttrAllMask; /* indicate which attributes are included in instance getAttributeAll */
  pt2Class->nNr_of_Services = pa_nNr_of_InstanceServices + ((0
      == pa_nInstGetAttrAllMask) ? 1 : 2); /* the class manages the behavior of the instances */
  pt2Class->pstServices = 0;
  pt2Class->acName = pa_acName; /* initialize the class-specific fields of the metaClass struct */
  pt2MetaClass->nClassID = 0xffffffff; /* set metaclass ID (this should never be referenced) */
  pt2MetaClass->nNr_of_Instances = 1; /* the class object is the only instance of the metaclass */
  pt2MetaClass->pstInstances = (S_CIP_Instance *) pt2Class;
  pt2MetaClass->nNr_of_Attributes = pa_nNr_of_ClassAttributes + 5; /* the metaclass remembers how many class attributes exist*/
  pt2MetaClass->nGetAttrAllMask = pa_nClassGetAttrAllMask; /* indicate which attributes are included in class getAttributeAll*/
  pt2MetaClass->nNr_of_Services = pa_nNr_of_ClassServices + ((0
      == pa_nClassGetAttrAllMask) ? 1 : 2); /* the metaclass manages the behavior of the class itself */
  pt2Class->pstServices = 0;
  pt2MetaClass->acName = (char *) IApp_CipCalloc(1, strlen(pa_acName) + 6); /* fabricate the name "meta<classname>"*/
  strcpy(pt2MetaClass->acName, "meta-");
  strcat(pt2MetaClass->acName, pa_acName);

  /* initialize the instance-specific fields of the Class struct*/
  pt2Class->m_stSuper.nInstanceNr = 0; /* the class object is instance zero of the class it describes (weird, but that's the spec)*/
  pt2Class->m_stSuper.pstAttributes = 0; /* this will later point to the class attibutes*/
  pt2Class->m_stSuper.pstClass = pt2MetaClass; /* the class's class is the metaclass (like SmallTalk)*/
  pt2Class->m_stSuper.pstNext = 0; /* the next link will always be zero, sinc there is only one instance of any particular class object */

  pt2MetaClass->m_stSuper.nInstanceNr = 0xffffffff; /*the metaclass object does not really have a valid instance number*/
  pt2MetaClass->m_stSuper.pstAttributes = 0; /* the metaclass has no attributes*/
  pt2MetaClass->m_stSuper.pstClass = 0; /* the metaclass has no class*/
  pt2MetaClass->m_stSuper.pstNext = 0; /* the next link will always be zero, since there is only one instance of any particular metaclass object*/

  /* further initialization of the class object*/

  pt2Class->m_stSuper.pstAttributes = (S_CIP_attribute_struct *) IApp_CipCalloc(
      pa_nNr_of_ClassAttributes + 5, sizeof(S_CIP_attribute_struct));
  /* TODO -- check that we didn't run out of memory?*/

  pt2MetaClass->pstServices = (S_CIP_service_struct *) IApp_CipCalloc(
      pt2MetaClass->nNr_of_Services, sizeof(S_CIP_service_struct));

  pt2Class->pstServices = (S_CIP_service_struct *) IApp_CipCalloc(
      pt2Class->nNr_of_Services, sizeof(S_CIP_service_struct));

  if (pa_nNr_of_Instances > 0)
    {
      addCIPInstances(pt2Class, pa_nNr_of_Instances); /*TODO handle return value and clean up if necessary*/
    }

  if ((registerClass(pt2Class)) == EIP_ERROR)
    { /* no memory to register class in Message Router */
      return 0; /*TODO handle return value and clean up if necessary*/
    }

  /* create the standard class attributes*/
  insertAttribute((S_CIP_Instance *) pt2Class, 1, CIP_UINT,
      (void *) &pt2Class->nRevision); /* revision */
  insertAttribute((S_CIP_Instance *) pt2Class, 2, CIP_UINT,
      (void *) &pt2Class->nNr_of_Instances); /*  largest instance number */
  insertAttribute((S_CIP_Instance *) pt2Class, 3, CIP_UINT,
      (void *) &pt2Class->nNr_of_Instances); /* number of instances currently existing*/
  insertAttribute((S_CIP_Instance *) pt2Class, 6, CIP_UINT,
      (void *) &pt2MetaClass->nMaxAttribute); /* max class attribute number*/
  insertAttribute((S_CIP_Instance *) pt2Class, 7, CIP_UINT,
      (void *) &pt2Class->nMaxAttribute); /* max instance attribute number*/

  /* create the standard class services*/
  if (0 != pa_nClassGetAttrAllMask)
    { /*only if the mask has values add the get_attribute_all service */
      insertService(pt2MetaClass, CIP_GET_ATTRIBUTE_ALL, &getAttributeAll,
          "GetAttributeAll"); /* bind instance services to the metaclass*/
    }
  insertService(pt2MetaClass, CIP_GET_ATTRIBUTE_SINGLE, &getAttributeSingle,
      "GetAttributeSingle");

  /* create the standard instance services*/
  if (0 != pa_nInstGetAttrAllMask)
    { /*only if the mask has values add the get_attribute_all service */
      insertService(pt2Class, CIP_GET_ATTRIBUTE_ALL, &getAttributeAll,
          "GetAttributeAll"); /* bind instance services to the class*/
    }
  insertService(pt2Class, CIP_GET_ATTRIBUTE_SINGLE, &getAttributeSingle,
      "GetAttributeSingle");

  return pt2Class;
}

void
insertAttribute(S_CIP_Instance * pa_pInstance, EIP_UINT16 pa_nAttributeNr,
    EIP_UINT8 pa_nCIP_Type, void *pa_pt2data)
{
  int i;
  S_CIP_attribute_struct *p;

  p = pa_pInstance->pstAttributes;
  OPENER_ASSERT(NULL != p);
  /* adding a attribute to a class that was not declared to have any attributes is not allowed */
  for (i = 0; i < pa_pInstance->pstClass->nNr_of_Attributes; i++)
    {
      if (p->pt2data == 0)
        { /* found non set attribute */
          p->CIP_AttributNr = pa_nAttributeNr;
          p->CIP_Type = pa_nCIP_Type;
          p->pt2data = pa_pt2data;

          if (pa_nAttributeNr > pa_pInstance->pstClass->nMaxAttribute) /* remember the max attribute number that was defined*/
            {
              pa_pInstance->pstClass->nMaxAttribute = pa_nAttributeNr;
            }
          return;
        }
      p++;
    }

  OPENER_TRACE_ERR("Tried to insert to many attributes into class: %lu, instance %lu\n", pa_pInstance->pstClass->m_stSuper.nInstanceNr, pa_pInstance->nInstanceNr );
  OPENER_ASSERT(0);
  /* trying to insert too many attributes*/
}

void
insertService(S_CIP_Class * pa_pClass, EIP_UINT8 pa_nServiceNr,
    TCIPServiceFunc pa_ptfuncService, char *name)
{
  int i;
  S_CIP_service_struct *p;

  p = pa_pClass->pstServices; /* get a pointer to the service array*/
  OPENER_ASSERT(p!=0);
  /* adding a service to a class that was not declared to have services is not allowed*/
  for (i = 0; i < pa_pClass->nNr_of_Services; i++) /* Iterate over all service slots attached to the class */
    {
      if (p->CIP_ServiceNr == pa_nServiceNr || p->m_ptfuncService == 0) /* found undefined service slot*/
        {
          p->CIP_ServiceNr = pa_nServiceNr; /* fill in service number*/
          p->m_ptfuncService = pa_ptfuncService; /* fill in function address*/
          p->name = name;
          return;
        }
      p++;
    }
  OPENER_ASSERT(0);
  /* adding more services than were declared is a no-no*/
}

S_CIP_attribute_struct *
getAttribute(S_CIP_Instance * pa_pInstance, EIP_UINT16 pa_nAttributeNr)
{
  int i;
  S_CIP_attribute_struct *p = pa_pInstance->pstAttributes; /* init pointer to array of attributes*/
  for (i = 0; i < pa_pInstance->pstClass->nNr_of_Attributes; i++)
    {
      if (pa_nAttributeNr == p->CIP_AttributNr)
        return p;
      else
        p++;
    }

  OPENER_TRACE_WARN("attribute %d not defined\n", pa_nAttributeNr);

  return 0;
}

/* TODO this needs to check for buffer overflow*/
EIP_STATUS
getAttributeSingle(S_CIP_Instance *pa_pstInstance,
    S_CIP_MR_Request *pa_pstMRRequest, S_CIP_MR_Response *pa_pstMRResponse,
    EIP_UINT8 *pa_acMsg)
{
  S_CIP_attribute_struct *p = getAttribute(pa_pstInstance,
      pa_pstMRRequest->RequestPath.AttributNr);

  pa_pstMRResponse->DataLength = 0;
  pa_pstMRResponse->ReplyService = (0x80 | pa_pstMRRequest->Service);
  pa_pstMRResponse->GeneralStatus = CIP_ERROR_ATTRIBUTE_NOT_SUPPORTED;
  pa_pstMRResponse->SizeofAdditionalStatus = 0;

  if ((p != 0) && (p->pt2data != 0))
    {
      OPENER_TRACE_INFO("getAttribute %d\n",
          pa_pstMRRequest->RequestPath.AttributNr); /* create a reply message containing the data*/

      /*TODO think if it is better to put this code in an own
       * getAssemblyAttributeSingle functions which will call get attribute
       * single.
       */
      if (p->CIP_Type == CIP_BYTE_ARRAY && pa_pstInstance->pstClass->nClassID
          == CIP_ASSEMBLY_CLASS_CODE)
        {
          /* we are getting a byte array of a assembly object, kick out to the app callback */
          OPENER_TRACE_INFO(" -> getAttributeSingle CIP_BYTE_ARRAY\r\n");
          IApp_BeforeAssemblyDataSend(pa_pstInstance);
        }

      pa_pstMRResponse->DataLength = encodeData(p->CIP_Type, p->pt2data,
          &pa_acMsg);
      pa_pstMRResponse->GeneralStatus = CIP_ERROR_SUCCESS;
    }

  return EIP_OK_SEND;
}

int
encodeData(EIP_UINT8 pa_nCIP_Type, void *pa_pt2data, EIP_UINT8 **pa_pnMsg)
{
  int counter = 0;
  S_CIP_Byte_Array *p;

  switch (pa_nCIP_Type)
    /* check the datatype of attribute */
    {
  case (CIP_BOOL):
  case (CIP_SINT):
  case (CIP_USINT):
  case (CIP_BYTE):
    **pa_pnMsg = *(EIP_UINT8 *) (pa_pt2data);
    ++(*pa_pnMsg);
    counter = 1;
    break;

  case (CIP_INT):
  case (CIP_UINT):
  case (CIP_WORD):
    htols(*(EIP_UINT16 *) (pa_pt2data), pa_pnMsg);
    counter = 2;
    break;

  case (CIP_DINT):
  case (CIP_UDINT):
  case (CIP_DWORD):
    htoll(*(EIP_UINT32 *) (pa_pt2data), pa_pnMsg);
    counter = 4;
    break;

#ifdef OPENER_SUPPORT_64BIT_DATATYPES
  case (CIP_LINT):
  case (CIP_ULINT):
  case (CIP_LWORD):
    htol64(*(EIP_UINT64 *) (pa_pt2data), pa_pnMsg);
    counter = 8;
    break;
#endif

  case (CIP_REAL):
  case (CIP_LREAL):
  case (CIP_STIME):
  case (CIP_DATE):
  case (CIP_TIME_OF_DAY):
  case (CIP_DATE_AND_TIME):
    break;
  case (CIP_STRING):
    {
      S_CIP_String *s = (S_CIP_String *) pa_pt2data;

      htols(*(EIP_UINT16 *) &(s->Length), pa_pnMsg);
      memcpy(*pa_pnMsg, s->String, s->Length);
      *pa_pnMsg += s->Length;

      counter = s->Length + 2; /* we have a two byte length field */
      if (counter & 0x01)
        {
          /* we have an odd byte count */
          **pa_pnMsg = 0;
          ++(*pa_pnMsg);
          counter++;
        }
      break;
    }
  case (CIP_STRING2):
  case (CIP_FTIME):
  case (CIP_LTIME):
  case (CIP_ITIME):
  case (CIP_STRINGN):
    break;

  case (CIP_SHORT_STRING):
    {
      S_CIP_Short_String *ss = (S_CIP_Short_String *) pa_pt2data;

      **pa_pnMsg = ss->Length;
      ++(*pa_pnMsg);

      memcpy(*pa_pnMsg, ss->String, ss->Length);
      *pa_pnMsg += ss->Length;

      counter = ss->Length + 1;
      break;
    }

  case (CIP_TIME):
    break;

  case (CIP_EPATH):
    counter = encodeEPath((S_CIP_EPATH *) pa_pt2data, pa_pnMsg);
    break;

  case (CIP_ENGUNIT):
    break;

  case (CIP_USINT_USINT):
    {
      S_CIP_Revision *rv = (S_CIP_Revision *) pa_pt2data;

      **pa_pnMsg = rv->MajorRevision;
      ++(*pa_pnMsg);
      **pa_pnMsg = rv->MinorRevision;
      ++(*pa_pnMsg);
      counter = 2;
      break;
    }

  case (CIP_UDINT_UDINT_UDINT_UDINT_UDINT_STRING):
    {
      /* TCP/IP attribute 5 */
      S_CIP_TCPIPNetworkInterfaceConfiguration *p =
          (S_CIP_TCPIPNetworkInterfaceConfiguration *) pa_pt2data;
      htoll(ntohl(p->IPAddress), pa_pnMsg);
      htoll(ntohl(p->NetworkMask), pa_pnMsg);
      htoll(ntohl(p->Gateway), pa_pnMsg);
      htoll(ntohl(p->NameServer), pa_pnMsg);
      htoll(ntohl(p->NameServer2), pa_pnMsg);
      counter = 20;
      counter += encodeData(CIP_STRING, &(p->DomainName), pa_pnMsg);
      break;
    }

  case (CIP_6USINT):
    {
      EIP_UINT8 *p = (EIP_UINT8 *) pa_pt2data;
      memcpy(*pa_pnMsg, p, 6);
      counter = 6;
      break;
    }

  case (CIP_MEMBER_LIST):
    break;

  case (CIP_BYTE_ARRAY):
    {
      OPENER_TRACE_INFO(" -> get attribute byte array\r\n");
      p = (S_CIP_Byte_Array *) pa_pt2data;
      memcpy(*pa_pnMsg, p->Data, p->len);
      *pa_pnMsg += p->len;
      counter = p->len;
    }
    break;

  case (INTERNAL_UINT16_6): /* TODO for port class attribute 9, hopefully we can find a better way to do this*/
    {
      EIP_UINT16 *p = (EIP_UINT16 *) pa_pt2data;

      htols(p[0], pa_pnMsg);
      htols(p[1], pa_pnMsg);
      htols(p[2], pa_pnMsg);
      htols(p[3], pa_pnMsg);
      htols(p[4], pa_pnMsg);
      htols(p[5], pa_pnMsg);
      counter = 12;
      break;
    }
  default:
    break;

    }

  return counter;
}

int
decodeData(EIP_UINT8 pa_nCIP_Type, void *pa_pt2data, EIP_UINT8 **pa_pnMsg)
{
  int nRetVal = -1;

  switch (pa_nCIP_Type)
    /* check the datatype of attribute */
    {
  case (CIP_BOOL):
  case (CIP_SINT):
  case (CIP_USINT):
  case (CIP_BYTE):
    *(EIP_UINT8 *) (pa_pt2data) = **pa_pnMsg;
    ++(*pa_pnMsg);
    nRetVal = 1;
    break;

  case (CIP_INT):
  case (CIP_UINT):
  case (CIP_WORD):
    (*(EIP_UINT16 *) (pa_pt2data)) = ltohs(pa_pnMsg);
    nRetVal = 2;
    break;

  case (CIP_DINT):
  case (CIP_UDINT):
  case (CIP_DWORD):
    (*(EIP_UINT32 *) (pa_pt2data)) = ltohl(pa_pnMsg);
    nRetVal = 4;
    break;

#ifdef OPENER_SUPPORT_64BIT_DATATYPES
  case (CIP_LINT):
  case (CIP_ULINT):
  case (CIP_LWORD):
    {
      (*(EIP_UINT32 *) (pa_pt2data)) = ltoh64(pa_pnMsg);
      nRetVal = 8;
    }
    break;
#endif

  case (CIP_STRING):
    {
      S_CIP_String *s = (S_CIP_String *) pa_pt2data;
      s->Length = ltohs(pa_pnMsg);
      memcpy(s->String, *pa_pnMsg, s->Length);
      *pa_pnMsg += s->Length;

      nRetVal = s->Length + 2; /* we have a two byte length field */
      if (nRetVal & 0x01)
        {
          /* we have an odd byte count */
          ++(*pa_pnMsg);
          nRetVal++;
        }
    }
    break;
  case (CIP_SHORT_STRING):
    {
      S_CIP_Short_String *ss = (S_CIP_Short_String *) pa_pt2data;

      ss->Length = **pa_pnMsg;
      ++(*pa_pnMsg);

      memcpy(ss->String, *pa_pnMsg, ss->Length);
      *pa_pnMsg += ss->Length;

      nRetVal = ss->Length + 1;
      break;
    }

  default:
    break;
    }

  return nRetVal;
}

EIP_STATUS
getAttributeAll(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_stMRRequest, S_CIP_MR_Response * pa_stMRResponse,
    EIP_UINT8 * pa_msg)
{
  int i, j;
  EIP_UINT8 *ptmp;
  S_CIP_attribute_struct *p_attr;
  S_CIP_service_struct *p_service;

  ptmp = pa_msg; /* pointer into the reply */
  p_attr = pa_pstInstance->pstAttributes; /* pointer to list of attributes*/
  p_service = pa_pstInstance->pstClass->pstServices; /* pointer to list of services*/

  if (pa_pstInstance->nInstanceNr == 2)
    {
      OPENER_TRACE_INFO("GetAttributeAll: instance number 2\n");
    }

  for (i = 0; i < pa_pstInstance->pstClass->nNr_of_Services; i++) /* hunt for the GET_ATTRIBUTE_SINGLE service*/
    {
      if (p_service->CIP_ServiceNr == CIP_GET_ATTRIBUTE_SINGLE) /* found the service */
        {
          if (0 == pa_pstInstance->pstClass->nNr_of_Attributes)
            {
              pa_stMRResponse->DataLength = 0; /*there are no attributes to be sent back*/
              pa_stMRResponse->ReplyService = (0x80 | pa_stMRRequest->Service);
              pa_stMRResponse->GeneralStatus = CIP_ERROR_SERVICE_NOT_SUPPORTED;
              pa_stMRResponse->SizeofAdditionalStatus = 0;
            }
          else
            {
              for (j = 0; j < pa_pstInstance->pstClass->nNr_of_Attributes; j++) /* for each instance attribute of this class */
                {
                  int attrNum = p_attr->CIP_AttributNr;
                  if (attrNum < 32
                      && (pa_pstInstance->pstClass->nGetAttrAllMask & 1
                          << attrNum)) /* only return attributes that are flagged as being part of GetAttributeALl */
                    {
                      pa_stMRRequest->RequestPath.AttributNr = attrNum;
                      if (EIP_OK_SEND != p_service->m_ptfuncService(
                          pa_pstInstance, pa_stMRRequest, pa_stMRResponse,
                          pa_msg))
                        {
                          return EIP_ERROR;
                        }
                      pa_msg += pa_stMRResponse->DataLength;
                    }
                  p_attr++;
                }
              pa_stMRResponse->DataLength = pa_msg - ptmp;
            }
          return EIP_OK_SEND;
        }
      p_service++;
    }
  return EIP_OK; /* Return 0 if cannot find GET_ATTRIBUTE_SINGLE service*/
}

int
encodeEPath(S_CIP_EPATH *pa_pstEPath, EIP_UINT8 **pa_pnMsg)
{
  int nLen;

  nLen = pa_pstEPath->PathSize;
  htols(pa_pstEPath->PathSize, pa_pnMsg);

  if (pa_pstEPath->ClassID < 256)
    {
      **pa_pnMsg = 0x20; /*8Bit Class Id */
      ++(*pa_pnMsg);
      **pa_pnMsg = (EIP_UINT8) pa_pstEPath->ClassID;
      ++(*pa_pnMsg);
      nLen -= 1;
    }
  else
    {
      **pa_pnMsg = 0x21; /*16Bit Class Id */
      ++(*pa_pnMsg);
      **pa_pnMsg = 0; /*padd byte */
      ++(*pa_pnMsg);
      htols(pa_pstEPath->ClassID, pa_pnMsg);
      nLen -= 2;
    }

  if (0 < nLen)
    {
      if (pa_pstEPath->InstanceNr < 256)
        {
          **pa_pnMsg = 0x24; /*8Bit Instance Id */
          ++(*pa_pnMsg);
          **pa_pnMsg = (EIP_UINT8) pa_pstEPath->InstanceNr;
          ++(*pa_pnMsg);
          nLen -= 1;
        }
      else
        {
          **pa_pnMsg = 0x25; /*16Bit Instance Id */
          ++(*pa_pnMsg);
          **pa_pnMsg = 0; /*padd byte */
          ++(*pa_pnMsg);
          htols(pa_pstEPath->InstanceNr, pa_pnMsg);
          nLen -= 2;
        }

      if (0 < nLen)
        {
          if (pa_pstEPath->AttributNr < 256)
            {
              **pa_pnMsg = 0x30; /*8Bit Attribute Id */
              ++(*pa_pnMsg);
              **pa_pnMsg = (EIP_UINT8) pa_pstEPath->AttributNr;
              ++(*pa_pnMsg);
              nLen -= 1;
            }
          else
            {
              **pa_pnMsg = 0x31; /*16Bit Attribute Id */
              ++(*pa_pnMsg);
              **pa_pnMsg = 0; /*padd byte */
              ++(*pa_pnMsg);
              htols(pa_pstEPath->AttributNr, pa_pnMsg);
              nLen -= 2;
            }
        }
    }

  return 2 + pa_pstEPath->PathSize * 2; /*path size is in 16 bit chunks according to the spec */
}

