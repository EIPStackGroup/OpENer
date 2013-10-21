/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <string.h>
#include "opener_user_conf.h"
#include "ciptcpipinterface.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "cipethernetlink.h"
#include "opener_api.h"
EIP_UINT32 TCP_Status = 0x1; /* #1  TCP status with 1 we indicate that we got a valid configuration from dhcp or bootp */
EIP_UINT32 Configuration_Capability = 0x04; /* #2  This is a default value meaning that it is a DHCP client see 5-3.2.2.2 EIP spec*/
EIP_UINT32 Configuration_Control = 0; /* #3  This is a TCPIP object attribute. For now it is always zero and is not used for anything. */
S_CIP_EPATH Physical_Link_Object = /* #4 */
  { 2, /* EIP_UINT8 PathSize in 16 Bit chunks*/
  CIP_ETHERNETLINK_CLASS_CODE, /* EIP_UINT16 ClassID*/
  1, /* EIP_UINT16 InstanceNr*/
  0 /* EIP_UINT16 AttributNr (not used as this is the EPATH the EthernetLink object)*/
  };

S_CIP_TCPIPNetworkInterfaceConfiguration Interface_Configuration = /* #5 */
  { 0, /* default IP address */
  0, /* NetworkMask */
  0, /* Gateway */
  0, /* NameServer */
  0, /* NameServer2 */
    { /* DomainName */
    0, NULL, } };

S_CIP_String Hostname = /* #6 */
  { 0, NULL };
/*! #8 the time to live value to be used for multi-cast connections
 *
 * Currently we implement it non setable and with the default value of 1.
 */EIP_UINT8 g_unTTLValue = 1;

/* ! #9 The multi cast configuration for this device
 *
 * Currently we implement it non setable and use the default
 * allocation algorithm
 */
SMcastConfig g_stMultiCastconfig =
  { 0, /* us the default allocation algorithm */
  0, /* reserved */
  1, /* we currently use only one multicast address */
  0 /* the multicast address will be allocated on ip address configuration */
  };

/************** Functions ****************************************/
EIP_STATUS
getAttributeSingleTCPIPInterface(S_CIP_Instance *pa_pstInstance,
    S_CIP_MR_Request *pa_pstMRRequest, S_CIP_MR_Response *pa_pstMRResponse);

EIP_STATUS
getAttributeAllTCPIPInterface(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_stMRRequest, S_CIP_MR_Response * pa_stMRResponse);

EIP_STATUS
configureNetworkInterface(const char *pa_acIpAdress,
    const char *pa_acSubNetMask, const char *pa_acGateway)
{
  EIP_UINT32 nHostId;

  Interface_Configuration.IPAddress = inet_addr(pa_acIpAdress);
  Interface_Configuration.NetworkMask = inet_addr(pa_acSubNetMask);
  Interface_Configuration.Gateway = inet_addr(pa_acGateway);

  /* calculate the CIP multicast address. The multicast address is calculated, not input*/
  nHostId = ntohl(Interface_Configuration.IPAddress)
      & ~ntohl(Interface_Configuration.NetworkMask); /* see CIP spec 3-5.3 for multicast address algorithm*/
  nHostId -= 1;
  nHostId &= 0x3ff;

  g_stMultiCastconfig.m_unMcastStartAddr = htonl(
      ntohl(inet_addr("239.192.1.0")) + (nHostId << 5));

  return EIP_OK;
}

void
configureDomainName(const char *pa_acDomainName)
{
  if (NULL != Interface_Configuration.DomainName.String)
    {
      /* if the string is already set to a value we have to free the resources
       * before we can set the new value in order to avoid memory leaks.
       */
      IApp_CipFree(Interface_Configuration.DomainName.String);
    }
  Interface_Configuration.DomainName.Length = strlen(pa_acDomainName);
  if (Interface_Configuration.DomainName.Length)
    {
      Interface_Configuration.DomainName.String = (EIP_BYTE *) IApp_CipCalloc(
          Interface_Configuration.DomainName.Length + 1, sizeof(EIP_INT8));
      strcpy(Interface_Configuration.DomainName.String, pa_acDomainName);
    }
  else
    {
      Interface_Configuration.DomainName.String = NULL;
    }
}

void
configureHostName(const char *pa_acHostName)
{
  if (NULL != Hostname.String)
    {
      /* if the string is already set to a value we have to free the resources
       * before we can set the new value in order to avoid memory leaks.
       */
      IApp_CipFree(Hostname.String);
    }
  Hostname.Length = strlen(pa_acHostName);
  if (Hostname.Length)
    {
      Hostname.String = (EIP_BYTE *) IApp_CipCalloc(Hostname.Length + 1,
          sizeof(EIP_BYTE));
      strcpy(Hostname.String, pa_acHostName);
    }
  else
    {
      Hostname.String = NULL;
    }
}

EIP_STATUS
setAttributeSingleTCP(S_CIP_Instance *pa_pstObjectInstance,
    S_CIP_MR_Request *pa_pstMRRequest, S_CIP_MR_Response *pa_pstMRResponse)
{
  S_CIP_attribute_struct *pAttribute = getAttribute(pa_pstObjectInstance,
      pa_pstMRRequest->RequestPath.AttributNr);
  (void) pa_pstObjectInstance; /*Suppress compiler warning */

  if (0 != pAttribute)
    {
      /* it is an attribute we currently support, however no attribute is setable */
      /* TODO: if you like to have a device that can be configured via this CIP object add your code here */
      /* TODO: check for flags associated with attributes */
      pa_pstMRResponse->GeneralStatus = CIP_ERROR_ATTRIBUTE_NOT_SETTABLE;
    }
  else
    {
      /* we don't have this attribute */
      pa_pstMRResponse->GeneralStatus = CIP_ERROR_ATTRIBUTE_NOT_SUPPORTED;
    }

  pa_pstMRResponse->SizeofAdditionalStatus = 0;
  pa_pstMRResponse->DataLength = 0;
  pa_pstMRResponse->ReplyService = (0x80 | pa_pstMRRequest->Service);
  return EIP_OK_SEND;
}

EIP_STATUS
CIP_TCPIP_Interface_Init()
{
  S_CIP_Class *p_stTCPIPClass;
  S_CIP_Instance *pstInstance;

  if ((p_stTCPIPClass = createCIPClass(CIP_TCPIPINTERFACE_CLASS_CODE, 0, /* # class attributes*/
  0xffffffff, /* class getAttributeAll mask*/
  0, /* # class services*/
  8, /* # instance attributes*/
  0xffffffff, /* instance getAttributeAll mask*/
  1, /* # instance services*/
  1, /* # instances*/
  "TCP/IP interface", 1)) == 0)
    {
      return EIP_ERROR;
    }
  pstInstance = getCIPInstance(p_stTCPIPClass, 1); /* bind attributes to the instance #1 that was created above*/

  insertAttribute(pstInstance, 1, CIP_DWORD, (void *) &TCP_Status, CIP_ATTRIB_GETABLE);
  insertAttribute(pstInstance, 2, CIP_DWORD,
      (void *) &Configuration_Capability, CIP_ATTRIB_GETABLE);
  insertAttribute(pstInstance, 3, CIP_DWORD, (void *) &Configuration_Control, CIP_ATTRIB_GETABLE);
  insertAttribute(pstInstance, 4, CIP_EPATH, &Physical_Link_Object, CIP_ATTRIB_GETABLE);
  insertAttribute(pstInstance, 5, CIP_UDINT_UDINT_UDINT_UDINT_UDINT_STRING,
      &Interface_Configuration, CIP_ATTRIB_GETABLE);
  insertAttribute(pstInstance, 6, CIP_STRING, (void *) &Hostname, CIP_ATTRIB_GETABLE);

  insertAttribute(pstInstance, 8, CIP_USINT, (void *) &g_unTTLValue, CIP_ATTRIB_GETABLE);
  insertAttribute(pstInstance, 9, CIP_ANY, (void *) &g_stMultiCastconfig, CIP_ATTRIB_GETABLE);

  insertService(p_stTCPIPClass, CIP_GET_ATTRIBUTE_SINGLE,
      &getAttributeSingleTCPIPInterface, "GetAttributeSingleTCPIPInterface");

  insertService(p_stTCPIPClass, CIP_GET_ATTRIBUTE_ALL,
      &getAttributeAllTCPIPInterface, "GetAttributeAllTCPIPInterface");

  insertService(p_stTCPIPClass, CIP_SET_ATTRIBUTE_SINGLE,
      &setAttributeSingleTCP, "SetAttributeSingle");

  return EIP_OK;
}

void
shutdownTCPIP_Interface(void)
{
  /*Only free the resources if they are initialized */
  if (NULL != Hostname.String)
    {
      IApp_CipFree(Hostname.String);
      Hostname.String = NULL;
    }

  if (NULL != Interface_Configuration.DomainName.String)
    {
      IApp_CipFree(Interface_Configuration.DomainName.String);
      Interface_Configuration.DomainName.String = NULL;
    }
}

EIP_STATUS
getAttributeSingleTCPIPInterface(S_CIP_Instance *pa_pstInstance,
    S_CIP_MR_Request *pa_pstMRRequest, S_CIP_MR_Response *pa_pstMRResponse)
{
  EIP_STATUS nRetVal = EIP_OK_SEND;
  EIP_BYTE *paMsg = pa_pstMRResponse->Data;
  EIP_UINT32 unMultiCastAddr;

  if (9 == pa_pstMRRequest->RequestPath.AttributNr)
    { /* attribute 9 can not be easily handled with the default mechanism therefore we will do it by hand */
      pa_pstMRResponse->DataLength = 0;
      pa_pstMRResponse->ReplyService = (0x80 | pa_pstMRRequest->Service);
      pa_pstMRResponse->GeneralStatus = CIP_ERROR_SUCCESS;
      pa_pstMRResponse->SizeofAdditionalStatus = 0;

      pa_pstMRResponse->DataLength += encodeData(CIP_USINT,
          &(g_stMultiCastconfig.m_unAllocControl), &paMsg);
      pa_pstMRResponse->DataLength += encodeData(CIP_USINT,
          &(g_stMultiCastconfig.m_unReserved), &paMsg);
      pa_pstMRResponse->DataLength += encodeData(CIP_UINT,
          &(g_stMultiCastconfig.m_unNumMcast), &paMsg);

      unMultiCastAddr = ntohl(
          g_stMultiCastconfig.m_unMcastStartAddr);

      pa_pstMRResponse->DataLength += encodeData(CIP_UDINT, &unMultiCastAddr,
          &paMsg);
    }
  else
    {
      nRetVal = getAttributeSingle(pa_pstInstance, pa_pstMRRequest,
          pa_pstMRResponse);
    }
  return nRetVal;
}

EIP_STATUS
getAttributeAllTCPIPInterface(S_CIP_Instance * pa_pstInstance,
    S_CIP_MR_Request * pa_stMRRequest, S_CIP_MR_Response * pa_stMRResponse)
{

  int j;
  EIP_UINT8 *ptmp;
  int nAttrNum;
  S_CIP_attribute_struct *pstAttribute;

  ptmp = pa_stMRResponse->Data; /* pointer into the reply */
  pstAttribute = pa_pstInstance->pstAttributes;

  for (j = 0; j < pa_pstInstance->pstClass->nNr_of_Attributes; j++) /* for each instance attribute of this class */
    {
      nAttrNum = pstAttribute->CIP_AttributNr;
      if (nAttrNum < 32
          && (pa_pstInstance->pstClass->nGetAttrAllMask & 1 << nAttrNum)) /* only return attributes that are flagged as being part of GetAttributeALl */
        {
          pa_stMRRequest->RequestPath.AttributNr = nAttrNum;

          if (8 == nAttrNum)
            { /* insert 6 zeros for the required empty safety network number according to Table 5-3.10 */
              memset(pa_stMRResponse->Data, 0, 6);
              pa_stMRResponse->Data += 6;
            }

          if (EIP_OK_SEND
              != getAttributeSingleTCPIPInterface(pa_pstInstance,
                  pa_stMRRequest, pa_stMRResponse))
            {
              pa_stMRResponse->Data = ptmp;
              return EIP_ERROR;
            }
          pa_stMRResponse->Data += pa_stMRResponse->DataLength;
        }
      pstAttribute++;
    }
  pa_stMRResponse->DataLength = pa_stMRResponse->Data - ptmp;
  pa_stMRResponse->Data = ptmp;

  return EIP_OK_SEND;
}
