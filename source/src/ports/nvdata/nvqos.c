/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/*
 * The following definition is needed for MinGW-w64 to enable C99 scanf
 * specifiers as required by fscanf() in NvQosLoad().
 */
#define __USE_MINGW_ANSI_STDIO 1


/** @file nvqos.c
 *  @brief This file implements the functions to handle QoS object's NV data.
 *
 *  This is only proof-of-concept code. Don't use it in a real product.
 *  Please think about atomic update of the external file or better parsing
 *  of the data on input.
 */
#include "nvqos.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "conffile.h"
#include "ciptypes.h"

#define QOS_CFG_NAME  "qos.cfg"


/** @brief Load NV data of the QoS object from file
 *
 *  @param  p_qos pointer to the QoS object's data structure
 *  @return kEipStatusOk: success; kEipStatusError: failure
 */
int NvQosLoad(CipQosObject *p_qos) {
  int rd_cnt = 0;
  EipStatus eip_status = kEipStatusError;

  CipUsint dscp_urgent = 0;
  CipUsint dscp_scheduled = 0;
  CipUsint dscp_high = 0;
  CipUsint dscp_low = 0;
  CipUsint dscp_explicit = 0;

  FILE  *p_file = ConfFileOpen(false, QOS_CFG_NAME);
  if (NULL != p_file) {

/* Disable VS fscanf depreciation warning. */
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif /* _MSC_VER */

    /* Read input data */
    rd_cnt = fscanf(p_file,
                    " %" SCNu8 ", %" SCNu8 ", %" SCNu8 ", %" SCNu8 ", %" SCNu8 "\n",
                    &dscp_urgent,
                    &dscp_scheduled,
                    &dscp_high,
                    &dscp_low,
                    &dscp_explicit);

/* Restore default depreciation warning behavior. */
#ifdef _MSC_VER
#pragma warning(default : 4996)
#endif /* _MSC_VER */

    /* Need to try to close all stuff in any case. */
    eip_status = ConfFileClose(&p_file);
  }
  if (kEipStatusOk == eip_status) {
    /* If all data were read copy them to the global QoS object. */
    if (5 == rd_cnt) {
      p_qos->dscp.urgent = dscp_urgent;
      p_qos->dscp.scheduled = dscp_scheduled;
      p_qos->dscp.high = dscp_high;
      p_qos->dscp.low = dscp_low;
      p_qos->dscp.explicit_msg = dscp_explicit;
    } else {
      eip_status = kEipStatusError;
    }
  }
  return eip_status;
}

/** @brief Store NV data of the QoS object to file
 *
 *  @param  p_qos pointer to the QoS object's data structure
 *  @return kEipStatusOk: success; kEipStatusError: failure
 */
EipStatus NvQosStore(const CipQosObject *p_qos) {
  FILE  *p_file = ConfFileOpen(true, QOS_CFG_NAME);
  EipStatus eip_status = kEipStatusOk;
  if (NULL != p_file) {
    /* Print output data */
    if (0 >= fprintf(p_file,
                     " %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8
                     "\n",
                     p_qos->dscp.urgent,
                     p_qos->dscp.scheduled,
                     p_qos->dscp.high,
                     p_qos->dscp.low,
                     p_qos->dscp.explicit_msg) ) {
      eip_status = kEipStatusError;
    }

    /* Need to try to close all stuff in any case. */
    eip_status =
      (kEipStatusError ==
       ConfFileClose(&p_file) ) ? kEipStatusError : eip_status;
  }
  return eip_status;
}
