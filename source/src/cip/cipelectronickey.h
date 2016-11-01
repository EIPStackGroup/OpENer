/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPELECTRONICKEY_H_
#define SRC_CIP_CIPELECTRONICKEY_H_

#include <stdbool.h>

#include "typedefs.h"

typedef struct electronic_key_format_4 ElectronicKeyFormat4;

ElectronicKeyFormat4 *ElectronicKeyFormat4New();
void ElectronicKeyFormat4Delete(ElectronicKeyFormat4 **key);

void SetElectronicKeyFormat4VendorId(const CipUint vendor_id, ElectronicKeyFormat4 *const electronic_key);
CipUint GetElectronicKeyFormat4VendorId(const ElectronicKeyFormat4 *const electronic_key);

void SetElectronicKeyFormat4DeviceType(CipUint device_type, ElectronicKeyFormat4 *const electronic_key);
CipUint GetElectronicKeyFormat4DeviceType(const ElectronicKeyFormat4 *const electronic_key);

void SetElectronicKeyFormat4ProductCode(CipUint product_code, ElectronicKeyFormat4 *const electronic_key);
CipUint GetElectronicKeyFormat4ProductCode(const ElectronicKeyFormat4 *const electronic_key);

void SetElectronicKeyFormat4MajorRevisionCompatibility(CipByte major_revision_compatibility, ElectronicKeyFormat4 *const electronic_key);
//CipByte GetElectronicKeyFormat4MajorRevisionCompatibility(const ElectronicKeyFormat4 *const electronic_key);

CipByte GetElectronicKeyFormat4MajorRevision(const ElectronicKeyFormat4 *const electronic_key);
bool GetElectronicKeyFormat4Compatibility(const ElectronicKeyFormat4 *const electronic_key);

void SetElectronicKeyFormat4MinorRevision(CipUsint minor_revision, ElectronicKeyFormat4 *const electronic_key);
CipUsint GetElectronicKeyFormat4MinorRevision(const ElectronicKeyFormat4 *const electronic_key);

#endif /* SRC_CIP_CIPELECTRONICKEY_H_ */
