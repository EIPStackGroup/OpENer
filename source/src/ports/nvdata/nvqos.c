/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

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
 *  @return       0: success; -1: failure
 */
int NvQosLoad(CipQosObject *p_qos) {
  int rd_cnt = 0;
  int rc;

  CipUsint dscp_urgent = 0;
  CipUsint dscp_scheduled = 0;
  CipUsint dscp_high = 0;
  CipUsint dscp_low = 0;
  CipUsint dscp_explicit = 0;

  FILE  *p_file = ConfFileOpen(false, QOS_CFG_NAME);
  if (NULL != p_file) {
    /* Read input data */
    rd_cnt = fscanf(p_file,
                    " %" SCNu8 ", %" SCNu8 ", %" SCNu8 ", %" SCNu8 ", %" SCNu8 "\n",
                    &dscp_urgent,
                    &dscp_scheduled,
                    &dscp_high,
                    &dscp_low,
                    &dscp_explicit);

    /* Need to try to close all stuff in any case. */
    rc = ConfFileClose(&p_file);
  }
  if (0 == rc) {
    /* If all data were read copy them to the global QoS object. */
    if (5 == rd_cnt) {
      p_qos->dscp.urgent = dscp_urgent;
      p_qos->dscp.scheduled = dscp_scheduled;
      p_qos->dscp.high = dscp_high;
      p_qos->dscp.low = dscp_low;
      p_qos->dscp.explicit = dscp_explicit;
    }
  }
  return rc;
}

/** @brief Store NV data of the QoS object to file
 *
 *  @param  p_qos pointer to the QoS object's data structure
 *  @return       0: success; -1: failure
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
                     p_qos->dscp.explicit) ) {
      eip_status = kEipStatusError;
    }

    /* Need to try to close all stuff in any case. */
    eip_status = kEipStatusError ==
                 ConfFileClose(&p_file) ? kEipStatusError : eip_status;
  }
  return eip_status;
}
