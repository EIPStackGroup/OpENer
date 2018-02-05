/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <string.h>

#include "ciptcpipinterface.h"
#include "networkconfig.h"
#include "cipcommon.h"
#include "ciperror.h"
#include "trace.h"
#include "opener_api.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <net/if.h>
#include <netinet/in.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>

#define LOOPBACK_BINARY 0x7f000001

void ConfigureMacAddress(const char *interface) {
  struct ifreq ifr;
  size_t if_name_len = strlen(interface);
  if ( if_name_len < sizeof(ifr.ifr_name) ) {
    memcpy(ifr.ifr_name, interface, if_name_len);
    ifr.ifr_name[if_name_len] = 0;
  } else {
    OPENER_TRACE_INFO("interface name is too long");
  }

  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

  if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
    memcpy( &(g_ethernet_link.physical_address), &ifr.ifr_hwaddr.sa_data,
            sizeof(g_ethernet_link.physical_address) );
  }
  CloseSocket(fd);
}

EipStatus ConfigureNetworkInterface(const char *const network_interface) {

  struct ifreq ifr;
  size_t if_name_len = strlen(network_interface);
  if ( if_name_len < sizeof(ifr.ifr_name) ) {
    memcpy(ifr.ifr_name, network_interface, if_name_len);
    ifr.ifr_name[if_name_len] = 0;
  } else {
    OPENER_TRACE_INFO("interface name is too long\n");
  }

  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  int ipaddr = 0;
  int netaddr = 0;
  if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
    ipaddr = ( (struct sockaddr_in *) &ifr.ifr_addr )->sin_addr.s_addr;
  }

  if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
    netaddr = ( (struct sockaddr_in *) &ifr.ifr_netmask )->sin_addr.s_addr;
  }

  interface_configuration_.ip_address = ipaddr;
  interface_configuration_.network_mask = netaddr;

  FILE *file_handle = fopen("/proc/net/route", "r");
  char line[100] = { 0 };
  char *string_part1 = NULL;
  char *string_part2 = NULL;
  char *gateway_string = NULL;
  CipUdint gateway = 0;

  while ( fgets(line, 100, file_handle) ) {
    if ( strstr(line, network_interface) ) {
      string_part1 = strtok(line, " \t");
      string_part2 = strtok(NULL, " \t");
      gateway_string = strtok(NULL, "\t");
    }
  }
  fclose(file_handle);

  if (inet_pton(AF_INET, gateway_string, &gateway) == 1) {
    if (gateway != LOOPBACK_BINARY) {
      interface_configuration_.gateway = gateway;
    } else {
      interface_configuration_.gateway = 0;
    }
  } else {
    interface_configuration_.gateway = 0;
  }

  /* calculate the CIP multicast address. The multicast address is calculated, not input*/
  EipUint32 host_id = ntohl(interface_configuration_.ip_address)
                      & ~ntohl(interface_configuration_.network_mask); /* see CIP spec 3-5.3 for multicast address algorithm*/
  host_id -= 1;
  host_id &= 0x3ff;

  g_multicast_configuration.starting_multicast_address = htonl(
    ntohl( inet_addr("239.192.1.0") ) + (host_id << 5) );

  CloseSocket(fd);

  return kEipStatusOk;
}

void ConfigureDomainName() {
  FILE *file_handle = fopen("/etc/resolv.conf", "r");

  char line[100] = { 0 };
  char *string_part1 = NULL;
  char *domain_name_string = NULL;
  char *dns1_string = NULL;
  char *dns2_string = NULL;
  CipBool done_domain = false;
  CipBool done_n1 = false;

  while ( fgets(line, 100, file_handle) ) {
    if (strstr(line, "domain") && !done_domain) {
      string_part1 = strtok(line, " ");
      domain_name_string = strtok(NULL, "\n");

      if (NULL != interface_configuration_.domain_name.string) {
        /* if the string is already set to a value we have to free the resources
         * before we can set the new value in order to avoid memory leaks.
         */
        CipFree(interface_configuration_.domain_name.string);
      }
      interface_configuration_.domain_name.length = strlen(domain_name_string);

      if (interface_configuration_.domain_name.length) {
        interface_configuration_.domain_name.string = (EipByte *) CipCalloc(
          interface_configuration_.domain_name.length + 1, sizeof(EipInt8) );
        strcpy(interface_configuration_.domain_name.string, domain_name_string);
      } else {
        interface_configuration_.domain_name.string = NULL;
      }
      done_domain = true;
      continue;
    }

    if (strstr(line, "nameserver") && !done_n1) {
      string_part1 = strtok(line, " ");
      dns1_string = strtok(NULL, "\n");

      inet_pton(AF_INET, dns1_string, &interface_configuration_.name_server);

      done_n1 = true;
      continue;
    }
    if (strstr(line, "nameserver ") && done_n1) {
      string_part1 = strtok(line, " ");
      dns2_string = strtok(NULL, "\n");

      inet_pton(AF_INET, dns2_string, &interface_configuration_.name_server_2);
      break;
    }
  }
  close(file_handle);

}

void ConfigureHostName() {
  char name[1024] = { 0 };
  gethostname( name, sizeof(name) );

  if (NULL != hostname_.string) {
    /* if the string is already set to a value we have to free the resources
     * before we can set the new value in order to avoid memory leaks.
     */
    CipFree(hostname_.string);
  }
  hostname_.length = strlen(name);
  if (hostname_.length) {
    hostname_.string = (EipByte *) CipCalloc( hostname_.length + 1,
                                              sizeof(EipByte) );
    strcpy(hostname_.string, name);
  } else {
    hostname_.string = NULL;
  }
}
