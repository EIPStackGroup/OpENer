/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPASSEMBLY_H_
#define OPENER_CIPASSEMBLY_H_

#include "typedefs.h"
#include "ciptypes.h"

/** @brief Assembly class code */
static const CipUint kCipAssemblyClassCode = 0x04U;

/* public functions */

/** @brief Setup the Assembly object
 *
 * Creates the Assembly Class with zero instances and sets up all services.
 *
 * @return Returns kEipStatusOk if assembly object was successfully created, otherwise kEipStatusError
 */
EipStatus CipAssemblyInitialize(void);

/** @brief clean up the data allocated in the assembly object instances
 *
 * Assembly object instances allocate per instance data to store attribute 3.
 * This will be freed here. The assembly object data given by the application
 * is not freed neither the assembly object instances. These are handled in the
 * main shutdown function.
 */
void ShutdownAssemblies(void);

/** @brief notify an Assembly object that data has been received for it.
 *
 *  The data will be copied into the assembly objects attribute 3 and
 *  the application will be informed with the AfterAssemblyDataReceived function.
 *
 *  @param instance the assembly object instance for which the data was received
 *  @param data pointer to the data received
 *  @param data_length number of bytes received
 *  @return
 *     - kEipStatusOk the received data was okay
 *     - kEipStatusError the received data was wrong
 */
EipStatus NotifyAssemblyConnectedDataReceived(CipInstance *const instance,
                                              const EipUint8 *const data,
                                              const EipUint16 data_length);

#endif /* OPENER_CIPASSEMBLY_H_ */
