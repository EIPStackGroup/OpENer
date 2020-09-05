/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPETHERNETLINK_H_
#define OPENER_CIPETHERNETLINK_H_
/** @file
 *  @brief Declare public interface of the CIP Ethernet Link Object
 */

#include "typedefs.h"
#include "ciptypes.h"

/** @brief This Ethernet Link class code as #define is still needed for a static
 *  initialization. */
#define CIP_ETHERNETLINK_CLASS_CODE   0xF6U
/** @brief Ethernet Link class code */
static const CipUint kCipEthernetLinkClassCode = CIP_ETHERNETLINK_CLASS_CODE;

/* public type definitions */

/** @brief Provide values for the Interface Flags (attribute #2) */
typedef enum {
  /** Set this bit if your device needs a reset to take over new settings made via
   * attribute #6. It is duplicates the meaning of kEthLinkCapManualReset */
  kEthLinkFlagsManualReset = 0x20,
} CipEthLinkIfaceFlags;

/** @brief Provide values for the Interface Control (attribute #6) Control bits member */
typedef enum {
  /** Set this bit to enable Auto-negotiation of ethernet link parameters. */
  kEthLinkIfCntrlAutonegotiate  = 0x01,
  /** When Auto-negotiation is disabled set this bit to force Full-Duplex mode else
   *  Half-Duplex mode is forced. */
  kEthLinkIfCntrlForceDuplexFD = 0x02,
  /** For convenience declare the sum of valid bits as the maximum allowed value. */
  kEthLinkIfCntrlMaxValid = kEthLinkIfCntrlAutonegotiate +
                            kEthLinkIfCntrlForceDuplexFD,
} CipEthLinkIfaceControl;

/** @brief Provide values for the Interface Type (attribute #7) */
typedef enum {
  /** Unknown interface type */
  kEthLinkIfTypeUnknown = 0x00,
  /** Internal (switch) port */
  kEthLinkIfTypeInternal = 0x01,
  /** Twisted pair (e.g., 10Base-T, 100Base-TX, 1000Base-T, etc.) */
  kEthLinkIfTypeTwistedPair = 0x02,
  /** Optical fiber (e.g., 100Base-FX) */
  kEthLinkIfTypeOptical = 0x03,
} CipEthLinkIfaceType;

/** @brief Provide bit masks for the Interface Capability (#11) attribute's Capability Bits */
typedef enum {
  /** Interface needs reset to activate attribute #6 */
  kEthLinkCapManualReset = 0x01,
  /** Interface supports link auto-negotiation */
  kEthLinkCapAutoNeg = 0x02,
  /** Interface supports link auto-crossover */
  kEthLinkCapAutoMDX = 0x04,
  /** Interface supports setting of Interface Control attribute(#6) */
  kEthLinkCapManualSpeed = 0x08,
} CipEthLinkCapabilityBits;


/** @brief Provide bit masks to select available speed / duplex combinations
 *
 *  Keep the bit index of these bit masks in sync with the array index of the
 *  matching speed / duplex structure in the internal @p speed_duplex_table
 *  of cipethernetlink.c
 */
typedef enum {
  kEthLinkSpeedDpx_10_HD = 0x01,
  kEthLinkSpeedDpx_10_FD = 0x02,
  kEthLinkSpeedDpx_100_HD = 0x04,
  kEthLinkSpeedDpx_100_FD = 0x08,
  kEthLinkSpeedDpx_1000_HD = 0x10,
  kEthLinkSpeedDpx_1000_FD = 0x20,
} CipEthLinkSpeedDpxSelect;


/** @brief Type definition to describe the Interface Capability
 *
 *  This structure is not a direct representation of the Interface Capability
 *  attribute (#11) but replaces the needed array of speed / duplex list entries
 *  by @ref speed_duplex_selector to create the needed array on the fly.
 */
typedef struct {
  /** Capability flags of CipEthLinkCapabilityBits group */
  CipDword capability_bits;
  /** Speed / duplex selector bit map of CipEthLinkSpeedDpxSelect */
  uint16_t speed_duplex_selector;
} CipEthernetLinkMetaInterfaceCapability;



#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
/** @brief Type definition of the Interface Counters attribute #4
 *
 * This union holds the 32-bit Interface Counters of the Ethernet Link object.
 *  This is attribute is becomes required if the HC Interface Counters attribute or
 *  Media Counters attribute is implemented, otherwise highly recommended.
 * That means for DLR capable devices this attribute is required because the
 *  Media Counters attribute is required for DLR capable devices.
 */
typedef union {
  CipUdint cntr32[11];
  struct {
    CipUdint in_octets;
    CipUdint in_ucast;
    CipUdint in_nucast;
    CipUdint in_discards;
    CipUdint in_errors;
    CipUdint in_unknown_protos;
    CipUdint out_octets;
    CipUdint out_ucast;
    CipUdint out_nucast;
    CipUdint out_discards;
    CipUdint out_errors;
  } ul;
} CipEthernetLinkInterfaceCounters;

/** @brief Type definition of the Media Counters attribute #5
 *
 * This union holds the 32-bit Media Counters of the Ethernet Link object.
 *  This attribute becomes required if the devices supports DLR or if the
 *  HC Media Counters attribute is implemented, otherwise highly recommended.
 */
typedef union {
  CipUdint cntr32[12];
  struct {
    CipUdint align_errs;
    CipUdint fcs_errs;
    CipUdint single_coll;
    CipUdint multi_coll;
    CipUdint sqe_test_errs;
    CipUdint def_trans;
    CipUdint late_coll;
    CipUdint exc_coll;
    CipUdint mac_tx_errs;
    CipUdint crs_errs;
    CipUdint frame_too_long;
    CipUdint mac_rx_errs;
  } ul;
} CipEthernetLinkMediaCounters;
#endif  /* ... && OPENER_ETHLINK_CNTRS_ENABLE != 0 */

/** @brief Type definition of the Interface Control attribute (#6)
 *
 */
typedef struct {
  CipWord control_bits;
  CipUint forced_interface_speed;
} CipEthernetLinkInterfaceControl;

/** @brief Data of an CIP Ethernet Link object */
typedef struct {
  EipUint32 interface_speed; /**< Attribute #1: 10/100/1000 Mbit/sec */
  EipUint32 interface_flags; /**< Attribute #2: Interface flags as defined in the CIP specification */
  EipUint8 physical_address[6]; /**< Attribute #3: MAC address of the Ethernet link */
#if defined(OPENER_ETHLINK_CNTRS_ENABLE) && 0 != OPENER_ETHLINK_CNTRS_ENABLE
  CipEthernetLinkInterfaceCounters interface_cntrs; /**< Attribute #4: Interface counters 32-bit wide */
  CipEthernetLinkMediaCounters media_cntrs; /**< Attribute #5: Media counters 32-bit wide */
#endif
#if defined(OPENER_ETHLINK_IFACE_CTRL_ENABLE) && \
  0 != OPENER_ETHLINK_IFACE_CTRL_ENABLE
  CipEthernetLinkInterfaceControl interface_control;  /** Attribute #6: control link properties */
#endif
  CipUsint interface_type;  /**< Attribute #7: Type of interface */
  CipShortString interface_label; /**< Attribute #10: Interface label */
  CipEthernetLinkMetaInterfaceCapability interface_caps; /**< Attribute #11: Interface capabilities */
} CipEthernetLinkObject;


/* public functions */
/** @brief Initialize the Ethernet Link Objects data
 *
 *  @return kEipStatusOk if initialization was successful, otherwise kEipStatusError
 */
EipStatus CipEthernetLinkInit(void);

/** @brief Initialize the Ethernet MAC of the Ethernet Link object instances
 *
 *  @param  p_physical_address pointer to 6 bytes of MAC address
 *
 * This function sets the MAC address of all involved Ethernet Link objects.
 */
void CipEthernetLinkSetMac(EipUint8 *p_physical_address);


/* global object instance(s) */

extern CipEthernetLinkObject g_ethernet_link[];

#endif /* OPENER_CIPETHERNETLINK_H_*/
