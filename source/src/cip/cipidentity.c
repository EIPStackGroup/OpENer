/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/**
 * @file cipidentity.c
 *
 * CIP Identity Object
 * ===================
 *
 * Implemented Attributes
 * ----------------------
 * - Attribute 1: VendorID
 * - Attribute 2: Device Type
 * - Attribute 3: Product Code
 * - Attribute 4: Revision
 * - Attribute 5: Status
 * - Attribute 6: Serial Number
 * - Attribute 7: Product Name
 * - Attribute 8: State
 *
 * Implemented Services
 * --------------------
 */

#include "cipidentity.h"

#include <string.h>

#include "opener_user_conf.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "ciperror.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"


/** @brief Definition of the global Identity Object */
CipIdentityObject g_identity =
{
  .vendor_id = OPENER_DEVICE_VENDOR_ID, /* Attribute 1: Vendor ID */
  .device_type = OPENER_DEVICE_TYPE, /* Attribute 2: Device Type */
  .product_code = OPENER_DEVICE_PRODUCT_CODE, /* Attribute 3: Product Code */
  .revision = { /* Attribute 4: Revision / CipUsint Major, CipUsint Minor */
    OPENER_DEVICE_MAJOR_REVISION,
    OPENER_DEVICE_MINOR_REVISION
  },
  .status = 0, /* Attribute 5: Status */
  .serial_number = 0, /* Attribute 6: Serial Number */
  .product_name = { /* Attribute 7: Product Name */
    sizeof(OPENER_DEVICE_NAME) - 1,
    (EipByte *)OPENER_DEVICE_NAME
  },
  .state = 255,
};


/** Private functions, sets the devices serial number
 * @param serial_number The serial number of the device
 */
void SetDeviceSerialNumber(const EipUint32 serial_number) {
  g_identity.serial_number = serial_number;
}

/** @brief Private function, sets the devices status
 * @param status The serial number of the device
 */
void SetDeviceStatus(const EipUint16 status) {
  g_identity.status = status;
}

/** @brief Reset service
 *
 * @param instance
 * @param message_router_request
 * @param message_router_response
 * @returns Currently always kEipOkSend is returned
 */
static EipStatus Reset(CipInstance *instance,
/* pointer to instance*/
                       CipMessageRouterRequest *message_router_request,
                       /* pointer to message router request*/
                       CipMessageRouterResponse *message_router_response,  /* pointer to message router response*/
                       const struct sockaddr *originator_address,
                       const int encapsulation_session) {
  (void) instance;

  EipStatus eip_status = kEipStatusOkSend;

  message_router_response->reply_service = (0x80
                                            | message_router_request->service);
  message_router_response->size_of_additional_status = 0;
  message_router_response->general_status = kCipErrorSuccess;

  if (message_router_request->request_path_size == 1) {
    switch (message_router_request->data[0]) {
      case 0: /* Reset type 0 -> emulate device reset / Power cycle */
        if ( kEipStatusError == ResetDevice() ) {
          message_router_response->general_status = kCipErrorInvalidParameter;
        }
        break;

      case 1: /* Reset type 1 -> reset to device settings */
        if ( kEipStatusError == ResetDeviceToInitialConfiguration() ) {
          message_router_response->general_status = kCipErrorInvalidParameter;
        }
        break;

      /* case 2: Not supported Reset type 2 -> Return to factory defaults except communications parameters */

      default:
        message_router_response->general_status = kCipErrorInvalidParameter;
        break;
    }
  } else /*TODO: Should be if (pa_stMRRequest->DataLength == 0)*/
  {
    /* The same behavior as if the data value given would be 0
       emulate device reset */

    if ( kEipStatusError == ResetDevice() ) {
      message_router_response->general_status = kCipErrorInvalidParameter;
    } else {
      /* eip_status = EIP_OK; */
    }
  }
  message_router_response->data_length = 0;
  return eip_status;
}

void InitializeCipIdentity(CipClass *class) {


  CipClass *meta_class = class->class_instance.cip_class;

  InsertAttribute( (CipInstance *) class, 1, kCipUint,
                   (void *) &class->revision,
                   kGetableSingleAndAll );                 /* revision */
  InsertAttribute( (CipInstance *) class, 2, kCipUint,
                   (void *) &class->number_of_instances, kGetableSingleAndAll ); /*  largest instance number */
  InsertAttribute( (CipInstance *) class, 3, kCipUint,
                   (void *) &class->number_of_instances, kGetAttributeSingle ); /* number of instances currently existing*/
  InsertAttribute( (CipInstance *) class, 4, kCipUint, (void *) &kCipUintZero,
                   kNotSetOrGetable ); /* optional attribute list - default = 0 */
  InsertAttribute( (CipInstance *) class, 5, kCipUint, (void *) &kCipUintZero,
                   kNotSetOrGetable ); /* optional service list - default = 0 */
  InsertAttribute( (CipInstance *) class, 6, kCipUint,
                   (void *) &meta_class->highest_attribute_number,
                   kGetableSingleAndAll );                 /* max class attribute number*/
  InsertAttribute( (CipInstance *) class, 7, kCipUint,
                   (void *) &class->highest_attribute_number,
                   kGetableSingleAndAll );                 /* max instance attribute number*/

}

EipStatus CipIdentityInit() {

  CipClass *class = CreateCipClass(kCipIdentityClassCode,
                                   0, /* # of non-default class attributes */
                                   7, /* # highest class attribute number*/
                                   2, /* # of class services*/
                                   7, /* # of instance attributes*/
                                   7, /* # highest instance attribute number*/
                                   3, /* # of instance services*/
                                   1, /* # of instances*/
                                   "identity", /* # class name (for debug)*/
                                   1, /* # class revision*/
                                   &InitializeCipIdentity); /* # function pointer for initialization*/

  if (class == 0) {
    return kEipStatusError;
  }

  CipInstance *instance = GetCipInstance(class, 1);
  InsertAttribute(instance,
                  1,
                  kCipUint,
                  &g_identity.vendor_id,
                  kGetableSingleAndAll);
  InsertAttribute(instance,
                  2,
                  kCipUint,
                  &g_identity.device_type,
                  kGetableSingleAndAll);
  InsertAttribute(instance,
                  3,
                  kCipUint,
                  &g_identity.product_code,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 4, kCipUsintUsint, &g_identity.revision,
                  kGetableSingleAndAll);
  InsertAttribute(instance,
                  5,
                  kCipWord,
                  &g_identity.status,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 6, kCipUdint, &g_identity.serial_number,
                  kGetableSingleAndAll);
  InsertAttribute(instance, 7, kCipShortString, &g_identity.product_name,
                  kGetableSingleAndAll);
  InsertService(class, kGetAttributeSingle, &GetAttributeSingle,
                "GetAttributeSingle");
  InsertService(class, kGetAttributeAll, &GetAttributeAll, "GetAttributeAll");
  InsertService(class, kReset, &Reset, "Reset");

  return kEipStatusOk;
}

void CipIdentitySetExtendedDeviceStatus(
  CipIdentityExtendedStatus extended_status) {
  OPENER_TRACE_INFO("Setting extended status: %x\n", extended_status);
  g_identity.status &= ~(0x70);
  g_identity.status |= extended_status;
}
