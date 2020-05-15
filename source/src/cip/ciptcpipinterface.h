/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPTCPIPINTERFACE_H_
#define OPENER_CIPTCPIPINTERFACE_H_

/** @file ciptcpipinterface.h
 * @brief Public interface of the TCP/IP Interface Object
 *
 */

#include "typedefs.h"
#include "ciptypes.h"

/** @brief TCP/IP Interface class code */
static const CipUint kCipTcpIpInterfaceClassCode = 0xF5U;

/* Declare constants for status attribute (#1) */
/** Indicates a pending configuration change in the TTL Value and/or Mcast Config attributes.*/
static const CipDword kTcpipStatusMcastPend = 0x10U;
/** Indicates a pending configuration change in the Interface Configuration attribute. */
static const CipDword kTcpipStatusIfaceCfgPend = 0x20U;
/** Indicates when an IP address conflict has been detected by ACD. */
static const CipDword kTcpipStatusAcdStatus = 0x40U;
/** Indicates when an IP address conflict has been detected by ACD or the defense failed. */
static const CipDword kTcpipStatusAcdFault = 0x80U;

/* Declare constants for config_control attribute (#3) */
static const CipDword kTcpipCfgCtrlStaticIp   = 0x00U;  /**< IP configuration method is manual IP assignment */
static const CipDword kTcpipCfgCtrlBootp      = 0x01U;  /**< IP configuration method is BOOTP */
static const CipDword kTcpipCfgCtrlDhcp       = 0x02U;  /**< IP configuration method is DHCP */
static const CipDword kTcpipCfgCtrlMethodMask = 0x0FU;  /**< bit mask for the method field */
static const CipDword kTcpipCfgCtrlDnsEnable  = 0x10U;  /**< enables DNS resolution on originator devices */

/** @brief Multicast Configuration struct, called Mcast config
 *
 */
typedef struct multicast_address_configuration {
  CipUsint alloc_control; /**< 0 for default multicast address generation algorithm; 1 for multicast addresses according to Num MCast and MCast Start Addr */
  CipUsint reserved_shall_be_zero; /**< shall be zero */
  CipUint number_of_allocated_multicast_addresses; /**< Number of IP multicast addresses allocated */
  CipUdint starting_multicast_address; /**< Starting multicast address from which Num Mcast addresses are allocated */
} MulticastAddressConfiguration;

/** @brief Declaration of the TCP/IP object's structure type
 */
typedef struct {
  CipDword status;            /**< attribute #1  TCP status */
  CipDword config_capability; /**< attribute #2 bitmap of capability flags */
  CipDword config_control;    /**< attribute #3 bitmap: control the interface configuration method: static / BOOTP / DHCP */
  CipEpath physical_link_object;  /**< attribute #4 references the Ethernet Link object for this  interface */
  CipTcpIpInterfaceConfiguration interface_configuration;/**< attribute #5 IP, network mask, gateway, name server 1 & 2, domain name*/
  CipString hostname; /**< #6 host name*/
  CipUsint mcast_ttl_value; /**< #8 the time to live value to be used for multi-cast connections */

  /** #9 The multicast configuration for this device */
  MulticastAddressConfiguration mcast_config;
  CipBool select_acd; /**< attribute #10 - Is ACD enabled? */

  /** #13 Number of seconds of inactivity before TCP connection is closed */
  CipUint encapsulation_inactivity_timeout;
} CipTcpIpObject;


/* global public variables */
extern CipTcpIpObject g_tcpip;  /**< declaration of TCP/IP object instance 1 data */

/* public functions */
/** @brief Initializing the data structures of the TCP/IP interface object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipTcpIpInterfaceInit(void);

/** @brief Clean up the allocated data of the TCP/IP interface object.
 *
 * Currently this is the host name string and the domain name string.
 *
 */
void ShutdownTcpIpInterface(void);

/** @brief Calculate Multicast address base from current IP setting
 *
 *  @param  tcpip pointer to TCP/IP object
 */
void CipTcpIpCalculateMulticastIp(CipTcpIpObject *const tcpip);

/** @brief Public Method to get Encapsulation Inactivity Timeout Value
 *
 *
 */
EipUint16 GetEncapsulationInactivityTimeout(CipInstance *instance);

#endif /* OPENER_CIPTCPIPINTERFACE_H_ */
