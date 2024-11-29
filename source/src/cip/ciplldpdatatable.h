/*******************************************************************************
 * Copyright (c) 2024, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#pragma once

/** @file ciplldpdatatable.h
 *  @brief Public interface of the LLDP Data Table Object
 *
 */

#include "ciptypes.h"
#include "typedefs.h"

/** @brief LLDP Data Table Object class code */
static const CipUint kCipLldpDataTableCode = 0x10AU;

typedef struct cip_lldp_data_table_system_capabilites_tlv {
  CipWord system_capabilities;
  CipWord enabled_capabilities;
} CipLldpDataTableSystemCapabilitesTlv;

typedef struct cip_lldp_data_table_ipv4_management_addresses {
  CipUsint management_address_count;
  CipUdint *management_addresses;
} CipLldpDataTableIpv4ManagementAddresses;

typedef struct cip_lldp_data_table_cip_identification {
  CipUint vendor_id;
  CipUint device_type;
  CipUint product_code;
  CipByte major_revision;
  CipUsint minor_revision;
  CipUdint cip_serial_number
} CipLldpDataTableCipIdentification;

typedef struct cip_lldp_data_table_additional_ethernet_capabilities {
  CipBool preemption_support;
  CipBool preemption_status;
  CipBool preemption_active;
  CipUsint additional_fragment_size;
} CipLldpDataTableAdditionalEthernetCapabilities;

typedef struct cip_lldp_data_table_t1s_phy_configuration {
  CipUsint phy_mode;
  CipUsint phy_node_id_for_pcla;
} CipLldpDataTableT1sPhyConfiguration;

typedef struct cip_lldp_data_table_object_values {
  CipUint ethernet_link_instance_number;
  CipOctet *mac_address;
  CipShortString interface_label;
  CipUint time_to_live;
  CipLldpDataTableSystemCapabilitesTlv system_capabilities_tlv;
  CipLldpDataTableIpv4ManagementAddresses ipv4_management_addresses;
  CipLldpDataTableCipIdentification cip_identification;
  CipLldpDataTableAdditionalEthernetCapabilities
    additional_ethernet_capabilities;
  CipUdint last_change;
  CipUint position_id;
  CipLldpDataTableT1sPhyConfiguration t1s_phy_configuration;

} CipLldpDataTableObjectValues;