/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#ifndef CIPASSEMBLY_H_
#define CIPASSEMBLY_H_

#include "typedefs.h"
#include "ciptypes.h"

#define CIP_ASSEMBLY_CLASS_CODE 0x04

/* public functions */

/*! Setup the Assembly object
 * 
 * Creates the Assembly Class with zero instances and sets up all services.
 */
EIP_STATUS CIP_Assembly_Init(void);

/*! notify an Assembly object that data has been received for it.
 * 
 *  The data will be copied into the assembly objects attribute 3 and
 *  the application will be informed with the IApp_after_assembly_data_received function.
 *  
 *  @param pa_pstInstance the assembly object instance for which the data was received
 *  @param pa_pnData pointer to the data received
 *  @param pa_nDatalength number of bytes received
 *  @return 
 *     - EIP_OK the received data was okay
 *     - EIP_ERROR the received data was wrong
 */ 
EIP_STATUS notifyAssemblyConnectedDataReceived(S_CIP_Instance *pa_pstInstance,
    EIP_UINT8 *pa_pnData, EIP_UINT16 pa_nDatalength);

#endif /*CIPASSEMBLY_H_*/
