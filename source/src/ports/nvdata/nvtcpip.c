/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvtcpip.c
 *  @brief This file implements the functions to handle TCP/IP object's NV data.
 *
 *  This is only a code skeleton. The real load and store operation is NOT
 *  implemented.
 */
#include "nvtcpip.h"

#include <string.h>

#include "trace.h"
#include "conffile.h"

#define TCPIP_CFG_NAME  "tcpip.cfg" /**< Name of the configuration file */


/** @brief Load NV data of the TCP/IP object from file
 *
 *  @param  p_tcp_ip pointer to the TCP/IP object's data structure
 *  @return kEipStatusOk: success; kEipStatusError: failure
 */
int NvTcpipLoad(CipTcpIpObject *p_tcp_ip) {
  /* Suppress unused parameter compiler warning. */
  (void)p_tcp_ip;

  EipStatus eip_status = kEipStatusOk;

  FILE *p_file = ConfFileOpen(false, TCPIP_CFG_NAME);
  if (NULL != p_file) {
    /* Read input data */
    OPENER_TRACE_ERR(
      "ERROR: Loading of TCP/IP object's NV data not implemented yet\n");
    /* TODO: Implement load */
    eip_status = kEipStatusError;
    /* If all NV attributes were read copy them attribute by attribute
     * to the caller's TCP/IP object. */
    /* TODO: copy all NV attributes */

    /* Need to try to close all stuff in any case. */
    eip_status =
      (kEipStatusError ==
       ConfFileClose(&p_file) ) ? kEipStatusError : eip_status;
  }

  return eip_status;
}

/** @brief Store NV data of the TCP/IP object to file
 *
 *  @param  p_tcp_ip pointer to the TCP/IP object's data structure
 *  @return kEipStatusOk: success; kEipStatusError: failure
 */
EipStatus NvTcpipStore(const CipTcpIpObject *p_tcp_ip) {
  /* Suppress unused parameter compiler warning. */
  (void)p_tcp_ip;

  FILE *p_file = ConfFileOpen(true, TCPIP_CFG_NAME);
  if (NULL != p_file) {
    /* Print output data */
    OPENER_TRACE_ERR(
      "ERROR: Storing of TCP/IP object's NV data not implemented yet\n");
    /* TODO: Implement store */
    EipStatus eip_status = kEipStatusError;

    /* Need to try to close all stuff in any case. */
    return (kEipStatusError ==
            ConfFileClose(&p_file) ) ? kEipStatusError : eip_status;
  } else {
    return kEipStatusError; /* File could not be openend*/
  }
}
