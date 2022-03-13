/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include "ciptcpipinterface.h"

#include <string.h>

#include "opener_user_conf.h"
#include "cipcommon.h"
#include "cipconnectionobject.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "cipstring.h"
#include "endianconv.h"
#include "cipethernetlink.h"
#include "opener_api.h"
#include "trace.h"
#include "cipassembly.h"

/* Define constants to initialize the config_capability attribute (#2). These
 *   are needed as defines because we use them for static initialization. */
#define CFG_CAPS_BOOTP_CLIENT         0x01U /**< Device has BOOTP client */
#define CFG_CAPS_DNS_CLIENT           0x02U /**< Device has DNS client */
#define CFG_CAPS_DHCP_CLIENT          0x04U /**< Device has DHCP client */
#define CFG_CAPS_CFG_SETTABLE         0x10U /**< Interface configuration can be set */
#define CFG_CAPS_CFG_CHG_NEEDS_RESET  0x40U /**< Interface configuration change needs RESET */
#define CFG_CAPS_ACD_CAPABLE          0x80U /**< Device supports ACD */

/* OPENER_TCPIP_IFACE_CFG_SETTABLE controls if the interface configuration is fully settable.
 *   Prepare additional defines needed here:
 *   - IFACE_CFG_SET_MODE is used to initialize the set mode of the affected attributes (3, 5, 6).
 *   - CFG_CAPS is the matching initial value for .config_capability
 */
#if defined (OPENER_TCPIP_IFACE_CFG_SETTABLE) && \
  0 != OPENER_TCPIP_IFACE_CFG_SETTABLE
  #define IFACE_CFG_SET_MODE  kSetable
  #define CFG_CAPS  (CFG_CAPS_DHCP_CLIENT | CFG_CAPS_CFG_SETTABLE | \
                     CFG_CAPS_CFG_CHG_NEEDS_RESET)
#else
  #define IFACE_CFG_SET_MODE  kNotSetOrGetable
  #define CFG_CAPS  (CFG_CAPS_DHCP_CLIENT)
#endif

/** definition of TCP/IP object instance 1 data */
CipTcpIpObject g_tcpip =
{
  .status = 0x01, /* attribute #1 TCP status with 1 we indicate that we got a valid configuration from DHCP, BOOTP or NV data */
  .config_capability = CFG_CAPS, /* attribute #2 config_capability */
  .config_control = 0x02, /* attribute #3 config_control: 0x02 means that the device shall obtain its interface configuration values via DHCP. */
#if 2 != OPENER_ETHLINK_INSTANCE_CNT
  /* For the details where the physical_link_object path should point to, depending on the #
   *  of Ethernet Link objects refer to Vol. 2, Section 5-4.3.2.4 "Physical Link Object". */
  .physical_link_object = {     /* attribute #4 physical link object */
    2,  /* PathSize in 16 Bit chunks */
    CIP_ETHERNETLINK_CLASS_CODE,  /* Class Code */
    OPENER_ETHLINK_INSTANCE_CNT,  /* Instance # */
    0   /* Attribute # (not used as this is the EPATH to the EthernetLink object)*/
  },
#else
  .physical_link_object = {     /* attribute #4 physical link object */
    0,  /* PathSize in 16 Bit chunks */
    0,  /* Class Code */
    0,  /* Instance # */
    0   /* Attribute # */
  },
#endif  /* #if OPENER_ETHLINK_INSTANCE_CNT != 2 */
  .interface_configuration = { /* attribute #5 interface_configuration */
    0, /* IP address */
    0, /* NetworkMask */
    0, /* Gateway */
    0, /* NameServer */
    0, /* NameServer2 */
    { /* DomainName */
      0, NULL,
    }
  },
  .hostname = { /* attribute #6 hostname */
    0,
    NULL
  },
  .mcast_ttl_value = 1,  /* attribute #8 mcast TTL value */
  .mcast_config = {   /* attribute #9 multicast configuration */
    0,  /* use the default allocation algorithm */
    0,  /* reserved */
    1,  /* we currently use only one multicast address */
    0   /* the multicast address will be allocated on IP address configuration */
  },
  .select_acd = false,
  .encapsulation_inactivity_timeout = 120 /* attribute #13 encapsulation_inactivity_timeout, use a default value of 120 */
};

/************** Static Functions *********************************/

#if defined (OPENER_TCPIP_IFACE_CFG_SETTABLE) && \
  0 != OPENER_TCPIP_IFACE_CFG_SETTABLE
/** Check for pb being an alphanumerical character
 *
 * Is slow but avoids issues with the locale if we're NOT in the 'C' locale.
 */
static bool isalnum_c(const EipByte byte) {
  return
    ('a' <= byte && byte <= 'z') ||
    ('A' <= byte && byte <= 'Z') ||
    ('0' <= byte && byte <= '9');
}

/** Check passed string to conform to the rules for host name labels
 *
 *  @param  label pointer to the label string to check
 *  @return         true if label is valid
 *
 * A host name label is a string of length 1 to 63 characters with
 *  the characters of the string conforming to this rules:
 *  - 1st  character: [A-Za-z0-9]
 *  - next character: [-A-Za-z0-9]
 *  - last character: [A-Za-z0-9]
 *  The minimum length of 1 is checked but not the maximum length
 *  that has already been enforced on data reception.
 */
static bool IsValidNameLabel(const EipByte *label) {
  if (!isalnum_c(*label) ) {
    return false;
  }
  ++label;
  while ('\0' != *label && (isalnum_c(*label) || '-' == *label) ) {
    ++label;
  }
  return ('\0' == *label && '-' != label[-1]);
}

/** Check if domain is a valid domain
 *
 *  @param  p_domain  pointer to domain string to check
 *  @return           true if domain is valid
 *
 * We check here for domain names that are part of a valid host name.
 *  - Do not allow leading or trailing dots.
 *  - Also a single '.' (the root domain) is not allowed.
 *  - A complete numeric domain is accepted even if it should not.
 *  - IDN domain names are not supported. Any IDN domain names must
 *    be converted to punycode (see https://www.punycoder.com/) by
 *    the user in advance.
 */
static bool IsValidDomain(EipByte *domain) {
  bool status = true;

  OPENER_TRACE_INFO("Enter '%s'->", domain);
  if ('.' == *domain) { /* Forbid leading dot */
    return false;
  }
  EipByte *dot = (EipByte *)strchr( (char *)domain, '.' );
  if (dot) {
    bool rc;

    *dot = '\0';
    status &= rc = IsValidNameLabel(domain);
    OPENER_TRACE_INFO("Checked %d '%s'\n", rc, domain);
    if ('\0' != dot[1]) {
      status &= IsValidDomain(dot + 1);
    }
    else {  /* Forbid trailing dot */
      status = false;
    }
    *dot = '.';
  }
  else {
    status = IsValidNameLabel(domain);
    OPENER_TRACE_INFO("Check end %d '%s'\n", status, domain);
  }
  return status;
}


/** Check if an IP address is a valid network mask
 *
 *  @param  netmask network mask in network byte order
 *  @return         valid status
 *
 *  Check if it is a valid network mask pattern. The pattern 0xffffffff and
 *  0x00000000 are considered as invalid.
 */
static bool IsValidNetmask(in_addr_t netmask) {
  in_addr_t v = ntohl(netmask);

  v = ~v;   /* Create the host mask */
  ++v;      /* This must be a power of 2 then */
  bool valid = v && !(v & (v - 1) );  /* Check if it is a power of 2 */

  return valid && (INADDR_BROADCAST != netmask);
}

/** Check if an IP address is in one of the network classes A, B or C
 *
 *  @param  ip_addr IP address in network byte order
 *  @return         status
 *
 *  Check if the IP address belongs to the network classes A, B or C.
 */
static bool IsInClassAbc(in_addr_t ip_addr) {
  in_addr_t ip = ntohl(ip_addr);
  return IN_CLASSA(ip) || IN_CLASSB(ip) || IN_CLASSC(ip);
}

/** Check if an IP address is on the loopback network
 *
 *  @param  ip_addr IP address in network byte order
 *  @return         status
 *
 *  Check if the IP address belongs to the loopback network
 *  127.0.0.0 - 127.255.255.255.
 */
static bool IsOnLoopbackNetwork(in_addr_t ip_addr) {
  in_addr_t ip = ntohl(ip_addr);
  return (ip & IN_CLASSA_NET) == (INADDR_LOOPBACK & IN_CLASSA_NET);
}

/** Check if an IP address is either the network or the broadcast address
 *
 *  @param  ip_addr   IP address in network byte order
 *  @param  net_mask  network mask in network byte order
 *  @return           status
 *
 * Check if an IP address is either the network or the broadcast address.
 *  In this case it is not a valid IP address for a host.
 *  This check is endian agnostic.
 */
static bool IsNetworkOrBroadcastIp(in_addr_t ip_addr,
                                   in_addr_t net_mask) {
  return ( (ip_addr & net_mask) == ip_addr ) ||  /* is network address */
         ( (ip_addr | ~net_mask) == ip_addr );  /* is broadcast address */
}

/** Check the Interface configuration being valid according to EIP specification
 *
 * In Vol. 2 the "Table 5-4.3 Instance Attributes" provides some information
 *  which checks should be carried out on the Interface configuration's IP
 *  addresses. Also there are some hints in the
 *  Figure 5-4.1 "Diagram Showing the Behavior of the TCP/IP Object".
 *
 * The following checks may carried out on the IP addresses:
 *  -   N0: IP is not 0 aka. INADDR_ANY
 *  - MASK: IP is a valid network mask
 *  -  ABC: IP is in class A, B or C
 *  - NLCL: IP is not localhost aka. INADDR_LOOPBACK
 *  -   NB: IP is neither network or broadcast address (using network_mask)
 *
 * This is the table which checks are applied to what IP:
 *  N0 | MASK | ABC | NLCL | NB | IP address
 *   + |   -  |  +  |   +  |  + | ip_address
 *   - |   +  |  -  |   -  |  - | network_mask
 *   - |   -  |  +  |   +  |  + | gateway
 *   - |   -  |  +  |   -  |  - | name_server / name_server_2
 * A configured gateway must be reachable according to the network mask.
 */
static bool IsValidNetworkConfig(const CipTcpIpInterfaceConfiguration *if_cfg) {
  if (INADDR_ANY == ntohl(if_cfg->ip_address) ) {  /* N0 */
    return false;
  }
  if (INADDR_ANY != ntohl(if_cfg->network_mask) &&  /* MASK */
      !IsValidNetmask(if_cfg->network_mask) ) {
    return false;
  }
  if (!IsInClassAbc(if_cfg->ip_address) ||        /* ABC */
      !IsInClassAbc(if_cfg->gateway) ||
      !IsInClassAbc(if_cfg->name_server) ||
      !IsInClassAbc(if_cfg->name_server_2) ) {
    return false;
  }
  if (IsOnLoopbackNetwork(if_cfg->ip_address) ||  /* NLCL */
      IsOnLoopbackNetwork(if_cfg->gateway) ) {
    return false;
  }
  /* Check NB */
  if (IsNetworkOrBroadcastIp(if_cfg->ip_address, if_cfg->network_mask) ||
      (INADDR_ANY != ntohl(if_cfg->gateway) &&
       IsNetworkOrBroadcastIp(if_cfg->gateway, if_cfg->network_mask) ) ) {
    return false;
  }
  if (INADDR_ANY != ntohl(if_cfg->gateway) &&
      INADDR_ANY != ntohl(if_cfg->network_mask) ) {
    /* gateway is configured. Check if it is reachable. */
    if ( (if_cfg->network_mask & if_cfg->ip_address) !=
         (if_cfg->network_mask & if_cfg->gateway) ) {
      return false;
    }
  }
  return true;
}

static bool IsIOConnectionActive(void) {
  DoublyLinkedListNode *node = connection_list.first;

  while (NULL != node) {
    CipConnectionObject *connection = node->data;
    if (ConnectionObjectIsTypeIOConnection(connection) &&
        kConnectionObjectStateTimedOut !=
        ConnectionObjectGetState(connection) ) {
      /* An IO connection is present but is only considered active
       *  if it is NOT in timeout state. */
      return true;
    }
    node = node->next;
  }

  return false;
}
#endif /* defined (OPENER_TCPIP_IFACE_CFG_SETTABLE) && 0 != OPENER_TCPIP_IFACE_CFG_SETTABLE*/


static CipUsint dummy_data_field = 0; /**< dummy data fiel to provide non-null data pointers for attributes without data fields */

/************** Functions ****************************************/

void EncodeCipTcpIpInterfaceConfiguration(const void *const data,
                                           ENIPMessage *const outgoing_message)
{
  CipTcpIpInterfaceConfiguration *
    tcp_ip_network_interface_configuration =
    (CipTcpIpInterfaceConfiguration *) data;
  AddDintToMessage(ntohl(tcp_ip_network_interface_configuration->ip_address),
                   outgoing_message);
  AddDintToMessage(ntohl(tcp_ip_network_interface_configuration->network_mask),
                   outgoing_message);
  AddDintToMessage(ntohl(tcp_ip_network_interface_configuration->gateway),
                   outgoing_message);
  AddDintToMessage(ntohl(tcp_ip_network_interface_configuration->name_server),
                   outgoing_message);
  AddDintToMessage(ntohl(tcp_ip_network_interface_configuration->name_server_2),
                   outgoing_message);
  EncodeCipString(&(tcp_ip_network_interface_configuration->domain_name),
                  outgoing_message);
}

void EncodeCipTcpIpMulticastConfiguration(const void *const data,
                                          ENIPMessage *const outgoing_message) {
  /* Suppress unused parameter compiler warning. */
  (void)data;

  EncodeCipUsint(&(g_tcpip.mcast_config.alloc_control), outgoing_message);
  EncodeCipUsint(&(g_tcpip.mcast_config.reserved_shall_be_zero),
                 outgoing_message);
  EncodeCipUint(&(g_tcpip.mcast_config.number_of_allocated_multicast_addresses),
                outgoing_message);

  CipUdint multicast_address = ntohl(
    g_tcpip.mcast_config.starting_multicast_address);

  EncodeCipUdint(&multicast_address, outgoing_message);
}

void EncodeSafetyNetworkNumber(const void *const data,
                               ENIPMessage *const outgoing_message) {
  /* Suppress unused parameter compiler warning. */
  (void)data;

  FillNextNMessageOctetsWithValueAndMoveToNextPosition(0, 6, outgoing_message);
}

void EncodeCipLastConflictDetected(const void *const data,
                                   ENIPMessage *const outgoing_message) {
  /* Suppress unused parameter compiler warning. */
  (void)data;

  const size_t kAttribute11Size = sizeof(CipUsint) + 6 * sizeof(CipUsint) + 28 *
                                  sizeof(CipUsint);
  OPENER_ASSERT(kAttribute11Size == 35);
  FillNextNMessageOctetsWithValueAndMoveToNextPosition(0,
                                                       kAttribute11Size,
                                                       outgoing_message);
}


int DecodeTcpIpInterfaceConfigurationControl( /* Attribute 3 */
		void *const data,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	CipDword configuration_control_received = GetDintFromMessage(
			&(message_router_request->data));
	if ((configuration_control_received & kTcpipCfgCtrlMethodMask) >= 0x03
			|| (configuration_control_received & ~kTcpipCfgCtrlMethodMask)) {
		message_router_response->general_status =
				kCipErrorInvalidAttributeValue;

	} else {

		/* Set reserved bits to zero on reception. */
		configuration_control_received &= (kTcpipCfgCtrlMethodMask
				| kTcpipCfgCtrlDnsEnable);

		*(CipDword *)data = configuration_control_received;
		number_of_decoded_bytes = 4;
		message_router_response->general_status = kCipErrorSuccess;
	}

	return number_of_decoded_bytes;
}

#if defined (OPENER_TCPIP_IFACE_CFG_SETTABLE) && \
          0 != OPENER_TCPIP_IFACE_CFG_SETTABLE

int DecodeCipTcpIpInterfaceConfiguration( /* Attribute 5 */
		CipTcpIpInterfaceConfiguration *const data, //kCipUdintUdintUdintUdintUdintString
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	CipTcpIpInterfaceConfiguration if_cfg;
	CipUdint tmp_ip;

	if (IsIOConnectionActive()) {
		message_router_response->general_status = kCipErrorDeviceStateConflict;
		return number_of_decoded_bytes;
	}
	if (kTcpipCfgCtrlStaticIp
			!= (g_tcpip.config_control & kTcpipCfgCtrlMethodMask)) {
		message_router_response->general_status = kCipErrorObjectStateConflict;
		return number_of_decoded_bytes;
	}
	memset(&if_cfg, 0, sizeof if_cfg);
	tmp_ip = GetUdintFromMessage(&(message_router_request->data));
	if_cfg.ip_address = htonl(tmp_ip);
	tmp_ip = GetUdintFromMessage(&(message_router_request->data));
	if_cfg.network_mask = htonl(tmp_ip);
	tmp_ip = GetUdintFromMessage(&(message_router_request->data));
	if_cfg.gateway = htonl(tmp_ip);
	tmp_ip = GetUdintFromMessage(&(message_router_request->data));
	if_cfg.name_server = htonl(tmp_ip);
	tmp_ip = GetUdintFromMessage(&(message_router_request->data));
	if_cfg.name_server_2 = htonl(tmp_ip);

	CipUint domain_name_length = GetUintFromMessage(
			&(message_router_request->data));
	if (domain_name_length > 48) { /* see Vol. 2, Table 5-4.3 Instance Attributes */
		message_router_response->general_status = kCipErrorTooMuchData;
		return number_of_decoded_bytes;
	}
	SetCipStringByData(&if_cfg.domain_name, domain_name_length,
			message_router_request->data);
	domain_name_length = (domain_name_length + 1) & (~0x0001u); /* Align for possible pad byte */
	OPENER_TRACE_INFO("Domain: ds %hu '%s'\n",
			domain_name_length,
			if_cfg.domain_name.string);

	if (!IsValidNetworkConfig(&if_cfg)
			|| (domain_name_length > 0
					&& !IsValidDomain(if_cfg.domain_name.string))) {
		message_router_response->general_status =
				kCipErrorInvalidAttributeValue;
		return number_of_decoded_bytes;
	}

	*data = if_cfg; //write data to attribute
	number_of_decoded_bytes = 20 + domain_name_length;

	/* Tell that this configuration change becomes active after a reset */
	g_tcpip.status |= kTcpipStatusIfaceCfgPend;
	message_router_response->general_status = kCipErrorSuccess;

	return number_of_decoded_bytes;

}

int DecodeCipTcpIpInterfaceHostName( /* Attribute 6 */
		CipString *const data,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	          CipString tmp_host_name = {
	            .length = 0u,
	            .string = NULL
	          };
	          CipUint host_name_length =
	            GetUintFromMessage(&(message_router_request->data) );
	          if (host_name_length > 64) {  /* see RFC 1123 on more details */
	            message_router_response->general_status = kCipErrorTooMuchData;
	            return number_of_decoded_bytes;
	          }
	          SetCipStringByData(&tmp_host_name,
	                             host_name_length,
	                             message_router_request->data);
	          host_name_length = (host_name_length + 1) & (~0x0001u);  /* Align for possible pad byte */
	          OPENER_TRACE_INFO("Host Name: ds %hu '%s'\n",
	                            host_name_length,
	                            tmp_host_name.string);

	          if (!IsValidNameLabel(tmp_host_name.string) ) {
	            message_router_response->general_status =
	              kCipErrorInvalidAttributeValue;
	            return number_of_decoded_bytes;
	          }

	          *data = tmp_host_name; //write data to attribute

	          /* Tell that this configuration change becomes active after a reset */
	          g_tcpip.status |= kTcpipStatusIfaceCfgPend;
	          message_router_response->general_status = kCipErrorSuccess;

	return number_of_decoded_bytes;

}

#endif /* defined (OPENER_TCPIP_IFACE_CFG_SETTABLE) && 0 != OPENER_TCPIP_IFACE_CFG_SETTABLE*/

int DecodeCipTcpIpInterfaceEncapsulationInactivityTimeout( /* Attribute 13 */
		void *const data,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	CipUint inactivity_timeout_received = GetUintFromMessage(
			&(message_router_request->data));

	if (inactivity_timeout_received > 3600) {
		message_router_response->general_status =
				kCipErrorInvalidAttributeValue;
	} else {

		*(CipUint *)data = inactivity_timeout_received;
		message_router_response->general_status = kCipErrorSuccess;
		number_of_decoded_bytes = 2;

	}

	return number_of_decoded_bytes;

}


EipStatus CipTcpIpInterfaceInit() {
  CipClass *tcp_ip_class = NULL;

  if ( ( tcp_ip_class = CreateCipClass(kCipTcpIpInterfaceClassCode, /* class code */
                                       0, /* # class attributes */
                                       7, /* # highest class attribute number */
                                       2, /* # class services */
                                       13, /* # instance attributes */
                                       13, /* # highest instance attribute number */
                                       3, /* # instance services */
                                       1, /* # instances */
                                       "TCP/IP interface", 4, /* # class revision */
                                       NULL /* # function pointer for initialization */
                                       ) ) == 0 ) {
    return kEipStatusError;
  }

  CipInstance *instance = GetCipInstance(tcp_ip_class, 1); /* bind attributes to the instance #1 that was created above */

  InsertAttribute(instance,
                  1,
                  kCipDword,
                  EncodeCipDword,
                  NULL,
                  &g_tcpip.status,
                  kGetableSingleAndAll);
  InsertAttribute(instance,
                  2,
                  kCipDword,
                  EncodeCipDword,
                  NULL,
                  &g_tcpip.config_capability,
                  kGetableSingleAndAll);
  InsertAttribute(instance,
                  3,
                  kCipDword,
                  EncodeCipDword,
                  DecodeTcpIpInterfaceConfigurationControl,
                  &g_tcpip.config_control,
                  kSetAndGetAble | kNvDataFunc | IFACE_CFG_SET_MODE );
  InsertAttribute(instance,
                  4,
                  kCipEpath,
                  EncodeCipEPath,
                  NULL,
                  &g_tcpip.physical_link_object,
                  kGetableSingleAndAll);

#if defined (OPENER_TCPIP_IFACE_CFG_SETTABLE) && \
          0 != OPENER_TCPIP_IFACE_CFG_SETTABLE
  InsertAttribute(instance,
                  5,
                  kCipUdintUdintUdintUdintUdintString,
                  EncodeCipTcpIpInterfaceConfiguration,
                  DecodeCipTcpIpInterfaceConfiguration,
                  &g_tcpip.interface_configuration,
                  kGetableSingleAndAll | kNvDataFunc | IFACE_CFG_SET_MODE);
  InsertAttribute(instance,
                  6,
                  kCipString,
                  EncodeCipString,
                  DecodeCipTcpIpInterfaceHostName,
                  &g_tcpip.hostname,
                  kGetableSingleAndAll | kNvDataFunc | IFACE_CFG_SET_MODE);

#else
  InsertAttribute(instance,
                  5,
                  kCipUdintUdintUdintUdintUdintString,
                  EncodeCipTcpIpInterfaceConfiguration,
                  NULL, //not settable
                  &g_tcpip.interface_configuration,
                  kGetableSingleAndAll | kNvDataFunc | IFACE_CFG_SET_MODE);
    InsertAttribute(instance,
                  6,
                  kCipString,
                  EncodeCipString,
                  NULL, //not settable
                  &g_tcpip.hostname,
                  kGetableSingleAndAll | kNvDataFunc | IFACE_CFG_SET_MODE);

#endif /* defined (OPENER_TCPIP_IFACE_CFG_SETTABLE) && 0 != OPENER_TCPIP_IFACE_CFG_SETTABLE*/

  InsertAttribute(instance,
                  7,
                  kCipAny,
                  EncodeSafetyNetworkNumber,
                  NULL,
                  &dummy_data_field,
                  kGetableAllDummy);
  InsertAttribute(instance,
                  8,
                  kCipUsint,
                  EncodeCipUsint,
                  NULL,
                  &g_tcpip.mcast_ttl_value,
                  kGetableSingleAndAll);
  InsertAttribute(instance,
                  9,
                  kCipAny,
                  EncodeCipTcpIpMulticastConfiguration,
                  NULL,
                  &g_tcpip.mcast_config,
                  kGetableSingleAndAll);
  InsertAttribute(instance,
                  10,
                  kCipBool,
                  EncodeCipBool,
                  NULL,
                  &g_tcpip.select_acd,
                  kGetableAllDummy);
  InsertAttribute(instance,
                  11,
                  kCipBool,
                  EncodeCipLastConflictDetected,
                  NULL,
                  &dummy_data_field,
                  kGetableAllDummy);
  InsertAttribute(instance,
                  12,
                  kCipBool,
                  EncodeCipBool,
                  NULL,
                  &dummy_data_field, kGetableAllDummy);
  InsertAttribute(instance,
                  13,
                  kCipUint,
                  EncodeCipUint,
                  DecodeCipTcpIpInterfaceEncapsulationInactivityTimeout,
                  &g_tcpip.encapsulation_inactivity_timeout,
                  kSetAndGetAble | kNvDataFunc);

  InsertService(tcp_ip_class, kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle");

  InsertService(tcp_ip_class, kGetAttributeAll, &GetAttributeAll,
                "GetAttributeAll");

  InsertService(tcp_ip_class, kSetAttributeSingle,
                &SetAttributeSingle,
                "SetAttributeSingle");

  return kEipStatusOk;
}

void ShutdownTcpIpInterface(void) {
  /*Only free the resources if they are initialized */
  if (NULL != g_tcpip.hostname.string) {
    CipFree(g_tcpip.hostname.string);
    g_tcpip.hostname.string = NULL;
  }

  if (NULL != g_tcpip.interface_configuration.domain_name.string) {
    CipFree(g_tcpip.interface_configuration.domain_name.string);
    g_tcpip.interface_configuration.domain_name.string = NULL;
  }
}

/**
 *  This function calculates the multicast base address to be used for CIP
 *  connections from the current IP setting. The algorithm is implemented
 *  according to CIP spec Volume 2,
 *  section 3-5.3 "Multicast Address Allocation for EtherNet/IP"
 */
void CipTcpIpCalculateMulticastIp(CipTcpIpObject *const tcpip) {
  /* Multicast base address according to spec: 239.192.1.0 */
  static const CipUdint cip_mcast_base_addr = 0xEFC00100;

  /* Calculate the CIP multicast address. The multicast address is calculated, not input */
  CipUdint host_id = ntohl(tcpip->interface_configuration.ip_address) &
                     ~ntohl(tcpip->interface_configuration.network_mask);
  host_id -= 1;
  host_id &= 0x3ff;

  tcpip->mcast_config.starting_multicast_address =
    htonl(cip_mcast_base_addr + (host_id << 5) );
}


EipUint16 GetEncapsulationInactivityTimeout(CipInstance *instance) {
  CipAttributeStruct *attribute = GetCipAttribute(instance, 13);
  OPENER_ASSERT(NULL != attribute);
  CipUint *data = (CipUint *) attribute->data;
  EipUint16 encapsulation_inactivity_timeout = *data;
  return encapsulation_inactivity_timeout;
}

