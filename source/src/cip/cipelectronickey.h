/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPELECTRONICKEY_H_
#define SRC_CIP_CIPELECTRONICKEY_H_

#include <stdbool.h>

#include "typedefs.h"

/** @brief Declaration of the electronic key format 4 data struct for the class
 *
 */
typedef struct electronic_key_format_4 ElectronicKeyFormat4;

extern const size_t kElectronicKeyFormat4Size;

/** @brief Constructor for the electroic key format 4 class
 *
 *	@return A new unset electronic key
 */
ElectronicKeyFormat4 *ElectronicKeyFormat4New();

/** @brief Destructor for the electroic key format 4 class
 *
 *  Safe destructor/free, nulls the pointer after freeing it
 *  @param electronic_key A format 4 electronic key
 */
void ElectronicKeyFormat4Delete(ElectronicKeyFormat4 **electronic_key);

/** @brief Sets vendor ID in the electronic key
 *
 *	@param vendor_id The vendor ID to be set into the electronic key
 *	@param electronic_key The electronic key to be set - will be modified
 */
void ElectronicKeyFormat4SetVendorId(const CipUint vendor_id,
                                     ElectronicKeyFormat4 *const electronic_key);

/** @brief Gets the vendor ID form the electronic key
 *
 *      @param electronic_key The format 4 electronic key from which the vendor ID will be extracted
 *      @return The vendor ID
 */
CipUint ElectronicKeyFormat4GetVendorId(
  const ElectronicKeyFormat4 *const electronic_key);

/** @brief Sets the device type in the electronic key
 *
 *	@param device_type The device type which shall be inserted into the electronic key
 *	@param electronic_key A format 4 electronic key
 */
void ElectronicKeyFormat4SetDeviceType(
  const CipUint device_type,ElectronicKeyFormat4 *const
  electronic_key);

/** @brief Gets the device type from a format 4 electronic key
 *
 *      @param The format 4 electronic key from which the device type will be extracted
 *      @return The device type
 */
CipUint ElectronicKeyFormat4GetDeviceType(
  const ElectronicKeyFormat4 *const electronic_key);

/** @brief Set product code in the electronic key
 *
 *      @param product_code The product code to be inserted
 *      @param electronic_key The electronic key to be modified
 */
void ElectronicKeyFormat4SetProductCode(
  const CipUint product_code,ElectronicKeyFormat4 *const
  electronic_key);

/** @brief Gets the product code from an format 4 electronic key
 *
 *      @param electronic_key The format 4 electronic key to be read
 *      @return The product code
 */
CipUint ElectronicKeyFormat4GetProductCode(
  const ElectronicKeyFormat4 *const electronic_key);

/** @brief Sets the major revision byte including the compatibility flag
 *
 *      @param major_revision_compatibility The major revision byte including the compatibility flag
 *      @param electronic_key The electronic key to be modified
 */
void ElectronicKeyFormat4SetMajorRevisionCompatibility(
  const CipByte major_revision_compatibility,
  ElectronicKeyFormat4 *const electronic_key);

/** @brief Gets the major revision from an format 4 electronic key
 *
 *      @param electronic_key An format 4 electronic key
 *      @return The device's major revision
 */
CipByte ElectronicKeyFormat4GetMajorRevision(
  const ElectronicKeyFormat4 *const electronic_key);

/** @brief Gets the Compatibility flag from the format 4 electronic key
 *
 *      @param electronic_key The format 4 electronic key to be read
 *      @return True if compatibility bit is set, false if otherwise
 */
bool ElectronicKeyFormat4GetMajorRevisionCompatibility(
  const ElectronicKeyFormat4 *const electronic_key);

/** @brief Sets the devices minor revision in an format 4 electronic key
 *
 *      @param minor_revision The minor revision to be set in the electronic key
 *      @param electronic_key The electronic key to be modified
 */
void ElectronicKeyFormat4SetMinorRevision(
  const CipUsint minor_revision,ElectronicKeyFormat4 *const
  electronic_key);

/** @brief Gets the minor revision from an format 4 electronic key
 *
 *      @param electronic_key The format 4 electronic key to be read
 *      @return The device's minor revision
 */
CipUsint ElectronicKeyFormat4GetMinorRevision(
  const ElectronicKeyFormat4 *const electronic_key);

#endif /* SRC_CIP_CIPELECTRONICKEY_H_ */
