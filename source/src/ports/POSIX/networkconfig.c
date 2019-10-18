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


EipStatus IfaceGetMacAddress(const char *p_iface, uint8_t *p_physical_address) {
  struct ifreq ifr;
  size_t if_name_len = strlen(p_iface);
  EipStatus status = kEipStatusError;

  if(if_name_len < sizeof(ifr.ifr_name)) {
    memcpy(ifr.ifr_name, p_iface, if_name_len);
    ifr.ifr_name[if_name_len] = '\0';

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if(ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
      memcpy(p_physical_address, &ifr.ifr_hwaddr.sa_data, 6);
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

  /* Calculate the CIP multicast address. The multicast address is
   * derived from the current IP address and network mask. */
  CipTcpIpCalculateMulticastIp(&g_tcpip);

  static const char route_location[] = "/proc/net/route";

  FILE *file_handle = fopen(route_location, "r");
  char file_buffer[132];
  char *gateway_string = NULL;

  if(!file_handle) {
    exit(EXIT_FAILURE);
  }
  else {
    char *needle_start;
    file_buffer[0] = '\0';  /* To enter the while loop */
    while(NULL ==
          (needle_start = strstr(file_buffer, network_interface) ) &&
          fgets(file_buffer, sizeof(file_buffer), file_handle) ) {
      /* Skip each non matching line */
    }
    fclose(file_handle);

    if(NULL != needle_start) {
      char *strtok_save = NULL;
      strtok_r(needle_start, " \t", &strtok_save);  /* Iface token */
      strtok_r(NULL, " \t", &strtok_save);  /* Destination token */
      gateway_string = strtok_r(NULL, " \t", &strtok_save);
    }
    else {
      OPENER_TRACE_ERR("network interface: %s not found", network_interface);
      exit(EXIT_FAILURE);
    }
  }

  CipUdint gateway = 0;
  char *p_end;
  /* The gateway string is a hex number in network byte order. */
  errno = 0;  /* To distinguish success / failure later */
  gateway = strtoul(gateway_string, &p_end, 16);

  if ((errno == ERANGE && gateway == ULONG_MAX) ||  /* overflow */
      (gateway_string == p_end) ||  /* No digits were found */
      ('\0' != *p_end) ) {          /* More characters after number */
    g_tcpip.interface_configuration.gateway = 0;
    return kEipStatusError;
  }
  /* Only reached on strtoul() conversion success */
  if(INADDR_LOOPBACK != gateway) {
    g_tcpip.interface_configuration.gateway = gateway;
  }
  else {
    g_tcpip.interface_configuration.gateway = 0;
  }
#if defined(OPENER_TRACE_ENABLED)
  {
    char ip_str[INET_ADDRSTRLEN];
    OPENER_TRACE_INFO("Decoded gateway: %s\n", inet_ntop(AF_INET, &gateway, ip_str, sizeof ip_str));  }
#endif

  return kEipStatusOk;
}

void ConfigureDomainName() {
  static const char resolv_conf_file[] = "/etc/resolv.conf";
  FILE *file_handle = fopen(resolv_conf_file, "r");
  char *file_buffer = NULL;
  size_t file_length;

  if(file_handle) {
    fseek(file_handle, 0, SEEK_END);
    file_length = ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);
    file_buffer = malloc(file_length+1u); /* +1u for zero termination */
    if(file_buffer) {
      size_t rd_sz = fread(file_buffer, 1, file_length, file_handle);
      fclose(file_handle);
      if (rd_sz != file_length) {
        OPENER_TRACE_ERR("Read error on file %s\n", resolv_conf_file);
        free(file_buffer);
        exit(EXIT_FAILURE);
      }
      file_buffer[file_length] = '\0';  /* zero terminate for sure */
    }
    else{
      OPENER_TRACE_ERR("Could not allocate memory for reading file %s\n",
                       resolv_conf_file);
      fclose(file_handle);
      exit(EXIT_FAILURE);
    }
  } else {
    OPENER_TRACE_ERR("Could not open file %s\n", resolv_conf_file);
    exit(EXIT_FAILURE);
  }

  char *value_string;
  char *strtok_save;
  char *strtok_key;
  char *p_line;
  CipUdint  dmy_dns;
  CipUdint  *p_dns = &g_tcpip.interface_configuration.name_server;
  /* Split the file_buffer into lines. */
  for (char *strtok_beg = file_buffer;
        NULL != (p_line = strtok_r(strtok_beg, "\n", &strtok_save));
        strtok_beg = NULL)
  {
    /* Inspect each line for keywords: search, domain, nameserver */
    switch (p_line[0]) {
    case '#':
      /* fall through */
    case ';':
      /* Simply skip comments */
      break;

    case 'd':
      /* fall through */
    case 's':
      strtok_r(p_line, " \t", &strtok_key);
      if (0 == strcmp("search", p_line) || 0 == strcmp("domain", p_line)) {
        if (NULL != (value_string = strtok_r(NULL, " \t", &strtok_key)))  {
          SetCipStringByCstr(&g_tcpip.interface_configuration.domain_name,
                            value_string);
        }
      }
      break;

    case 'n':
      strtok_r(p_line, " \t", &strtok_key);
      if (0 == strcmp("nameserver", p_line)) {
        if (NULL != (value_string = strtok_r(NULL, " \t", &strtok_key))) {
          inet_pton(AF_INET, value_string, p_dns);
          /* Adjust destination for next nameserver occurrence. */
          if (p_dns != &dmy_dns) {
            if (p_dns == &g_tcpip.interface_configuration.name_server) {
              p_dns = &g_tcpip.interface_configuration.name_server_2;
            }
            else {
              /* After 2 nameserver lines any further nameservers are ignored. */
              p_dns = &dmy_dns;
            }
          }
        }
      }
      break;
    }
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
