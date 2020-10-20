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
#include <time.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "cipstring.h"
#include "networkconfig.h"
#include "cipcommon.h"
#include "ciperror.h"
#include "trace.h"
#include "opener_api.h"

EipStatus IfaceGetMacAddress(const char *iface,
                             uint8_t *const physical_address) {
  struct ifreq ifr;
  size_t if_name_len = strlen(iface);
  EipStatus status = kEipStatusError;

  if(if_name_len < sizeof(ifr.ifr_name) ) {
    memcpy(ifr.ifr_name, iface, if_name_len);
    ifr.ifr_name[if_name_len] = '\0';

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if(ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
      memcpy(physical_address, &ifr.ifr_hwaddr.sa_data, 6);
      status = kEipStatusOk;
    }
    close(fd);
  } else {
    errno = ENAMETOOLONG;
    OPENER_TRACE_ERR("interface name is too long\n");
  }

  return status;
}

static EipStatus GetIpAndNetmaskFromInterface(const char *iface,
                                              CipTcpIpInterfaceConfiguration *iface_cfg)
{
  struct ifreq ifr;
  size_t if_name_len = strlen(iface);
  if(if_name_len < sizeof(ifr.ifr_name) ) {
    memcpy(ifr.ifr_name, iface, if_name_len);
    ifr.ifr_name[if_name_len] = 0;
  } else {
    errno = ENAMETOOLONG;
    OPENER_TRACE_ERR("interface name is too long\n");
    return kEipStatusError;
  }

  {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    int ipaddr = 0;
    int netaddr = 0;
    if(ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
      ipaddr = ( (struct sockaddr_in *) &ifr.ifr_addr )->sin_addr.s_addr;
    } else {
      close(fd);
      return kEipStatusError;
    }

    if(ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
      netaddr = ( (struct sockaddr_in *) &ifr.ifr_netmask )->sin_addr.s_addr;
    } else {
      close(fd);
      return kEipStatusError;
    }

    iface_cfg->ip_address = ipaddr;
    iface_cfg->network_mask = netaddr;

    close(fd);
  }
  return kEipStatusOk;
}

static EipStatus GetGatewayFromRoute(const char *iface,
                                     CipTcpIpInterfaceConfiguration *iface_cfg)
{
  static const char route_location[] = "/proc/net/route";

  FILE *file_handle = fopen(route_location, "r");
  char file_buffer[132];
  char *gateway_string = NULL;

  if(!file_handle) {
    return kEipStatusError;
  } else {
    char *needle_start;
    file_buffer[0] = '\0'; /* To enter the while loop */
    while(NULL ==
          (needle_start =
             strstr(file_buffer,
                    iface) ) &&
          fgets(file_buffer, sizeof(file_buffer), file_handle) ) {
      /* Skip each non matching line */
    }
    fclose(file_handle);

    if(NULL != needle_start) {
      char *strtok_save = NULL;
      strtok_r(needle_start, " \t", &strtok_save); /* Iface token */
      strtok_r(NULL, " \t", &strtok_save); /* Destination token */
      gateway_string = strtok_r(NULL, " \t", &strtok_save);
    } else {
      OPENER_TRACE_ERR("network interface: '%s' not found\n", iface);
      return kEipStatusError;
    }
  }

  unsigned long tmp_gw;
  char *end;
  /* The gateway string is a hex number in network byte order. */
  errno = 0; /* To distinguish success / failure later */
  tmp_gw = strtoul(gateway_string, &end, 16);

  if( (errno == ERANGE && tmp_gw == ULONG_MAX) || /* overflow */
      (gateway_string == end) || /* No digits were found */
      ('\0' != *end) ) { /* More characters after number */
    iface_cfg->gateway = 0;
    return kEipStatusError;
  }
  CipUdint gateway = tmp_gw;

  /* Only reached on strtoul() conversion success */
  if(INADDR_LOOPBACK != gateway) {
    iface_cfg->gateway = gateway;
  } else {
    iface_cfg->gateway = 0;
  }
#if defined(OPENER_TRACE_ENABLED)
  {
    char ip_str[INET_ADDRSTRLEN];
    OPENER_TRACE_INFO("Decoded gateway: %s\n",
                      inet_ntop(AF_INET, &gateway, ip_str, sizeof ip_str) );
  }
#endif

  return kEipStatusOk;
}

static EipStatus GetDnsInfoFromResolvConf(
  CipTcpIpInterfaceConfiguration *iface_cfg) {
  static const char resolv_conf_file[] = "/etc/resolv.conf";
  FILE *file_handle = fopen(resolv_conf_file, "r");
  char *file_buffer = NULL;
  size_t file_length;

  if(file_handle) {
    fseek(file_handle, 0, SEEK_END);
    file_length = ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);
    file_buffer = malloc(file_length + 1U); /* +1U for zero termination */
    if(file_buffer) {
      size_t rd_sz = fread(file_buffer, 1, file_length, file_handle);
      fclose(file_handle);
      if(rd_sz != file_length) {
        OPENER_TRACE_ERR("Read error on file %s\n", resolv_conf_file);
        free(file_buffer);
        return kEipStatusError;
      }
      file_buffer[file_length] = '\0'; /* zero terminate for sure */
    } else {
      OPENER_TRACE_ERR("Could not allocate memory for reading file %s\n",
                       resolv_conf_file);
      fclose(file_handle);
      return kEipStatusError;
    }
  } else {
    OPENER_TRACE_ERR("Could not open file %s\n", resolv_conf_file);
    return kEipStatusError;
  }

  char *value_string;
  char *strtok_save;
  char *strtok_key;
  CipUdint dmy_dns;
  CipUdint *dns = &iface_cfg->name_server;
  /* Split the file_buffer into lines. */
  char *line = strtok_r(file_buffer, "\n", &strtok_save);
  while(NULL != line) {
    /* Inspect each line for keywords: search, domain, nameserver */
    switch(line[0]) {
      case '#':
      /* fall through */
      case ';':
        /* Simply skip comments */
        break;

      case 'd':
      /* fall through */
      case 's':
        strtok_r(line, " \t", &strtok_key);
        if(0 == strcmp("search", line) || 0 == strcmp("domain", line) ) {
          if(NULL != (value_string = strtok_r(NULL, " \t", &strtok_key) ) ) {
            SetCipStringByCstr(&iface_cfg->domain_name, value_string);
          }
        }
        break;

      case 'n':
        strtok_r(line, " \t", &strtok_key);
        if(0 == strcmp("nameserver", line) ) {
          if(NULL != (value_string = strtok_r(NULL, " \t", &strtok_key) ) ) {
            inet_pton(AF_INET, value_string, dns);
            /* Adjust destination for next nameserver occurrence. */
            if(dns != &dmy_dns) {
              if(dns == &iface_cfg->name_server) {
                dns = &iface_cfg->name_server_2;
              } else {
                /* After 2 nameserver lines any further nameservers are ignored. */
                dns = &dmy_dns;
              }
            }
          }
        }
        break;
    }
    line = strtok_r(NULL, "\n", &strtok_save);
  }
  free(file_buffer);
  return kEipStatusOk;
}

static int nanosleep_simple32(uint32_t sleep_ns) {
  struct timespec tsv = { 0, (long) sleep_ns };
  struct timespec trem;
  int rc;

  OPENER_ASSERT(sleep_ns < 1000000000UL);
  do {
    rc = nanosleep(&tsv, &trem);
    tsv = trem;
  } while(-1 == rc && EINTR == errno);

  return rc;
}

EipStatus IfaceGetConfiguration(const char *iface,
                                CipTcpIpInterfaceConfiguration *iface_cfg) {
  CipTcpIpInterfaceConfiguration local_cfg;
  EipStatus status;

  memset(&local_cfg, 0x00, sizeof local_cfg);

  status = GetIpAndNetmaskFromInterface(iface, &local_cfg);
  if(kEipStatusOk == status) {
    (void) nanosleep_simple32(300000000u); /* sleep 300ms to let route "settle" */
    status = GetGatewayFromRoute(iface, &local_cfg);
    if(kEipStatusOk == status) {
      status = GetDnsInfoFromResolvConf(&local_cfg);
    }
  }
  if(kEipStatusOk == status) {
    /* Free first and then making a shallow copy of local_cfg.domain_name is
     *  ok, because local_cfg goes out of scope now. */
    ClearCipString(&iface_cfg->domain_name);
    *iface_cfg = local_cfg;
  }
  return status;
}

/* For an API documentation look at opener_api.h. */
#define WAIT_CYCLE_NS   100000000U
EipStatus IfaceWaitForIp(const char *const iface,
                         int timeout,
                         volatile int *const p_abort_wait) {
  struct ifreq ifr;
  int rc;

  size_t if_name_len = strlen(iface);
  if(if_name_len < sizeof(ifr.ifr_name) ) {
    memcpy(ifr.ifr_name, iface, if_name_len);
    ifr.ifr_name[if_name_len] = 0;
  } else {
    errno = ENAMETOOLONG;
    OPENER_TRACE_INFO("interface name is too long\n");
    return kEipStatusError;
  }

  {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    uint32_t ipaddr;

    timeout *= 10; /* 100ms wait per nanosleep_simple32() */
    do {
      ipaddr = 0U;

      if(0 == (rc = ioctl(fd, SIOCGIFADDR, &ifr) ) ) {
        ipaddr = ( (struct sockaddr_in *) &ifr.ifr_addr )->sin_addr.s_addr;
      } else {
        if(EADDRNOTAVAIL != errno) {
          return rc;
        }
      }
      if(timeout > 0) {
        --timeout;
      }
    } while( (0 == ipaddr) && (0 != timeout) && (0 == *p_abort_wait) &&
             (0 == nanosleep_simple32(WAIT_CYCLE_NS) ) );

    OPENER_TRACE_INFO("ip=%08x, timeout=%d\n", ntohl(ipaddr), timeout);
    close(fd);
  }

  return rc;
}

void GetHostName(CipString *hostname) {
  char name_buf[HOST_NAME_MAX];

  int rc = gethostname(name_buf, sizeof name_buf);
  name_buf[HOST_NAME_MAX - 1] = '\0'; /* Ensure termination */
  if(0 == rc) {
    SetCipStringByCstr(hostname, name_buf);
  }
}
