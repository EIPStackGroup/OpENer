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

// #include <inttypes.h>
// #include <stdio.h>
#include <string.h>

#include "trace.h"

#include "conffile.h"

#define TCPIP_CFG_NAME  "tcpip.cfg" /**< Name of the configuration file */


/** @brief Load NV data of the TCP/IP object from file
 *
 *  @param  p_qos pointer to the TCP/IP object's data structure
 *  @return       0: success; -1: failure
 */
int NvTcpipLoad(CipTcpIpObject *p_tcp_ip) {
  CipTcpIpObject tcpip;
  FILE  *p_file;
  int rc;

  memset(&tcpip, 0, sizeof tcpip);
  rc = ConfFileOpen(false, TCPIP_CFG_NAME, &p_file);
  if (0 == rc) {
    /* Read input data */
    OPENER_TRACE_ERR(
      "ERROR: Loading of TCP/IP object's NV data not implemented yet\n");
    /* TODO: Implement load */
    rc = kEipStatusError;

    /* Need to try to close all stuff in any case. */
    rc = ConfFileClose(&p_file);
  }
  if (0 == rc) {
    /* If all NV attributes were read copy them attribute by attribute
     * to the caller's TCP/IP object. */
    /* TODO: copy all NV attributes */
  }
  return rc;
}

/** @brief Store NV data of the TCP/IP object to file
 *
 *  @param  p_qos pointer to the TCP/IP object's data structure
 *  @return       0: success; -1: failure
 */
int NvTcpipStore(const CipTcpIpObject *p_tcp_ip) {
  FILE  *p_file;
  int rc;

  rc = ConfFileOpen(true, TCPIP_CFG_NAME, &p_file);
  if (rc >= 0) {
    /* Print output data */
    OPENER_TRACE_ERR(
      "ERROR: Storing of TCP/IP object's NV data not implemented yet\n");
    /* TODO: Implement store */
    rc = kEipStatusError;

    /* Need to try to close all stuff in any case. */
    rc |= ConfFileClose(&p_file);
  }
  return rc;
}
