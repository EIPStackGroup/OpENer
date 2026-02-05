/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include "networkconfig.h"  // NOLINT(build/include_subdir)

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api/opener_api.h"
#include "cip/cipcommon.h"
#include "cip/ciperror.h"
#include "cip/cipstring.h"
#include "core/trace.h"
#include "lwip/netif.h"

EipStatus IfaceGetMacAddress(TcpIpInterface* iface,
                             uint8_t* const physical_address) {
  memcpy(physical_address, iface->hwaddr, NETIF_MAX_HWADDR_LEN);

  return kEipStatusOk;
}

static EipStatus GetIpAndNetmaskFromInterface(
  TcpIpInterface* iface, CipTcpIpInterfaceConfiguration* iface_cfg) {
  iface_cfg->ip_address   = iface->ip_addr.addr;
  iface_cfg->network_mask = iface->netmask.addr;

  return kEipStatusOk;
}

static EipStatus GetGatewayFromRoute(
  TcpIpInterface* iface, CipTcpIpInterfaceConfiguration* iface_cfg) {
  iface_cfg->gateway = iface->gw.addr;

  return kEipStatusOk;
}

EipStatus IfaceGetConfiguration(TcpIpInterface* iface,
                                CipTcpIpInterfaceConfiguration* iface_cfg) {
  CipTcpIpInterfaceConfiguration local_cfg;
  EipStatus status;

  memset(&local_cfg, 0x00, sizeof local_cfg);

  status = GetIpAndNetmaskFromInterface(iface, &local_cfg);
  if (kEipStatusOk == status) {
    status = GetGatewayFromRoute(iface, &local_cfg);
  }
  if (kEipStatusOk == status) {
    /* Free first and then making a shallow copy of local_cfg.domain_name is
     *  ok, because local_cfg goes out of scope now. */
    ClearCipString(&iface_cfg->domain_name);
    *iface_cfg = local_cfg;
  }
  return status;
}

void GetHostName(TcpIpInterface* iface, CipString* hostname) {
  SetCipStringByCstr(hostname, netif_get_hostname(iface));
}
