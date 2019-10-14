/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ciptcpipinterface.h"
#include "cipethernetlink.h"
#include "cipstring.h"
#include "networkconfig.h"
#include "cipcommon.h"
#include "ciperror.h"
#include "trace.h"
#include "opener_api.h"


EipStatus ConfigureMacAddressByInterface(const char *interface) {
  struct ifreq ifr;
  size_t if_name_len = strlen(interface);
  EipStatus status = kEipStatusError;

  if(if_name_len < sizeof(ifr.ifr_name)) {
    memcpy(ifr.ifr_name, interface, if_name_len);
    ifr.ifr_name[if_name_len] = '\0';

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if(ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
      memcpy(&(g_ethernet_link.physical_address), &ifr.ifr_hwaddr.sa_data,
             sizeof(g_ethernet_link.physical_address) );
      status = kEipStatusOk;
    }
    close(fd);
  }
  else {
    OPENER_TRACE_ERR("interface name is too long");
  }

  return status;
}

EipStatus ConfigureNetworkInterface(const char *const network_interface) {

  struct ifreq ifr;
  size_t if_name_len = strlen(network_interface);
  if(if_name_len < sizeof(ifr.ifr_name) ) {
    memcpy(ifr.ifr_name, network_interface, if_name_len);
    ifr.ifr_name[if_name_len] = 0;
  }
  else{
    OPENER_TRACE_INFO("interface name is too long\n");
  }

  {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    int ipaddr = 0;
    int netaddr = 0;
    if(ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
      ipaddr = ( (struct sockaddr_in *) &ifr.ifr_addr )->sin_addr.s_addr;
    } else {
      return kEipStatusError;
    }

    if(ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
      netaddr = ( (struct sockaddr_in *) &ifr.ifr_netmask )->sin_addr.s_addr;
    } else {
      return kEipStatusError;
    }

    g_tcpip.interface_configuration.ip_address = ipaddr;
    g_tcpip.interface_configuration.network_mask = netaddr;

    close(fd);
  }

  char route_location[] = "/proc/net/route";

  FILE *file_handle = fopen(route_location, "r");
  char file_buffer[100] = { 0 };
  char *gateway_string = NULL;
  CipUdint gateway = 0;

  if(file_handle) {
    char *needle_start = NULL;
    while(NULL ==
          (needle_start =
             strstr(file_buffer,
                    network_interface) ) &&
          fgets(file_buffer, sizeof(file_buffer), file_handle) ) {
    }

    if(NULL != needle_start) {
      char *strtok_save = NULL;
      strtok_r(needle_start, " \t", &strtok_save);
      strtok_r(needle_start, " \t", &strtok_save);
      gateway_string = strtok_r(needle_start, "\t", &strtok_save);
    }
    else{
      OPENER_TRACE_ERR("network interface: %s not found", network_interface);
      fclose(file_handle);
      exit(1);
    }
  }

  if(inet_pton(AF_INET, gateway_string, &gateway) == 1) {
    if(INADDR_LOOPBACK != gateway) {
      g_tcpip.interface_configuration.gateway = gateway;
    }
    else{
      g_tcpip.interface_configuration.gateway = 0;
    }
  }
  else{
    g_tcpip.interface_configuration.gateway = 0;
  }

  /* Calculate the CIP multicast address. The multicast address is
   * derived from the current IP address. */
  CipTcpIpCalculateMulticastIp(&g_tcpip);

  fclose(file_handle);

  return kEipStatusOk;
}

void ConfigureDomainName() {
  char resolv_conf_file[] = "/etc/resolv.conf";
  FILE *file_handle = fopen(resolv_conf_file, "r");
  char *file_buffer = NULL;
  size_t file_length;
  char *domain_name_string = NULL;
  char *dns1_string = NULL;
  char *dns2_string = NULL;

  if(file_handle) {
    fseek(file_handle, 0, SEEK_END);
    file_length = ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);
    file_buffer = malloc(file_length);
    if(file_buffer) {
      size_t rd_sz = fread(file_buffer, 1, file_length, file_handle);
      fclose(file_handle);
      if (rd_sz != file_length) {
        OPENER_TRACE_ERR("Read error on file %s\n", resolv_conf_file);
        exit(1);
      }
    }
    else{
      OPENER_TRACE_ERR("Could not allocate memory for reading file %s\n",
                       resolv_conf_file);
      fclose(file_handle);
      exit(1);
    }
  } else {
    OPENER_TRACE_ERR("Could not open file %s\n", resolv_conf_file);
    exit(1);
  }

  if(strstr(file_buffer, "domain") ) {
    char *strtok_save = NULL;
    strtok_r(file_buffer, " ", &strtok_save);
    domain_name_string = strtok_r(file_buffer, "\n", &strtok_save);

    if(NULL != g_tcpip.interface_configuration.domain_name.string) {
      /* if the string is already set to a value we have to free the resources
       * before we can set the new value in order to avoid memory leaks.
       */
      CipFree(g_tcpip.interface_configuration.domain_name.string);
    }
    g_tcpip.interface_configuration.domain_name.length = strlen(
      domain_name_string);

    if(g_tcpip.interface_configuration.domain_name.length) {
      g_tcpip.interface_configuration.domain_name.string =
        (EipByte *) CipCalloc(
          g_tcpip.interface_configuration.domain_name.length + 1,
          sizeof(EipByte) );
      /* *.domain_name.string was calloced with *.domain_name.length+1 which
       *    provides a trailing '\0' when memcpy( , , *.length) is done!
       */
      memcpy(g_tcpip.interface_configuration.domain_name.string,
             domain_name_string,
             g_tcpip.interface_configuration.domain_name.length);
    }
    else{
      g_tcpip.interface_configuration.domain_name.string = NULL;
    }
  }

  if(strstr(file_buffer, "nameserver") ) {
    char *strtok_save = NULL;
    strtok_r(file_buffer, " ", &strtok_save);
    dns1_string = strtok_r(NULL, "\n", &strtok_save);

    inet_pton(AF_INET, dns1_string,
              &g_tcpip.interface_configuration.name_server);
  }

  if(strstr(file_buffer, "nameserver ") ) {
    char *strtok_save = NULL;
    strtok_r(file_buffer, " ", &strtok_save);
    dns2_string = strtok_r(file_buffer, "\n", &strtok_save);

    inet_pton(AF_INET,
              dns2_string,
              &g_tcpip.interface_configuration.name_server_2);
  }

  free(file_buffer);
}

void ConfigureHostName() {
  char    name_buf[HOST_NAME_MAX];
  int     rc;

  rc = gethostname(name_buf, sizeof name_buf);
  name_buf[HOST_NAME_MAX-1] = '\0'; /* Ensure termination */
  if (0 == rc) {
      SetCipStringByCstr(&g_tcpip.hostname, name_buf);
  }
}
