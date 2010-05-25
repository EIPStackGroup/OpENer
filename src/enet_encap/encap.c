/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <string.h>
#include "opener_api.h"
#include "cpf.h"
#include "encap.h"
#include "endianconv.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "cipconnectionmanager.h"
#include "cipidentity.h"

#define INVALID_SESSION -1

#define SENDER_CONTEXT_SIZE 8                   /*size of sender context in encapsulation header*/

/* definition of known encapsulation commands */
#define COMMAND_NOP                     0x0000
#define COMMAND_LISTSERVICES            0x0004
#define COMMAND_LISTIDENTITY            0x0063
#define COMMAND_LISTINTERFACES          0x0064
#define COMMAND_REGISTERSESSION         0x0065
#define COMMAND_UNREGISTERSESSION       0x0066
#define COMMAND_SENDRRDATA              0x006F
#define COMMAND_SENDUNITDATA            0x0070

/* definition of capability flags */
#define SUPPORT_CIP_TCP                 0x0020
#define SUPPORT_CIP_UDP_CLASS_0_OR_1    0x0100

/*Identity data from cipidentity.c*/
extern EIP_UINT16 VendorID;
extern EIP_UINT16 DeviceType;
extern EIP_UINT16 ProductCode;
extern S_CIP_Revision Revison;
extern EIP_UINT16 ID_Status;
extern EIP_UINT32 SerialNumber;
extern S_CIP_Short_String ProductName;

/*ip address data taken from TCPIPInterfaceObject*/
extern S_CIP_TCPIPNetworkInterfaceConfiguration Interface_Configuration;

/*** defines ***/

#define ITEM_ID_LISTIDENTITY 0x000C

#define SUPPORTED_PROTOCOL_VERSION 1

#define SUPPORTED_OPTIONS_MASK  0x00  /*Mask of which options are supported as of the current CIP specs no other option value as 0 should be supported.*/

#define ENCAPSULATION_HEADER_SESSION_HANDLE_POS 4   /*the position of the session handle within the encapsulation header*/

/*struct S_Identity S_Identity_Object;
 const static UINT8 productname[] = "test device";
 */

struct S_Encapsulation_Interface_Information g_stInterfaceInformation;

int anRegisteredSessions[OPENER_NUMBER_OF_SUPPORTED_SESSIONS];

/*** private functions ***/
void
ListServices(struct S_Encapsulation_Data *pa_stReceiveData);
void
ListInterfaces(struct S_Encapsulation_Data *pa_stReceiveData);
void
ListIdentity(struct S_Encapsulation_Data *pa_stReceiveData);
void 
RegisterSession(int pa_nSockfd, struct S_Encapsulation_Data *pa_stReceiveData);
EIP_STATUS
UnregisterSession(struct S_Encapsulation_Data *pa_stReceiveData);
EIP_STATUS
SendUnitData(struct S_Encapsulation_Data *pa_stReceiveData);
EIP_STATUS
SendRRData(struct S_Encapsulation_Data *pa_stReceiveData);

int
getFreeSessionIndex(void);
EIP_INT16
createEncapsulationStructure(EIP_UINT8 * buf, int length,
    struct S_Encapsulation_Data *pa_S_ReceiveData);
EIP_STATUS
checkRegisteredSessions(struct S_Encapsulation_Data *pa_S_ReceiveData);
int
encapsulate_data(struct S_Encapsulation_Data *pa_S_SendData);

/*   void encapInit(void)
 *   initialize sessionlist and interfaceinformation.
 */

void
encapInit(void)
{
  int i;

  /* initialize Sessions to invalid == free session */
  for (i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; i++)
    {
      anRegisteredSessions[i] = EIP_INVALID_SOCKET;
    }

  /*TODO make the interface information configurable*/
  /* initialize interface information */
  g_stInterfaceInformation.TypeCode = CIP_ITEM_ID_LISTSERVICE_RESPONSE;
  g_stInterfaceInformation.Length = sizeof(g_stInterfaceInformation);
  g_stInterfaceInformation.EncapsulationProtocolVersion = 1;
  g_stInterfaceInformation.CapabilityFlags = SUPPORT_CIP_TCP
      | SUPPORT_CIP_UDP_CLASS_0_OR_1;
  strcpy((char *) g_stInterfaceInformation.NameofService, "communications");
}

/*   int handleReceivedExplictData(int pa_socket, EIP_UINT8* pa_buf, int pa_length. int *pa_nRemainingBytes)
 *   Read received bytes, copy to struct S_Encapsulation_data and handles the command.
 *      pa_socket	socket handle from which data is received.
 *      pa_buf		buffer to be read.
 *      pa_length	length of the data in pa_buf.
 *  return length of reply
 */
int
handleReceivedExplictData(int pa_socket, /* socket from which data was received*/
EIP_UINT8 * pa_buf, /* input buffer*/
int pa_length, /* length of input*/
int *pa_nRemainingBytes) /* return how many bytes of the input are left over after we're done here*/
{
  int nRetVal = 0;
  struct S_Encapsulation_Data sEncapData;
  /* eat the encapsulation header*/
  /* the structure contains a pointer to the encapsulated data*/
  /* returns how many bytes are left after the encapsulated data*/
  *pa_nRemainingBytes = createEncapsulationStructure(pa_buf, pa_length,
      &sEncapData);

  if (SUPPORTED_OPTIONS_MASK == sEncapData.nOptions) /*TODO generate appropriate error response*/
    {
      if (*pa_nRemainingBytes >= 0) /* check if the message is corrupt: header size + claimed payload size > than what we actually received*/
        {
          /* full package or more received */
          sEncapData.nStatus = OPENER_ENCAP_STATUS_SUCCESS;
          nRetVal = 1;
          /* most of these functions need a reply to be send */
          switch (sEncapData.nCommand_code)
            {
          case (COMMAND_NOP):
            /* Nop needs no reply and does nothing */
            nRetVal = 0;
            break;

          case (COMMAND_LISTSERVICES):
            ListServices(&sEncapData);
            break;

          case (COMMAND_LISTIDENTITY):
            ListIdentity(&sEncapData);
            break;

          case (COMMAND_LISTINTERFACES):
            ListInterfaces(&sEncapData);
            break;

          case (COMMAND_REGISTERSESSION):
            RegisterSession(pa_socket, &sEncapData);
            break;

          case (COMMAND_UNREGISTERSESSION):
            nRetVal = UnregisterSession(&sEncapData);
            break;

          case (COMMAND_SENDRRDATA):
            nRetVal = SendRRData(&sEncapData);
            break;

          case (COMMAND_SENDUNITDATA):
            nRetVal = SendUnitData(&sEncapData);
            break;
            
          default:
            sEncapData.nStatus = OPENER_ENCAP_STATUS_INVALID_COMMAND;
            sEncapData.nData_length = 0;
            break;
            }
          /* if nRetVal is greater then 0 data has to be sent */
          if(0 < nRetVal)
            {
              nRetVal = encapsulate_data(&sEncapData);
            }  
        }
    }

  return nRetVal;
}

/*   INT8 encapsulate_data(struct S_Encapsulation_Data *pa_stSendData)
 *   add encapsulation header and sender_context to data.
 *      pa_stSendData pointer to structure with header and datapointer.
 *  return size of reply
 */
int
encapsulate_data(struct S_Encapsulation_Data *pa_stSendData)
{
  EIP_UINT8 *acCommBuf = &(pa_stSendData->m_acCommBufferStart[2]);
  /*htols(pa_stSendData->nCommand_code, &pa_stSendData->pEncapsulation_Data);*/
  htols(pa_stSendData->nData_length, &acCommBuf);
  /*the CommBuf should already contain the correct session handle*/
  /*htoll(pa_stSendData->nSession_handle, &pa_stSendData->pEncapsulation_Data); */
  acCommBuf += 4;
  htoll(pa_stSendData->nStatus, &acCommBuf);
  /*the CommBuf should already contain the correct sender context*/
  /*memcpy(pa_stSendData->pEncapsulation_Data, pa_stSendData->anSender_context, SENDER_CONTEXT_SIZE);*/
  //pa_stSendData->pEncapsulation_Data += SENDER_CONTEXT_SIZE + 2; /* the plus 2 is for the options value*/
  /*the CommBuf should already contain the correct  options value*/
  /*htols((EIP_UINT16)pa_stSendData->nOptions, &pa_stSendData->pEncapsulation_Data);*/

  return ENCAPSULATION_HEADER_LENGTH + pa_stSendData->nData_length;
}


/*   INT8 ListServices(struct S_Encapsulation_Data *pa_S_ReceiveData)
 *   generate reply with "Communications Services" + compatibility Flags.
 *      pa_S_ReceiveData pointer to structure with received data
 */
void
ListServices(struct S_Encapsulation_Data *pa_stReceiveData)
{
  EIP_UINT8 *pacCommBuf = pa_stReceiveData->m_acCurrentCommBufferPos;

  pa_stReceiveData->nData_length = g_stInterfaceInformation.Length + 2;

  /* copy Interface data to nmsg for sending */
  htols(1, &pacCommBuf);
  htols(g_stInterfaceInformation.TypeCode, &pacCommBuf);
  htols((EIP_UINT16) (g_stInterfaceInformation.Length - 4), &pacCommBuf);
  htols(g_stInterfaceInformation.EncapsulationProtocolVersion, &pacCommBuf);
  htols(g_stInterfaceInformation.CapabilityFlags, &pacCommBuf);
  memcpy(pacCommBuf, g_stInterfaceInformation.NameofService,
      sizeof(g_stInterfaceInformation.NameofService));
}

void
ListInterfaces(struct S_Encapsulation_Data *pa_stReceiveData)
{
  EIP_UINT8 *pacCommBuf = pa_stReceiveData->m_acCurrentCommBufferPos;
  pa_stReceiveData->nData_length = 2;
  htols(0x0000, &pacCommBuf); /* copy Interface data to nmsg for sending */
}

/*   INT8 ListIdentity(struct S_Encapsulation_Data *pa_S_ReceiveData)
 *   send Get_Attribute_All to Identity Object and send data + sender_context back.
 *      pa_S_ReceiveData pointer to structure with received data
 *  return status
 * 			0 .. success
 */
void
ListIdentity(struct S_Encapsulation_Data * pa_stReceiveData)
{
  /* List Identity reply according to EIP/CIP Specification */
  EIP_UINT8 *pacCommBuf = pa_stReceiveData->m_acCurrentCommBufferPos;
  EIP_BYTE *acIdLenBuf;

  htols(1, &pacCommBuf); /* one item */
  htols(ITEM_ID_LISTIDENTITY, &pacCommBuf);

  acIdLenBuf = pacCommBuf;
  pacCommBuf += 2; /*at this place the real length will be inserted below*/

  htols(SUPPORTED_PROTOCOL_VERSION, &pacCommBuf);
  htols(htons(AF_INET), &pacCommBuf);
  htols(htons(OPENER_ETHERNET_PORT), &pacCommBuf);
  htoll(Interface_Configuration.IPAddress, &pacCommBuf);
  memset(pacCommBuf, 0, 8);
  pacCommBuf += 8;

  htols(VendorID, &pacCommBuf);
  htols(DeviceType, &pacCommBuf);
  htols(ProductCode, &pacCommBuf);
  *(pacCommBuf)++ = Revison.MajorRevision;
  *(pacCommBuf)++ = Revison.MinorRevision;
  htols(ID_Status, &pacCommBuf);
  htoll(SerialNumber, &pacCommBuf);
  *pacCommBuf++ = (unsigned char) ProductName.Length;
  memcpy(pacCommBuf, ProductName.String, ProductName.Length);
  pacCommBuf += ProductName.Length;
  *pacCommBuf++ = 0xFF;

  pa_stReceiveData->nData_length = pacCommBuf
      - &(pa_stReceiveData->m_acCommBufferStart[ENCAPSULATION_HEADER_LENGTH]);
  htols(pacCommBuf - acIdLenBuf - 2, &acIdLenBuf); /* the -2 is for not counting the length field*/
}

/*   void RegisterSession(struct S_Encapsulation_Data *pa_S_ReceiveData)
 *   Check supported protocol, generate session handle, send replay back to originator.
 *      pa_nSockfd       socket this request is associated to. Needed for double register check
 *      pa_S_ReceiveData pointer to received data with request/response.
 */
void
RegisterSession(int pa_nSockfd, struct S_Encapsulation_Data * pa_stReceiveData)
{
  int i;
  int nSessionIndex = 0;
  EIP_UINT8 *pacBuf;
  EIP_UINT16 nProtocolVersion = ltohs(&pa_stReceiveData->m_acCurrentCommBufferPos);
  EIP_UINT16 nOptionFlag = ltohs(&pa_stReceiveData->m_acCurrentCommBufferPos);

  /* check if requested protocol version is supported and the register session option flag is zero*/
  if ((0 < nProtocolVersion)
      && (nProtocolVersion <= SUPPORTED_PROTOCOL_VERSION) && (0 == nOptionFlag))
    { /*Option field should be zero*/
      /* check if the socket has already a session open */
      for (i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i)
        {
          if (anRegisteredSessions[i] == pa_nSockfd)
            {
              /* the socket has already registered a session this is not allowed*/
              pa_stReceiveData->nSession_handle = i + 1; /*return the already assigned session back, the cip spec is not clear about this needs to be tested*/
              pa_stReceiveData->nStatus
                  = OPENER_ENCAP_STATUS_UNSUPPORTED_PROTOCOL;
              nSessionIndex = INVALID_SESSION;
              pacBuf = &pa_stReceiveData->m_acCommBufferStart[ENCAPSULATION_HEADER_SESSION_HANDLE_POS];
              htoll(pa_stReceiveData->nSession_handle, &pacBuf); /*encapsulate_data will not update the session handle so we have to do it here by hand*/
              break;
            }
        }

      if (INVALID_SESSION != nSessionIndex)
        {
          nSessionIndex = getFreeSessionIndex();
          if (INVALID_SESSION == nSessionIndex) /* no more sessions available */
            {
              pa_stReceiveData->nStatus = OPENER_ENCAP_STATUS_INSUFFICIENT_MEM;
            }
          else
            { /* successful session registered */
              anRegisteredSessions[nSessionIndex] = pa_nSockfd; /* store associated socket */
              pa_stReceiveData->nSession_handle = nSessionIndex + 1;
              pa_stReceiveData->nStatus = OPENER_ENCAP_STATUS_SUCCESS;
              pacBuf = &pa_stReceiveData->m_acCommBufferStart[ENCAPSULATION_HEADER_SESSION_HANDLE_POS];
              htoll(pa_stReceiveData->nSession_handle, &pacBuf); /*encapsulate_data will not update the session handle so we have to do it here by hand*/
            }
        }
    }
  else
    { /* protocol not supported */
      pa_stReceiveData->nStatus = OPENER_ENCAP_STATUS_UNSUPPORTED_PROTOCOL;
    }

  pa_stReceiveData->nData_length = 4;
}

/*   INT8 UnregisterSession(struct S_Encapsulation_Data *pa_S_ReceiveData)
 *   close all corresponding TCP connections and delete session handle.
 *      pa_S_ReceiveData pointer to unregister session request with corresponding socket handle.
 */
EIP_STATUS
UnregisterSession(struct S_Encapsulation_Data * pa_stReceiveData)
{
  int i;

  if ((0 < pa_stReceiveData->nSession_handle)
      && (pa_stReceiveData->nSession_handle
          <= OPENER_NUMBER_OF_SUPPORTED_SESSIONS))
    {
      i = pa_stReceiveData->nSession_handle - 1;
      if (EIP_INVALID_SOCKET != anRegisteredSessions[i])
        {
          IApp_CloseSocket(anRegisteredSessions[i]);
          anRegisteredSessions[i] = EIP_INVALID_SOCKET;
          return EIP_OK;
        }
    }

  /* no such session registered */
  pa_stReceiveData->nData_length = 0;
  pa_stReceiveData->nStatus = OPENER_ENCAP_STATUS_INVALID_SESSION_HANDLE;
  return EIP_OK_SEND;
}

/*   INT8 SendUnitData(struct S_Encapsulation_Data *pa_S_ReceiveData)
 *   Call Connection Manager.
 *      pa_S_ReceiveData pointer to structure with data and header information.
 */
EIP_STATUS
SendUnitData(struct S_Encapsulation_Data * pa_stReceiveData)
{
  EIP_INT16 nSendSize;
  EIP_STATUS eRetVal = EIP_OK_SEND;

  /* Command specific data UDINT .. Interface Handle, UINT .. Timeout, CPF packets */
  /* don't use the data yet */
  ltohl(&pa_stReceiveData->m_acCurrentCommBufferPos); /* skip over null interface handle*/
  ltohs(&pa_stReceiveData->m_acCurrentCommBufferPos); /* skip over unused timeout value*/
  pa_stReceiveData->nData_length -= 6; /* the rest is in CPF format*/

  if (EIP_ERROR != checkRegisteredSessions(pa_stReceiveData)) /* see if the EIP session is registered*/
    {
      nSendSize
      = notifyConnectedCPF(pa_stReceiveData,
          &pa_stReceiveData->m_acCommBufferStart[ENCAPSULATION_HEADER_LENGTH]);

      if (0 < nSendSize)
        { /* need to send reply */
          pa_stReceiveData->nData_length = nSendSize;
        }
      else
        {
          eRetVal = EIP_ERROR;
        }
    }
  else
    { /* received a package with non registered session handle */
      pa_stReceiveData->nData_length = 0;
      pa_stReceiveData->nStatus = OPENER_ENCAP_STATUS_INVALID_SESSION_HANDLE;
    }
  return eRetVal;
}

/*   INT8 SendRRData(struct S_Encapsulation_Data *pa_stReceiveData)
 *   Call UCMM or Message Router if UCMM not implemented.
 *      pa_stReceiveData pointer to structure with data and header information.
 *  return status 	0 .. success.
 * 					-1 .. error
 */
EIP_STATUS
SendRRData(struct S_Encapsulation_Data * pa_stReceiveData)
{
  EIP_INT16 nSendSize;
  EIP_STATUS eRetVal = EIP_OK_SEND;
  /* Commandspecific data UDINT .. Interface Handle, UINT .. Timeout, CPF packets */
  /* don't use the data yet */
  ltohl(&pa_stReceiveData->m_acCurrentCommBufferPos); /* skip over null interface handle*/
  ltohs(&pa_stReceiveData->m_acCurrentCommBufferPos); /* skip over unused timeout value*/
  pa_stReceiveData->nData_length -= 6; /* the rest is in CPF format*/

  if (EIP_ERROR != checkRegisteredSessions(pa_stReceiveData)) /* see if the EIP session is registered*/
    {
      nSendSize
      = notifyCPF(pa_stReceiveData,
          &pa_stReceiveData->m_acCommBufferStart[ENCAPSULATION_HEADER_LENGTH]);

      if (nSendSize >= 0)
        { /* need to send reply */
          pa_stReceiveData->nData_length = nSendSize;
        }
      else
        {
          eRetVal = EIP_ERROR;
        }
    }
  else    
    { /* received a package with non registered session handle */
      pa_stReceiveData->nData_length = 0;
      pa_stReceiveData->nStatus = OPENER_ENCAP_STATUS_INVALID_SESSION_HANDLE;
    }

  return eRetVal;
}

/*   INT8 getFreeSessionIndex()
 *   search for available sessions an return index.
 *  return index of free session in anRegisteredSessions.
 * 			-1 .. no free session available
 */
int
getFreeSessionIndex(void)
{
  int i;
  for (i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; i++)
    {
      if (EIP_INVALID_SOCKET == anRegisteredSessions[i])
        {
          return i;
        }
    }
  return -1;
}

/*   INT16 createEncapsulationStructure(INT16 pa_sockfd, INT8 *pa_buf, UINT16 pa_length, struct S_Encapsulation_Data *pa_S_Data)
 *   copy data from pa_buf in little endian to host in structure.
 *      pa_length	length of the data in pa_buf.
 *      pa_S_Data	structure to which data shall be copied
 *  return difference between bytes in pa_buf an data_length
 *  		0 .. full package received
 * 			>0 .. more than one packet received
 * 			<0 .. only fragment of data portion received
 */
EIP_INT16
createEncapsulationStructure(EIP_UINT8 * pa_buf, /* receive buffer*/
int pa_length, /* size of stuff in  buffer (might be more than one message)*/
struct S_Encapsulation_Data * pa_stData) /* the struct to be created*/

{
  pa_stData->m_acCommBufferStart = pa_buf;
  pa_stData->nCommand_code = ltohs(&pa_buf);
  pa_stData->nData_length = ltohs(&pa_buf);
  pa_stData->nSession_handle = ltohl(&pa_buf);
  pa_stData->nStatus = ltohl(&pa_buf);
  /*memcpy(pa_stData->anSender_context, pa_buf, SENDER_CONTEXT_SIZE);*/
  pa_buf += SENDER_CONTEXT_SIZE;
  pa_stData->nOptions = ltohl(&pa_buf);
  pa_stData->m_acCurrentCommBufferPos = pa_buf;
  return (pa_length - ENCAPSULATION_HEADER_LENGTH - pa_stData->nData_length);
}

/*   INT8 checkRegisteredSessions(struct S_Encapsulation_Data *pa_S_ReceiveData)
 *   check if received package belongs to registered session.
 *      pa_stReceiveData received data.
 *  return 0 .. Session registered
 *  		-1 .. invalid session -> return unsupported command received
 */
EIP_STATUS
checkRegisteredSessions(struct S_Encapsulation_Data * pa_stReceiveData)
{

  if ((0 < pa_stReceiveData->nSession_handle)
      && (pa_stReceiveData->nSession_handle
          <= OPENER_NUMBER_OF_SUPPORTED_SESSIONS))
    {
      if (EIP_INVALID_SOCKET
          != anRegisteredSessions[pa_stReceiveData->nSession_handle - 1])
        {
          return EIP_OK;
        }
    }

  return EIP_ERROR;
}

void
closeSession(int pa_nSocket)
{
  int i;
  for (i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i)
    {
      if (anRegisteredSessions[i] == pa_nSocket)
        {
          IApp_CloseSocket(pa_nSocket);
          anRegisteredSessions[i] = EIP_INVALID_SOCKET;
          break;
        }
    }
}

void encapShutDown(void){
  int i;
    for (i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i)
      {
        if (EIP_INVALID_SOCKET == anRegisteredSessions[i])
          {
            IApp_CloseSocket(anRegisteredSessions[i]);
            anRegisteredSessions[i] = EIP_INVALID_SOCKET;
          }
      }
}

