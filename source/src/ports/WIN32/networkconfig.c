/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "ciptcpipinterface.h"
#include "networkconfig.h"
#include "cipcommon.h"
#include "ciperror.h"
#include "opener_api.h"
#include "trace.h"
#include <string.h>

EipStatus ConfigureNetworkInterface(const char *ip_address,
                                    const char *subnet_mask,
                                    const char *gateway) {

  interface_configuration_.ip_address = inet_addr(ip_address);
  interface_configuration_.network_mask = inet_addr(subnet_mask);
  interface_configuration_.gateway = inet_addr(gateway);

  /* calculate the CIP multicast address. The multicast address is calculated, not input*/
  EipUint32 host_id = ntohl(interface_configuration_.ip_address)
                      & ~ntohl(interface_configuration_.network_mask); /* see CIP spec 3-5.3 for multicast address algorithm*/
  host_id -= 1;
  host_id &= 0x3ff;

  g_multicast_configuration.starting_multicast_address = htonl(
    ntohl( inet_addr("239.192.1.0") ) + (host_id << 5) );

  return kEipStatusOk;
}

void ConfigureMacAddress(const EipUint8 *const mac_address) {
  memcpy( &g_ethernet_link.physical_address, mac_address,
          sizeof(g_ethernet_link.physical_address) );

}

void ConfigureDomainName(const char *domain_name) {
  if (NULL != interface_configuration_.domain_name.string) {
    /* if the string is already set to a value we have to free the resources
     * before we can set the new value in order to avoid memory leaks.
     */
    CipFree(interface_configuration_.domain_name.string);
  }
  interface_configuration_.domain_name.length = strlen(domain_name);
  if (interface_configuration_.domain_name.length) {
    interface_configuration_.domain_name.string = (EipByte *) CipCalloc(
      interface_configuration_.domain_name.length + 1, sizeof(EipInt8) );
    strcpy(interface_configuration_.domain_name.string, domain_name);
  } else {
    interface_configuration_.domain_name.string = NULL;
  }
}

void ConfigureHostName(const char *const RESTRICT hostname) {
  if (NULL != hostname_.string) {
    /* if the string is already set to a value we have to free the resources
     * before we can set the new value in order to avoid memory leaks.
     */
    CipFree(hostname_.string);
  }
  hostname_.length = strlen(hostname);
  if (hostname_.length) {
    hostname_.string = (EipByte *) CipCalloc( hostname_.length + 1,
                                              sizeof(EipByte) );
    strcpy(hostname_.string, hostname);
  } else {
    hostname_.string = NULL;
  }
}
