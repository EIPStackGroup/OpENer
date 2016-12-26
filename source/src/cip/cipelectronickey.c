/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <stdlib.h>

#include "cipelectronickey.h"

typedef struct electronic_key_format_4 {
  CipUint vendor_id;
  CipUint device_type;
  CipUint product_code;
  CipByte major_revision_compatibility;
  CipUsint minor_revision;
} ElectronicKeyFormat4;

const size_t kElectronicKeyFormat4Size = sizeof(ElectronicKeyFormat4);

ElectronicKeyFormat4 *ElectronicKeyFormat4New() {
  return (ElectronicKeyFormat4 *)calloc( 1, sizeof(ElectronicKeyFormat4) );
}

void ElectronicKeyFormat4Delete(ElectronicKeyFormat4 **electronic_key) {
  free(*electronic_key);
  *electronic_key = NULL;
}

void ElectronicKeyFormat4SetVendorId(const CipUint vendor_id,
                                     ElectronicKeyFormat4 *const electronic_key)
{
  electronic_key->vendor_id = vendor_id;
}

CipUint ElectronicKeyFormat4GetVendorId(
  const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->vendor_id;
}

void ElectronicKeyFormat4SetDeviceType(const CipUint device_type,
                                       ElectronicKeyFormat4 *const electronic_key)
{
  electronic_key->device_type = device_type;
}

CipUint ElectronicKeyFormat4GetDeviceType(
  const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->device_type;
}

void ElectronicKeyFormat4SetProductCode(const CipUint product_code,
                                        ElectronicKeyFormat4 *const electronic_key)
{
  electronic_key->product_code = product_code;
}

CipUint ElectronicKeyFormat4GetProductCode(
  const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->product_code;
}

void ElectronicKeyFormat4SetMajorRevisionCompatibility(
  const CipByte major_revision_compatibility,
  ElectronicKeyFormat4 *const electronic_key) {
  electronic_key->major_revision_compatibility = major_revision_compatibility;
}

CipByte ElectronicKeyFormat4GetMajorRevision(
  const ElectronicKeyFormat4 *const electronic_key) {
  const CipByte kMajorRevisionMask = 0x7F;
  return (electronic_key->major_revision_compatibility & kMajorRevisionMask);
}

bool ElectronicKeyFormat4GetMajorRevisionCompatibility(
  const ElectronicKeyFormat4 *const electronic_key) {
  const CipByte kCompatibilityMask = 0x80;
  if( kCompatibilityMask ==
      (electronic_key->major_revision_compatibility & kCompatibilityMask) ) {
    return true;
  }
  return false;
}

void ElectronicKeyFormat4SetMinorRevision(const CipUsint minor_revision,
                                          ElectronicKeyFormat4 *const electronic_key)
{
  electronic_key->minor_revision = minor_revision;
}
CipUsint ElectronicKeyFormat4GetMinorRevision(
  const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->minor_revision;
}
