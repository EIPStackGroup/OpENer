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

#define QOS_CFG_NAME  "qos.cfg"


/** @brief Load NV data of the QoS object from file
 *
 *  @param  p_qos pointer to the QoS object's data structure
 *  @return       0: success; -1: failure
 */
int NvQosLoad(CipQosObject *p_qos)
{
  CipQosObject  qos;
  FILE  *p_file;
  int   rd_cnt = 0;
  int   rc;

  rc = ConfFileOpen(false, QOS_CFG_NAME, &p_file);
  if (0 == rc) {
    /* Read input data */
    memset(&qos, 0x00, sizeof qos);
    rd_cnt = fscanf(p_file, " %" SCNu8 ", %" SCNu8 ", %" SCNu8 ", %" SCNu8 ", %" SCNu8 "\n",
                    &qos.dscp.urgent, &qos.dscp.scheduled, &qos.dscp.high,
                    &qos.dscp.low, &qos.dscp.explicit);

    /* Need to try to close all stuff in any case. */
    rc = ConfFileClose(&p_file);
  }
  if (0 == rc) {
    /* If all data were read copy them to the global QoS object. */
    if (5 == rd_cnt) {
      p_qos->dscp.urgent = qos.dscp.urgent;
      p_qos->dscp.scheduled = qos.dscp.scheduled;
      p_qos->dscp.high = qos.dscp.high;
      p_qos->dscp.low = qos.dscp.low;
      p_qos->dscp.explicit = qos.dscp.explicit;
    }
  }
  return rc;
}

/** @brief Store NV data of the QoS object to file
 *
 *  @param  p_qos pointer to the QoS object's data structure
 *  @return       0: success; -1: failure
 */
int NvQosStore(const CipQosObject *p_qos)
{
  FILE  *p_file;
  int   rc;

  rc = ConfFileOpen(true, QOS_CFG_NAME, &p_file);
  if (rc >= 0) {
    /* Print output data */
    rc = fprintf(p_file, " %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "\n",
                 p_qos->dscp.urgent, p_qos->dscp.scheduled, p_qos->dscp.high,
                 p_qos->dscp.low, p_qos->dscp.explicit);
    if (rc > 0) {
      rc = 0;
    }

    /* Need to try to close all stuff in any case. */
    rc |= ConfFileClose(&p_file);
  }
  return rc;
}
