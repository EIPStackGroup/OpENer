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

EIP_UINT32 TCP_Status = 0; /* #1  This is a TCPIP object attribute. For now it is always zero. */
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
  { 0, NULL};

/*!Multicast address to be used for I/O connections*/
EIP_UINT32 g_nMultiCastAddress;

EIP_STATUS
configureNetworkInterface(const char *pa_acIpAdress,
    const char *pa_acSubNetMask, const char *pa_acGateway)
{
  EIP_UINT32 nHostId;
  
  Interface_Configuration.IPAddress = inet_addr(pa_acIpAdress);
  Interface_Configuration.NetworkMask = inet_addr(pa_acSubNetMask);
  Interface_Configuration.Gateway = inet_addr(pa_acGateway);

  /* calculate the CIP multicast address. The multicast address is calculated, not input*/
  nHostId = ntohl(Interface_Configuration.IPAddress) & ~ ntohl(
      Interface_Configuration.NetworkMask); /* see CIP spec 3-5.3 for multicast address algorithm*/
  nHostId -= 1;
  nHostId &= 0x3ff;
  g_nMultiCastAddress = htonl(ntohl(inet_addr("239.192.1.0")) + (nHostId << 5));

  return EIP_OK;
}

//FIXME check on NULL

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
      Interface_Configuration.DomainName.String = (EIP_INT8 *) IApp_CipCalloc(
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
      Hostname.String = (EIP_INT8 *) IApp_CipCalloc(Hostname.Length + 1,
          sizeof(EIP_INT8));
      strcpy(Hostname.String, pa_acHostName);
    }
  else
    {
      Hostname.String = NULL;
    }
}

EIP_STATUS
setAttributeSingleTCP(S_CIP_Instance *pa_pstObjectInstance, /* pointer to instance*/
S_CIP_MR_Request *pa_pstMRRequest, /* pointer to message router request */
S_CIP_MR_Response *pa_pstMRResponse, /* pointer to message router response*/
EIP_UINT8 *pa_msg)
{
  pa_pstObjectInstance = 0; /*surrpress compiler warning */
  pa_msg = 0; /*surrpress compiler warning */

  if ((1 <= pa_pstMRRequest->RequestPath.AttributNr)
      && (pa_pstMRRequest->RequestPath.AttributNr <= 6))
    {
      /* it is an attribute we currently support, however no attribute is setable */
      /*TODO if you like to have a device that can be configured via this CIP object add your code here */
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
  6, /* # instance attributes*/
  0xffffffff, /* instance getAttributeAll mask*/
  1, /* # instance services*/
  1, /* # instances*/
  "TCP/IP interface", 1)) == 0)
    {
      return EIP_ERROR;
    }
  pstInstance = getCIPInstance(p_stTCPIPClass, 1); /* bind attributes to the instance #1 that was created above*/

  insertAttribute(pstInstance, 1, CIP_DWORD, (void *) &TCP_Status);
  insertAttribute(pstInstance, 2, CIP_DWORD, (void *) &Configuration_Capability);
  insertAttribute(pstInstance, 3, CIP_DWORD, (void *) &Configuration_Control);
  insertAttribute(pstInstance, 4, CIP_EPATH, &Physical_Link_Object);
  insertAttribute(pstInstance, 5, CIP_UDINT_UDINT_UDINT_UDINT_UDINT_STRING,
      &Interface_Configuration);
  insertAttribute(pstInstance, 6, CIP_STRING, (void *) &Hostname);

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

