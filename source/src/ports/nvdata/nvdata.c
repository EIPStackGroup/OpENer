/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvdata.c
 *  @brief This file implements the functions to load all needed Non Volatile data.
 *
 */
#include "nvdata.h"

/* Include headers of objects that need support for NV data here. */
#include "nvqos.h"

/** @brief Load NV data for all object classes
 *
 *  @return kEipStatusOk on success, kEipStatusError if failure for any object occurred
 *
 * This function loads the NV data for each object class that supports NV data from
 *  external storage. If any of the load routines fails then for that object class
 *  the current object instance values are written as new NV data. That should be
 *  the default data.
 *
 * The load routines should be of the form
 *    int Nv<ObjClassName>Load(<ObjectInstanceDataType> *p_obj_instance);
 *  and return (-1) on failure and (0) on success.
 */
EipStatus NvdataLoad(void) {
  EipStatus status = kEipStatusOk;
  int       rc;

  /* Load NV data for QoS object instance */
  rc = NvQosLoad(&g_qos);
  status |= rc;
  if (0 != rc) {
    rc = NvQosStore(&g_qos);
    status |= rc;
  }

  return status;
}
