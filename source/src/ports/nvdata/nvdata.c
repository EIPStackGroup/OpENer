/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvdata.c
 * @brief This file implements common stuff to handle Non Volatile data.
 *
 * This module implements NvdataLoad(), a function to load all NV data of
 *  known common objects.
 * Also this module provides callback functions to store NV data of known
 *  objects when called by the EIP stack.
 */
#include "nvdata.h"

#include "trace.h"

/* Include headers of objects that need support for NV data here. */
#include "nvqos.h"
#include "nvtcpip.h"

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
  /* Load NV data for QoS object instance */
  EipStatus eip_status = NvQosLoad(&g_qos);
  if (kEipStatusError != eip_status) {
    eip_status =
      ( kEipStatusError == NvQosStore(&g_qos) ) ? kEipStatusError : eip_status;
  }

  return eip_status;
}

/** A PostSetCallback for QoS class to store NV attributes
 *
 * @param  instance  pointer to instance of QoS class
 * @param  attribute pointer to attribute structure
 * @param  service   the CIP service code of current request
 *
 *  @return kEipStatusOk: success; kEipStatusError: failure
 *
 * This function implements the PostSetCallback for the QoS class. The
 * purpose of this function is to save the NV attributes of the QoS
 * class instance to external storage.
 *
 * This application specific implementation chose to save all attributes
 * at once using a single NvQosStore() call.
 */
EipStatus NvQosSetCallback(CipInstance *const instance,
                           CipAttributeStruct *const attribute,
                           CipByte service) {
  /* Suppress unused parameter compiler warning. */
  (void)service;

  /* Suppress parameters used only for trace macros. */
#ifndef OPENER_WITH_TRACES
  (void)instance;
#endif /* OPENER_WITH_TRACES */

  EipStatus status = kEipStatusOk;

  if ( 0 != (kNvDataFunc & attribute->attribute_flags) ) {
    OPENER_TRACE_INFO("NV data update: %s, i %" PRIu32 ", a %" PRIu16 "\n",
                      instance->cip_class->class_name,
                      instance->instance_number,
                      attribute->attribute_number);
    status = NvQosStore(&g_qos);
  }
  return status;
}

/** A PostSetCallback for TCP/IP class to store NV attributes
 *
 * @param  instance  pointer to instance of TCP/IP class
 * @param  attribute pointer to attribute structure
 * @param  service   the CIP service code of current request
 *
 * This function implements the PostSetCallback for the TCP/IP class. The
 * purpose of this function is to save the NV attributes of the TCP/IP
 * class instance to external storage.
 *
 * This application specific implementation chose to save all attributes
 * at once using a single NvTcpipStore() call.
 */
EipStatus NvTcpipSetCallback(CipInstance *const instance,
                             CipAttributeStruct *const attribute,
                             CipByte service) {
  /* Suppress parameters used only for trace macros. */
#ifndef OPENER_WITH_TRACES
  (void)instance;
#endif /* OPENER_WITH_TRACES */

  EipStatus status = kEipStatusOk;

  if ( 0 != (kNvDataFunc & attribute->attribute_flags) ) {
    /* Workaround: Update only if service is not flagged. */
    if ( 0 == (0x80 & service) ) {
      OPENER_TRACE_INFO("NV data update: %s, i %" PRIu32 ", a %" PRIu16 "\n",
                        instance->cip_class->class_name,
                        instance->instance_number,
                        attribute->attribute_number);
      status = NvTcpipStore(&g_tcpip);
    }
  }
  return status;
}
