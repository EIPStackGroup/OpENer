/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/* ---------- Include files ---------------------------- */
#define WIN32_LEAN_AND_MEAN
#include "networkconfig.h"

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "cipcommon.h"
#include "cipstring.h"
#include "opener_api.h"
#include "opener_error.h"
#include "trace.h"


/* ---------- Macro definitions ------------------------ */
#define MALLOC(x) malloc(x)
#define FREE(x)   free(x)

/* ----- Windows types PRI macros ------------- */
#define PRIDW   "lu"
#define PRIUL   "lu"
#define PRIuSZT PRIuPTR
#define PRIxSZT PRIxPTR


/* ---------- Local functions implementation ----------- */

/** @brief Extract interface index from string if string is a number
 *
 *  @param  iface   interface identifier string
 *  @return         converted number; on failure errno != 0
 *
 * Convert a number (interface index) from the supplied interface identifier
 *  string if possible. On conversion success @p errno is set to zero.
 *  On failure @p errno is set to ERANGE or EINVAL.
 */
static ULONG StrToIfaceIdx(const char *iface) {
  ULONG iface_idx;
  char  *end;
  /* The input string is a decimal interface index number or an
   *  interface name. */
  errno = 0;    /* To distinguish success / failure later */
  iface_idx = strtoul(iface, &end, 10);
  /* overflow is signaled by (errno == ERANGE) */

  if ( (iface == end) ||    /* No digits were found */
       ('\0' != *end) ) {   /* More characters after number */
    errno = EINVAL;         /* Signal conversion failure */
  }
  return iface_idx;
}

/** @brief Derive the interface index from a given interface alias name string
 *
 *  @param  iface     interface identifier string
 *  @param  if_index  pointer to return the derived interface index
 *  @return           Success: NO_ERROR, Failure any matching Windows error code
 *
 * The interface alias name supplied via the @p iface parameter is first
 *  converted from a multi-byte string to a wide character string to be able to
 *  call ConvertInterfaceAliasToLuid(). This function returns the Locally
 *  Unique IDentifier (LUID) if an interface matching the alias name is found.
 *  The match doesn't need to be exact. A matching unique prefix of the alias
 *  is sufficient.
 * The LUID can then be used to retrieve the interface index needed for the
 *  other functions by calling ConvertInterfaceLuidToIndex().
 */
static DWORD ConvertToIndexFromFakeAlias(const char *iface,
                                         PNET_IFINDEX iface_idx) {
  WCHAR       *p_if_alias;
  NET_LUID if_luid;

  size_t mbtowc_rc = mbstowcs(NULL, iface, 0);
  if ( (size_t)-1 == mbtowc_rc ) {  /* Invalid byte sequence encountered */
    return ERROR_NO_UNICODE_TRANSLATION;
  }

  size_t wc_cnt = mbtowc_rc + 1U; /* +1U for nul character */
  p_if_alias = MALLOC(sizeof(WCHAR) * wc_cnt);
  if (NULL == p_if_alias) {
    return ERROR_OUTOFMEMORY;
  }

  (void) mbstowcs(p_if_alias, iface, wc_cnt);
  DWORD cnv_status = ConvertInterfaceAliasToLuid(p_if_alias, &if_luid);
  if (NETIO_SUCCESS(cnv_status) ) {
    cnv_status = ConvertInterfaceLuidToIndex(&if_luid, iface_idx);
  }
  FREE(p_if_alias);
  return cnv_status;
}

/** @brief Determines the interface index number from string input
 *
 *  @param  iface     string with interface index number or interface alias name
 *  @param  iface_idx address of interface index destination variable
 *  @param            EipStatusOk on success, EipStatusError on failure
 *
 * This function tries to determine a Windows interface index from the @ref iface
 *  string.
 *
 * At first it is tried to evaluate the input as a decimal number if that
 *  succeeds the function returns the converted number and EipStatusOk.
 *
 * If the input string is not a number it is assumed to be a Windows interface
 *  alias name. This function then in turn calls ConvertToIndexFromFakeAlias()
 *  to find an interface matching that alias.
 */
static EipStatus DetermineIfaceIndexByString(const char *iface,
                                             PNET_IFINDEX iface_idx) {
  *iface_idx = StrToIfaceIdx(iface);

  BOOL arg_is_numerical = (0 == errno);
  if (!arg_is_numerical) {
    DWORD cnv_status = ConvertToIndexFromFakeAlias(iface, iface_idx);
    if (NO_ERROR != cnv_status) {
      char *error_message = GetErrorMessage(cnv_status);
      OPENER_TRACE_ERR(
        "ConvertToIndexFromFakeAlias() failed: %" PRIDW " - %s\n",
        cnv_status,
        error_message);
      FreeErrorMessage(error_message);
      return kEipStatusError;
    }
  }
  return kEipStatusOk;
}

/** @brief Retrieve an IP_ADAPTER_ADDRESSES table in an allocated memory block
 *
 *  @param  flags         specify what kind of information to include in the
 *                        result
 *  @param  pp_addr_table pointer to the location where to put the
 *                        PIP_ADAPTER_ADDRESSES pointer
 *
 * This function encapsulates the needed memory allocation and retry logic that
 *  is needed to call GetAdaptersAddresses() successfully and to retrieve the
 *  complete IP_ADAPTER_ADDRESSES table.
 * The @p flags parameter is used to tell the GetAdaptersAddresses() function
 *  which information to include and what information to exclude from the
 *  result.
 */
EipStatus RetrieveAdapterAddressesTable(ULONG flags,
                                        PIP_ADAPTER_ADDRESSES *pp_addr_table) {
  PIP_ADAPTER_ADDRESSES p_addr_table;
  ULONG ret_val;
  /* Start allocating with a guessed minimum size. */
  ULONG outBufLen = 16 * sizeof(IP_ADAPTER_ADDRESSES);
  do {
    p_addr_table = (PIP_ADAPTER_ADDRESSES)MALLOC(outBufLen);
    if (NULL == p_addr_table) {
      OPENER_TRACE_ERR(
        "Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
      return kEipStatusError;
    }
    ret_val = GetAdaptersAddresses(AF_INET,
                                   flags,
                                   NULL,
                                   p_addr_table,
                                   &outBufLen);

    if (ERROR_BUFFER_OVERFLOW == ret_val) {
      FREE(p_addr_table);
      p_addr_table = NULL;
    }
  } while (ERROR_BUFFER_OVERFLOW == ret_val);

  if (NO_ERROR != ret_val || NULL == p_addr_table) {
    if (NULL != p_addr_table) {
      FREE(p_addr_table);
      p_addr_table = NULL;
    }
    char *error_message = GetErrorMessage(ret_val);
    OPENER_TRACE_ERR("GetAdaptersAddresses() failed: %" PRIUL " - %s\n",
                     ret_val, error_message);
    FreeErrorMessage(error_message);
    return kEipStatusError;
  }
  *pp_addr_table = p_addr_table;
  return kEipStatusOk;
}

/** @brief Converts a wide-character string to a CIP string.
 *
 * @param src Source wide-character string.
 *
 * @param dest Destination CIP string.
 *
 * @return kEipStatusOk if the conversion was successful;
 *         kEipStatusError if a memory allocation error occurred or
 *         the source string was too large.
 */
static DWORD WideToCipString(const WCHAR *const src,
                             CipString *const dest) {
  void *buf = NULL;

  OPENER_ASSERT(src != NULL);
  OPENER_ASSERT(dest != NULL);

  /*
   * Evaluate the source string, ensuring two properties:
   *  1) the source string can be encoded as multi-byte sequence
   *  2) the number of characters fits in EipUint16, excluding
   *    the nul terminator.
   */
  const size_t num_chars = wcstombs(NULL, src, 0);
  if ( (size_t)-1 == num_chars ) {
    return ERROR_NO_UNICODE_TRANSLATION;
  }
  if (num_chars >= UINT16_MAX) {
    return ERROR_BUFFER_OVERFLOW;
  }

  /* New buffer includes nul termination. */
  const size_t buffer_size = num_chars + 1U;

  if (num_chars) {
    /* Allocate a new destination buffer. */
    buf = MALLOC(buffer_size);
    if (NULL == buf) {
      return ERROR_OUTOFMEMORY;
    }

    /* Transfer the string to the new buffer. */
    size_t cnv_chars = wcstombs(buf, src, buffer_size);
    OPENER_ASSERT(cnv_chars == num_chars);
  }

  /* Release the any previous string content. */
  FreeCipString(dest);

  /* Transfer the new content to the destination. */
  dest->length = num_chars;
  dest->string = buf;

  return ERROR_SUCCESS;
}

/** @brief Extract IPv4 IP address from SOCKET_ADDRESS structure as CipUdint
 *
 *  @param  socket_address  pointer to a Windows SOCKET_ADDRESS structure
 *  @return                 IPv4 address taken from @p socket_address
 */
static CipUdint GetIpFromSocketAddress(const SOCKET_ADDRESS *socket_address) {
  SOCKADDR_IN *sin = ( (SOCKADDR_IN *)socket_address->lpSockaddr );
  return sin->sin_addr.S_un.S_addr;
}


/* ---------- Public functions implementation ---------- */

/* For Doxygen descriptions see opener_api.h. */
EipStatus IfaceGetMacAddress(const char *iface,
                             uint8_t *physical_address) {
  ULONG iface_idx;

  if(kEipStatusOk != DetermineIfaceIndexByString(iface, &iface_idx) ) {
    return kEipStatusError;
  }

  /* Select what to include in or exclude from the adapter addresses table. */
  const ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST |
                      GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER |
                      GAA_FLAG_SKIP_FRIENDLY_NAME;
  PIP_ADAPTER_ADDRESSES p_addr_table = NULL;
  if (kEipStatusOk != RetrieveAdapterAddressesTable(flags, &p_addr_table) ) {
    return kEipStatusError;
  }

  /* Now search the right interface in the adapter addresses table. */
  PIP_ADAPTER_ADDRESSES p_addr_entry = p_addr_table;
  while (NULL != p_addr_entry) {
    if (iface_idx == p_addr_entry->IfIndex) {
      /* Get MAC address from matched interface */
      OPENER_TRACE_INFO("MAC address: %02" PRIX8 "-%02" PRIX8 "-%02" PRIX8
                        "-%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8 "\n",
                        p_addr_entry->PhysicalAddress[0],
                        p_addr_entry->PhysicalAddress[1],
                        p_addr_entry->PhysicalAddress[2],
                        p_addr_entry->PhysicalAddress[3],
                        p_addr_entry->PhysicalAddress[4],
                        p_addr_entry->PhysicalAddress[5]);
      memcpy_s(physical_address,
               6,
               p_addr_entry->PhysicalAddress,
               p_addr_entry->PhysicalAddressLength);
      break;  /* leave search after iface_idx match */
    }
    p_addr_entry = p_addr_entry->Next;
  }
  FREE(p_addr_table);

  /* Return success if we searched the table and had a match. */
  return (p_addr_entry) ? kEipStatusOk : kEipStatusError;
}

/* For Doxygen descriptions see opener_api.h. */
EipStatus IfaceGetConfiguration(const char *iface,
                                CipTcpIpInterfaceConfiguration *iface_cfg) {
  ULONG iface_idx;

  if(kEipStatusOk != DetermineIfaceIndexByString(iface, &iface_idx) ) {
    return kEipStatusError;
  }

  /* Select what to include in or exclude from the adapter addresses table. */
  const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                      GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX;
  PIP_ADAPTER_ADDRESSES p_addr_table = NULL;
  if (kEipStatusOk != RetrieveAdapterAddressesTable(flags, &p_addr_table) ) {
    return kEipStatusError;
  }

  CipTcpIpInterfaceConfiguration local_cfg;
  memset(&local_cfg, 0x00, sizeof local_cfg);

  /* Now search the right interface in the adapter addresses table. */
  PIP_ADAPTER_ADDRESSES p_addr_entry = p_addr_table;
  while (NULL != p_addr_entry) {
    if (iface_idx == p_addr_entry->IfIndex) {

      if (IfOperStatusUp != p_addr_entry->OperStatus) {
        OPENER_TRACE_ERR("IfaceGetConfiguration(): Interface '%s' is not up.\n",
                         iface);
        FREE(p_addr_table);
        return kEipStatusError;
      }
      /* Extract ip_addr, netmask, gateway, nameserver, nameserver 2, domain ... */
      {
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast =
          p_addr_entry->FirstUnicastAddress;
        if (NULL != pUnicast) {
          local_cfg.ip_address = GetIpFromSocketAddress(&pUnicast->Address);
        }
      }
      {
        PIP_ADAPTER_PREFIX pPrefix = p_addr_entry->FirstPrefix;
        if (NULL != pPrefix) {
          local_cfg.network_mask =
            htonl(0xffffffff << (32U - pPrefix->PrefixLength) );
        }
      }
      {
        PIP_ADAPTER_GATEWAY_ADDRESS pGateway =
          p_addr_entry->FirstGatewayAddress;
        if (NULL != pGateway) {
          local_cfg.gateway = GetIpFromSocketAddress(&pGateway->Address);
        }
      }
      {
        IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer =
          p_addr_entry->FirstDnsServerAddress;
        if (NULL != pDnServer) {
          local_cfg.name_server = GetIpFromSocketAddress(&pDnServer->Address);
          pDnServer = pDnServer->Next;
          if (NULL != pDnServer) {
            local_cfg.name_server_2 =
              GetIpFromSocketAddress(&pDnServer->Address);
          }
        }
      }
      DWORD ret_val = WideToCipString(p_addr_entry->DnsSuffix,
                                      &local_cfg.domain_name);
      if (NO_ERROR != ret_val) {
        char *error_message = GetErrorMessage(ret_val);
        OPENER_TRACE_ERR("WideToCipString(DnsSuffix) failed with error: %"
                         PRIDW " - %s\n", ret_val, error_message);
        FreeErrorMessage(error_message);
        FREE(p_addr_table);
        return kEipStatusError;
      }
      break;  /* leave search after iface_idx match */
    }
    p_addr_entry = p_addr_entry->Next;
  }
  FREE(p_addr_table);

  if (p_addr_entry) {
    /* Free first and then making a shallow copy of local_cfg.domain_name is
     *  ok, because local_cfg goes out of scope on return. */
    FreeCipString(&iface_cfg->domain_name);
    *iface_cfg = local_cfg;
  }

  /* Return success if we searched the table and had a match. */
  return (p_addr_entry) ? kEipStatusOk : kEipStatusError;
}

/* For Doxygen descriptions see opener_api.h. */
EipStatus IfaceWaitForIp(const char *iface,
                         int timeout,
                         volatile int *abort_wait) {
  ULONG iface_idx;

  if(kEipStatusOk != DetermineIfaceIndexByString(iface, &iface_idx) ) {
    return kEipStatusError;
  }

  {
    PMIB_IPADDRTABLE pmib_ipaddr_table = NULL;
    ULONG addr_table_sz = 0;
    uint32_t ipaddr;

#define WAIT_CYCLE_MS 100
    /* Calculate cycles of SleepEx(WAIT_CYCLE_MS) needed. */
    timeout *= (1000 / WAIT_CYCLE_MS);
    do {
      DWORD dw_ret;
      ipaddr = 0U;

      do {
        dw_ret = GetIpAddrTable(pmib_ipaddr_table, &addr_table_sz, FALSE);
        if (ERROR_INSUFFICIENT_BUFFER == dw_ret) {
          if (pmib_ipaddr_table) {
            FREE(pmib_ipaddr_table);
          }
          pmib_ipaddr_table = MALLOC(addr_table_sz);
          if (NULL == pmib_ipaddr_table) {
            OPENER_TRACE_ERR("Memory allocation failed for "
                             "MIB_IPADDRTABLE struct\n");
            return kEipStatusError;
          }
        }
      } while (ERROR_INSUFFICIENT_BUFFER == dw_ret);
      if (NO_ERROR != dw_ret) {
        char *error_message = GetErrorMessage(dw_ret);
        OPENER_TRACE_ERR("%s() failed with error: %" PRIDW " - %s\n",
                         __func__, dw_ret, error_message);
        FreeErrorMessage(error_message);
        return kEipStatusError;
      }

      /* Search entry matching the interface index and determine IP address. */
      for (int i = 0; i < (int) pmib_ipaddr_table->dwNumEntries; i++) {
        if (pmib_ipaddr_table->table[i].dwIndex == iface_idx) {
          if (0 == (pmib_ipaddr_table->table[i].wType &
                    (MIB_IPADDR_DELETED | MIB_IPADDR_DISCONNECTED |
                     MIB_IPADDR_TRANSIENT) ) ) {
            ipaddr = pmib_ipaddr_table->table[i].dwAddr;
          }
        }
      }

      if (timeout > 0) {
        --timeout;
      }
    } while ( (0 == ipaddr) && (0 != timeout) && (0 == *abort_wait) &&
              (0 == SleepEx(WAIT_CYCLE_MS, FALSE) ) );

    OPENER_TRACE_INFO("IP=%08" PRIx32 ", timeout=%d\n",
                      (uint32_t)ntohl(ipaddr),
                      timeout);
    if (pmib_ipaddr_table) {
      FREE(pmib_ipaddr_table);
    }
  }
  return kEipStatusOk;
}

#define HOST_NAME_MAX 256 /* Should be long enough according rfc1132. */
void GetHostName(CipString *hostname) {
  CipWord wVersionRequested;
  WSADATA wsaData;
  int err;

  /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
  wVersionRequested = MAKEWORD(2, 2);

  err = WSAStartup(wVersionRequested, &wsaData);
  if (err != 0) {
    /* Tell the user that we could not find a usable Winsock DLL.  */
    char *error_message = GetErrorMessage(err);
    printf("WSAStartup failed with error: %d - %s\n",
           err, error_message);
    FreeErrorMessage(error_message);
    return;
  }

  char name_buf[HOST_NAME_MAX] = "";
  err = gethostname(name_buf, sizeof(name_buf) );
  if (0 != err) {
    int error_code = GetSocketErrorNumber();
    char *error_message = GetErrorMessage(error_code);
    printf("gethostname() failed, %d - %s\n",
           error_code,
           error_message);
    FreeErrorMessage(error_message);
    return;
  }
  SetCipStringByCstr(hostname, name_buf);
}
