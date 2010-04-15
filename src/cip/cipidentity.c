/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include <string.h>
#include "opener_user_conf.h"
#include "cipidentity.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "opener_api.h"

/* attributes in CIP Identity Object */

EIP_UINT16 VendorID = OPENER_DEVICE_VENDOR_ID; /* #1 */
EIP_UINT16 DeviceType = OPENER_DEVICE_TYPE; /* #2 */
EIP_UINT16 ProductCode = OPENER_DEVICE_PRODUCT_CODE; /* #3 */
S_CIP_Revision Revison =
  { OPENER_DEVICE_MAJOR_REVISION, OPENER_DEVICE_MINOR_REVISION }; /* #4 */
EIP_UINT16 ID_Status = CIP_IDENTITY_STATE_OPERATIONAL; /* #5 TODO find out what this is and how it gets set */
EIP_UINT32 SerialNumber = 0; /* #6  Has to be set prior to OpENer initialization */
S_CIP_Short_String ProductName =
  { sizeof(OPENER_DEVICE_NAME) - 1, OPENER_DEVICE_NAME }; /* #7 */


void setDeviceSerialNumber(EIP_UINT32 pa_nSerialNumber)
  {
    SerialNumber = pa_nSerialNumber;
  }

static EIP_STATUS Reset(S_CIP_Instance *pa_pstInstance, /* pointer to instance*/
    S_CIP_MR_Request *pa_stMRRequest, /* pointer to message router request*/
    S_CIP_MR_Response *pa_stMRResponse, /* pointer to message router response*/
    EIP_UINT8 *pa_anMsg)
  {
    (void)pa_pstInstance;
    (void)pa_anMsg;

    pa_stMRResponse->ReplyService = (0x80 | pa_stMRRequest->Service);
    pa_stMRResponse->SizeofAdditionalStatus = 0;
    pa_stMRResponse->GeneralStatus = CIP_ERROR_SUCCESS;

    if (pa_stMRRequest->DataLength == 1)
      {
        switch (pa_stMRRequest->Data[0])
          {
        case 0: /*emulate device reset*/
          if (EIP_ERROR == IApp_ResetDevice())
            {
              pa_stMRResponse->GeneralStatus = CIP_ERROR_INVALID_PARAMETER;
            }
          break;

        case 1: /*reset to device settings*/
          if (EIP_ERROR == IApp_ResetDeviceToInitialConfiguration())
            {
              pa_stMRResponse->GeneralStatus = CIP_ERROR_INVALID_PARAMETER;
            }
          break;

        default:
          pa_stMRResponse->GeneralStatus = CIP_ERROR_INVALID_PARAMETER;
          break;
          }
      }
    else
      {
        /*The same behavior as if the data value given would be 0
          emulate device reset*/
        if (EIP_ERROR == IApp_ResetDevice())
          {
            pa_stMRResponse->GeneralStatus = CIP_ERROR_INVALID_PARAMETER;
          }
      }
    pa_stMRResponse->DataLength = 0;
    return EIP_OK;
  }

EIP_STATUS CIP_Identity_Init()
  {
    S_CIP_Class *pClass;
    S_CIP_Instance *pInstance;

    pClass = createCIPClass(CIP_IDENTITY_CLASS_CODE, 0, /* # of non-default class attributes*/
        MASK4(1, 2, 6, 7), /* class getAttributeAll mask		CIP spec 5-2.3.2*/
        0, /* # of class services*/
        7, /* # of instance attributes*/
        MASK7(1, 2, 3, 4, 5, 6, 7), /* instance getAttributeAll mask	CIP spec 5-2.3.2*/
        1, /* # of instance services*/
        1, /* # of instances*/
        "identity", /* class name (for debug)*/
        1); /* class revision*/

    if (pClass == 0)
      return EIP_ERROR;

    pInstance = getCIPInstance(pClass, 1);

    insertAttribute(pInstance, 1, CIP_UINT, &VendorID);
    insertAttribute(pInstance, 2, CIP_UINT, &DeviceType);
    insertAttribute(pInstance, 3, CIP_UINT, &ProductCode);
    insertAttribute(pInstance, 4, CIP_USINT_USINT, &Revison);
    insertAttribute(pInstance, 5, CIP_WORD, &ID_Status);
    insertAttribute(pInstance, 6, CIP_UDINT, &SerialNumber);
    insertAttribute(pInstance, 7, CIP_SHORT_STRING, &ProductName);

    insertService(pClass, CIP_RESET, &Reset, "Reset");

    return EIP_OK;
  }
