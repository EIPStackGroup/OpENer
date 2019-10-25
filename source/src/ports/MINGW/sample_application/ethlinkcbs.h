/******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 *****************************************************************************/
#ifndef OPENER_ETHLINKCBS_H_
#define OPENER_ETHLINKCBS_H_
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
#include "typedefs.h"
#include "ciptypes.h"

/*---------------------------------------------------------------------------*/
/*                             PROTOTYPES                                    */
/*---------------------------------------------------------------------------*/

EipStatus EthLnkPreGetCallback
(
    CipInstance *const instance,
    CipAttributeStruct *const attribute,
    CipByte service
);

EipStatus EthLnkPostGetCallback
(
    CipInstance *const instance,
    CipAttributeStruct *const attribute,
    CipByte service
);


#endif  /* #ifndef OPENER_ETHLINKCBS_H_ */
