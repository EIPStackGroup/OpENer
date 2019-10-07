/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#define WIN32_LEAN_AND_MEAN
#include "ciptcpipinterface.h"
#include "networkconfig.h"
#include "cipcommon.h"
#include "ciperror.h"
#include "opener_api.h"
#include "trace.h"
#include "cipethernetlink.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3


EipStatus ConfigureNetworkInterface(const char *const network_interface) {

  PIP_ADAPTER_INFO pAdapterInfo;
  PIP_ADAPTER_INFO pAdapter = NULL;
  CipDword dwRetVal = 0;

  CipUdint ulOutBufLen = sizeof(IP_ADAPTER_INFO);
  pAdapterInfo = (IP_ADAPTER_INFO *)CipCalloc(1,sizeof(IP_ADAPTER_INFO) );
  if (pAdapterInfo == NULL) {
    printf("Error allocating memory needed to call GetAdaptersinfo\n");
    return kEipStatusError;
  }
  // Make an initial call to GetAdaptersInfo to get
  // the necessary size into the ulOutBufLen variable
  if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
    CipFree(pAdapterInfo);
    pAdapterInfo = (IP_ADAPTER_INFO *)CipCalloc(ulOutBufLen,sizeof(CipUdint) );
    if (pAdapterInfo == NULL) {
      printf("Error allocating memory needed to call GetAdaptersinfo\n");
      return kEipStatusError;
    }
  }


  if ( (dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) ) == NO_ERROR ) {
    pAdapter = pAdapterInfo;
    while (pAdapter) {
      if (strcmp( pAdapter->IpAddressList.IpAddress.String,
                  network_interface) == 0) {
        for (int i = 0; i < 6; i++) {
          memcpy(&g_ethernet_link.physical_address, pAdapter->Address,
                 6 * sizeof(CipUsint) );
        }

        g_tcpip.interface_configuration.ip_address = inet_addr(
          pAdapter->IpAddressList.IpAddress.String);
        g_tcpip.interface_configuration.network_mask = inet_addr(
          pAdapter->IpAddressList.IpMask.String);
        g_tcpip.interface_configuration.gateway = inet_addr(
          pAdapter->GatewayList.IpAddress.String);

        /* Calculate the CIP multicast address. The multicast address is
         * derived from the current IP address. */
        CipTcpIpCalculateMulticastIp(&g_tcpip);
      }
      pAdapter = pAdapter->Next;
    }
  }
  else {
    printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);

  }
  CipFree(pAdapterInfo);
  CipFree(pAdapter);
  return kEipStatusOk;
}

void ConfigureDomainName() {
  // This was a parameter!
  int interface_index = 0;

  CipDword dwSize = 0;
  int i = 0;
  // Set the flags to pass to GetAdaptersAddresses
  CipUdint flags = GAA_FLAG_INCLUDE_PREFIX;
  CipDword dwRetVal = 0;
  // default to unspecified address family (both)
  CipUdint family = AF_UNSPEC;

  LPVOID lpMsgBuf = NULL;

  PIP_ADAPTER_ADDRESSES pAddresses = NULL;
  PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
  IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
  CipUdint outBufLen = 0;
  CipUdint tries = 0;

  family = AF_INET;
  // Allocate a 15 KB buffer to start with.
  outBufLen = WORKING_BUFFER_SIZE;

  do {

    pAddresses = (IP_ADAPTER_ADDRESSES *)CipCalloc(1,outBufLen);
    if (pAddresses == NULL) {
      printf
        ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
      exit(1);
    }

    dwRetVal =
      GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
      CipFree(pAddresses);
      pAddresses = NULL;
    }
    else {
      break;
    }

    tries++;

  } while ( (dwRetVal == ERROR_BUFFER_OVERFLOW) && (tries < MAX_TRIES) );

  if (dwRetVal == NO_ERROR) {
    // If successful, output some information from the data we received
    pCurrAddresses = pAddresses;
    while (pCurrAddresses) {
      if (interface_index == pCurrAddresses->IfIndex) {
        pDnServer = pCurrAddresses->FirstDnsServerAddress;
        if (pDnServer) {
          for (i = 0; pDnServer != NULL; i++) {
            pDnServer = pDnServer->Next;
          }
        }

        char pStringBuf[INET_ADDRSTRLEN];
        if (i != 0) {

          if (NULL != g_tcpip.interface_configuration.domain_name.string) {
            /* if the string is already set to a value we have to free the resources
             * before we can set the new value in order to avoid memory leaks.
             */
            CipFree(g_tcpip.interface_configuration.domain_name.string);
          }
          g_tcpip.interface_configuration.domain_name.length = strlen(
            pCurrAddresses->DnsSuffix);
          if (g_tcpip.interface_configuration.domain_name.length) {
            g_tcpip.interface_configuration.domain_name.string =
              (CipByte *)CipCalloc(
                g_tcpip.interface_configuration.domain_name.length + 1,
                sizeof(CipUsint) );
            strcpy(g_tcpip.interface_configuration.domain_name.string,
                   pCurrAddresses->DnsSuffix);
          }
          else {
            g_tcpip.interface_configuration.domain_name.string = NULL;
          }
        }
        else{ g_tcpip.interface_configuration.domain_name.length = 0;}

      }
      pCurrAddresses = pCurrAddresses->Next;
    }
  }
  else {
    OPENER_TRACE_INFO("Call to GetAdaptersAddresses failed with error: %d\n",
                      dwRetVal);
    if (dwRetVal == ERROR_NO_DATA) {
      OPENER_TRACE_INFO(
        "\tNo addresses were found for the requested parameters\n");
    }
    else {

      if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, dwRetVal,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        // Default language
                        (LPTSTR)&lpMsgBuf, 0, NULL) ) {
        OPENER_TRACE_INFO("\tError: %s", lpMsgBuf);
        CipFree(lpMsgBuf);
        if (pAddresses) {
          CipFree(pAddresses);
        }
        exit(1);
      }
    }
  }

  if (pAddresses) {
    CipFree(pAddresses);
  }


}

void ConfigureHostName(void) {
  CipWord wVersionRequested;
  WSADATA wsaData;
  int err;

  /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
  wVersionRequested = MAKEWORD(2, 2);

  err = WSAStartup(wVersionRequested, &wsaData);
  if (err != 0) {
    /* Tell the user that we could not find a usable */
    /* Winsock DLL.                                  */
    printf("WSAStartup failed with error: %d\n", err);
    return;
  }

  char hostname[256] = "";
  gethostname(hostname, sizeof(hostname) );

  //WSACleanup();




  if (NULL != g_tcpip.hostname.string) {
    /* if the string is already set to a value we have to free the resources
     * before we can set the new value in order to avoid memory leaks.
     */
    CipFree(g_tcpip.hostname.string);
  }
  g_tcpip.hostname.length = strlen(hostname);
  if (g_tcpip.hostname.length) {
    g_tcpip.hostname.string = (CipByte *) CipCalloc(
      g_tcpip.hostname.length + 1,
      sizeof(CipByte) );
    strcpy(g_tcpip.hostname.string, hostname);
  } else {
    g_tcpip.hostname.string = NULL;
  }
}
