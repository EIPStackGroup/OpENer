/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/

#ifndef PORTS_MINGW_SAMPLE_APPLICATION_ETHLINKCBS_H_
#define PORTS_MINGW_SAMPLE_APPLICATION_ETHLINKCBS_H_

/** @file
 * @brief Declaration of Ethernet Link object callbacks
 *
 * This header declares the Ethernet Link object callbacks. These callbacks
 *  handle the update and clear operation for the interface and media counters
 *  of every Ethernet Link object of our device.
 */

/*---------------------------------------------------------------------------*/
/*                               INCLUDES                                    */
/*---------------------------------------------------------------------------*/
#include "cip/ciptypes.h"
#include "core/typedefs.h"

/*---------------------------------------------------------------------------*/
/*                             PROTOTYPES                                    */
/*---------------------------------------------------------------------------*/

EipStatus EthLnkPreGetCallback(CipInstance* const instance,
                               CipAttributeStruct* const attribute,
                               CipByte service);

EipStatus EthLnkPostGetCallback(CipInstance* const instance,
                                CipAttributeStruct* const attribute,
                                CipByte service);

#endif  // PORTS_MINGW_SAMPLE_APPLICATION_ETHLINKCBS_H_
