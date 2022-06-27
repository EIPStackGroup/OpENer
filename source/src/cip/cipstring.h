/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file
 * @brief Declare functions to operate on CIP string types
 *
 * Some functions to create CIP string types from C strings or data buffers.
 */

#ifndef OPENER_CIPSTRING_H_
#define OPENER_CIPSTRING_H_

#include "typedefs.h"
#include "ciptypes.h"

CipStringN *SetCipStringNByData(CipStringN *const cip_string,
                                CipUint str_len,
                                CipUint size,
                                const CipOctet *const data);

/** @brief Copies the content of C-string to a CipStringN under the expectation, that each C-String element is a StringN octet
 *
 * @param cip_string Target CipStringN
 * @param string Source C-string
 * @param symbol_size Size of a symbol in CipStringN instance
 *
 * @return Target CipStringN
 *
 */
CipStringN *SetCipStringNByCstr(CipStringN *const cip_string,
                                const char *const string,
                                const CipUint symbol_size);

/** @brief Clears the internal structure of a CipStringN
 *
 * @param cip_string The CipStringN to be freed
 *
 * @return Returns the address of the cleared CipStringN
 *
 */
CipStringN *ClearCipStringN(CipStringN *const cip_string);

/** @brief Frees the memory of a CipStringN
 *
 * @param cip_string The CipStringN to be freed
 *
 */
void FreeCipStringN(CipStringN *const cip_string);

/** @brief Sets length and data for a CipString2 based on an octet stream and symbol length
 *
 * @param cip_string The CipString2 to be set
 * @param str_len Amount of CipString2 symbols
 * @param data The octet stream
 *
 * @return The CipString2 address
 */
CipString2 *SetCipString2ByData(CipString2 *const cip_string,
                                CipUint str_len,
                                const CipOctet *const data);

/** @brief Copies the content of C-string to a CipString2 under the expectation, that each C-String element is a CipString2 octet
 *
 * @param cip_string Target CipString2
 * @param string Source C-string
 *
 * @return Target CipString2
 *
 */
CipString2 *SetCipString2ByCstr(CipString2 *const cip_string,
                                const char *const string);

/** @brief Clears the internal structure of a CipString2
 *
 * @param cip_string The CipString2 to be cleared
 *
 * @return Returns the address of the cleared CipString2
 *
 */
CipString2 *ClearCipString2(CipString2 *const cip_string);

/** @brief Frees a CipString2 structure
 *
 * @param cip_string The CipString2 structure to be freed
 *
 * @return Freed CipString2 structure
 *
 */
void FreeCipString2(CipString2 *const cip_string);

/** @brief Clears the internal CipString structure
 *
 * @param cip_string The CipString structure to be cleared
 *
 * @return Cleared CipString structure
 *
 */
CipString *ClearCipString(CipString *const cip_string);

/** @brief Frees a CipString structure
 *
 * @param cip_string The CipString structure to be freed
 *
 */
void FreeCipString(CipString *const cip_string);

/** @brief Sets length and data for a CipString based on an octet stream and symbol length
 *
 * @param cip_string The string to be set
 * @param str_len Amount of CipString symbols
 * @param data The octet stream
 *
 * @return The CipString address
 */
CipString *SetCipStringByData(CipString *const cip_string,
                              CipUint str_len,
                              const CipOctet *const data);

/** @brief Copies the content of C-string to a CipString under the expectation, that each C-String element is a CipString octet
 *
 * @param cip_string Target CipString
 * @param string Source C-string
 *
 * @return Target CipString
 *
 */
CipString *SetCipStringByCstr(CipString *const cip_string,
                              const char *const string);

/** @brief Clears the internal CipShortString structure
 *
 * @param cip_string The CipShortString structure to be cleared
 *
 * @return Cleared CipShortString structure
 *
 */
CipShortString *ClearCipShortString(CipShortString *const cip_string);

/** @brief Frees a CipShortString structure
 *
 * @param cip_string The CipShortString structure to be freed
 *
 */
void FreeCipShortString(CipShortString *const cip_string);

/** @brief Sets length and data for a CipShortString based on an octet stream and symbol length
 *
 * @param cip_string The string to be set
 * @param str_len Amount of CipString symbols
 * @param data The octet stream
 *
 * @return The CipShortString address
 */
CipShortString *SetCipShortStringByData(CipShortString *const cip_string,
                                        const CipUsint str_len,
                                        const CipOctet *const data);

/** @brief Copies the content of C-string to a CipShortString under the expectation, that each C-String element is a CipShortString octet
 *
 * @param cip_string Target CipShortString
 * @param string Source C-string
 *
 * @return Target CipShortString
 *
 */
CipShortString *SetCipShortStringByCstr(CipShortString *const cip_string,
                                        const char *const string);

/** @brief Returns a NUL terminated C-string from a CipShortString
 *
 * @param string The CipShortString to extract from
 * @param buf Pointer to a buffer to store the contents in
 * @param len Length of buffer
 *
 * @return POSIX OK(0) if the complete string fit in @param buf, otherwise non-zero.
 */
int GetCstrFromCipShortString(CipShortString *const string, char *buf,
			      size_t len);

#endif /* of OPENER_CIPSTRING_H_ */
