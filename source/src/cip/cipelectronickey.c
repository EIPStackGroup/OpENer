 /*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "cipelectronickey.h"

typedef struct electronic_key_format_4 {
  CipUint vendor_id;
  CipUint device_type;
  CipUint product_code;
  CipByte major_revision_compatibility;
  CipUsint minor_revision;
} ElectronicKeyFormat4;

void SetElectronicKeyFormat4VendorId(const CipUint vendor_id, ElectronicKeyFormat4 *const electronic_key) {
  electronic_key->vendor_id = vendor_id;
}

CipUint GetElectronicKeyFormat4VendorId(const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->vendor_id;
}

void SetElectronicKeyFormat4DeviceType(CipUint device_type, ElectronicKeyFormat4 *const electronic_key) {
  electronic_key->device_type = device_type;
}

CipUint GetElectronicKeyFormat4DeviceType(const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->device_type;
}

void SetElectronicKeyFormat4ProductCode(CipUint product_code, ElectronicKeyFormat4 *const electronic_key) {
  electronic_key->product_code = product_code;
}

CipUint GetElectronicKeyFormat4ProductCode(const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->product_code;
}

void SetElectronicKeyFormat4MajorRevisionCompatibility(CipByte major_revision_compatibility, ElectronicKeyFormat4 *const electronic_key) {
  electronic_key->major_revision_compatibility = major_revision_compatibility;
}

CipByte GetElectronicKeyFormat4MajorRevision(const ElectronicKeyFormat4 *const electronic_key) {
  const CipByte kMajorRevisionMask = 0x7F;
  return (electronic_key->major_revision_compatibility & kMajorRevisionMask);
}

bool GetElectronicKeyFormat4Compatibility(const ElectronicKeyFormat4 *const electronic_key) {
  const CipByte kCompatibilityMask = 0x80;
  if(kCompatibilityMask == (electronic_key->major_revision_compatibility & kCompatibilityMask) ) {
    return true;
  }
  return false;
}

void SetElectronicKeyFormat4MinorRevision(CipUsint minor_revision, ElectronicKeyFormat4 *const electronic_key) {
  electronic_key->minor_revision = minor_revision;
}
CipUsint GetElectronicKeyFormat4MinorRevision(const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->minor_revision;
}
