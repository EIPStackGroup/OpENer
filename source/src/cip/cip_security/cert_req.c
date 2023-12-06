/*
 *  Certificate request generation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 * 
 ******************************************************************************
 *  Modifications copyright (C) 2023, Rockwell Automation, Inc.
 *  All rights reserved.
 ******************************************************************************/

/** @file
 * @brief Certificate signing request (CSR) generation
 *
 * ********************************************************************
 * include files
 */
#include "mbedtls/build_info.h"
#include "mbedtls/platform.h"
/* md.h is included this early since MD_CAN_XXX macros are defined there. */
#include "mbedtls/md.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/x509_csr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cert_req.h"
#include "ciptypes.h"
#include "trace.h"

#define DFL_FILENAME RSA_KEY_FILE_LOCATION  // config in CMakeLists.txt
#define DFL_PASSWORD NULL
#define DFL_DEBUG_LEVEL 0
#define DFL_OUTPUT_FILENAME FILE_OBJECT_CSR_FILE_LOCATION  // config in CMakeLists.txt
#define SUBJECT_NAME_TEMPLATE "CN=%s,O=%s,OU=%s,L=%s,ST=%s,C=%s,R=%s"
#define DFL_KEY_USAGE 0
#define DFL_FORCE_KEY_USAGE 0
#define DFL_NS_CERT_TYPE 0
#define DFL_FORCE_NS_CERT_TYPE 0
#define DFL_MD_ALG MBEDTLS_MD_SHA256

/*
 * global options
 */
struct options {
  const char *filename;     /* filename of the key file             */
  const char *password;     /* password for the key file            */
  int debug_level;          /* level of debugging                   */
  const char *output_file;  /* where to store the constructed key file  */
  const char *subject_name; /* subject name for certificate request   */
  mbedtls_x509_san_list *san_list; /* subjectAltName for certificate request */
  unsigned char key_usage;         /* key usage flags                      */
  int force_key_usage;             /* Force adding the KeyUsage extension  */
  unsigned char ns_cert_type;      /* NS cert type                         */
  int force_ns_cert_type;          /* Force adding NsCertType extension    */
  mbedtls_md_type_t md_alg;        /* Hash algorithm used for signature.   */
} options;

/** @brief  Write certificate request
 * 
 *  @param req structure containing CSR Parameters
 *  @param output_file  where to store the constructed key file
 *  @param f_rng random number generator function
 *  @param p_rng random number generator param
 *  @return status
 */
int write_certificate_request(mbedtls_x509write_csr *req,
                              const char *output_file,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng) {

  int ret; //return value
  unsigned char output_buf[4096];

  memset(output_buf, 0, 4096);
  if ((ret = mbedtls_x509write_csr_pem(req, output_buf, 4096, f_rng, p_rng)) <
      0) {
    return ret;
  }

  size_t len = strlen((char *)output_buf);
  FILE *file;

  if ((file = fopen(output_file, "w")) == NULL) {
    return -1;
  }

  if (fwrite(output_buf, 1, len, file) != len) {
    fclose(file);
    return -1;
  }

  fclose(file);

  return 0;
}

/* function called in OpENer certificatemanagement */
int MbedtlsWriteCSR(CipShortString *short_strings) { 
  int ret = 1;
  int exit_code = MBEDTLS_EXIT_FAILURE;
  mbedtls_pk_context key;
  char buf[1024];

  mbedtls_x509write_csr req;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg; //ctr_drbg - Counter Mode Deterministic Random Bit Generator
  mbedtls_x509_san_list *cur, *prev; //Subject Alternative Name List

  /*
   * Set to sane values
   */
  mbedtls_x509write_csr_init(&req);
  mbedtls_pk_init(&key);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  memset(buf, 0, sizeof(buf));
  mbedtls_entropy_init(&entropy);

#if defined(MBEDTLS_USE_PSA_CRYPTO)
  psa_status_t status = psa_crypto_init();
  if (status != PSA_SUCCESS) {
    mbedtls_fprintf(stderr,
                    "Failed to initialize PSA Crypto implementation: %d\n",
                    (int)status);
    goto exit;
  }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

  /* create subjectName string from input params */
  char subjectName[500];
  snprintf(subjectName, sizeof(subjectName), SUBJECT_NAME_TEMPLATE, short_strings[0].string,
                                                                    short_strings[1].string,
                                                                    short_strings[2].string,
                                                                    short_strings[3].string,
                                                                    short_strings[4].string,
                                                                    short_strings[5].string,
                                                                    short_strings[6].string);

  options.filename = DFL_FILENAME;
  options.password = DFL_PASSWORD;
  options.debug_level = DFL_DEBUG_LEVEL;
  options.output_file = DFL_OUTPUT_FILENAME;
  options.subject_name = subjectName;
  options.key_usage = DFL_KEY_USAGE;
  options.force_key_usage = DFL_FORCE_KEY_USAGE;
  options.ns_cert_type = DFL_NS_CERT_TYPE;
  options.force_ns_cert_type = DFL_FORCE_NS_CERT_TYPE;
  options.md_alg = DFL_MD_ALG;
  options.san_list = NULL;

  /* Set the MD algorithm to use for the signature in the CSR */
  mbedtls_x509write_csr_set_md_alg(&req, options.md_alg);

  /* Set the Key Usage Extension flags in the CSR */
  if (options.key_usage || options.force_key_usage == 1) {
    ret = mbedtls_x509write_csr_set_key_usage(&req, options.key_usage);

    if (ret != 0) {
      OPENER_TRACE_INFO(
          " failed\n  !  mbedtls_x509write_csr_set_key_usage returned %d", ret);
      goto exit;
    }
  }

  /* Set the Cert Type flags in the CSR */
  if (options.ns_cert_type || options.force_ns_cert_type == 1) {
    ret = mbedtls_x509write_csr_set_ns_cert_type(&req, options.ns_cert_type);

    if (ret != 0) {
      OPENER_TRACE_INFO(
          " failed\n  !  mbedtls_x509write_csr_set_ns_cert_type returned %d", ret);
      goto exit;
    }
  }

  /*
   * 0. Seed the PRNG (Pseudorandom number generator)
   */
  OPENER_TRACE_INFO("  . Seeding the random number generator...");
  fflush(stdout);

  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   NULL,
                                   0)) != 0) {
    OPENER_TRACE_INFO(" failed\n  !  mbedtls_ctr_drbg_seed returned %d", ret);
    goto exit;
  }

  OPENER_TRACE_INFO(" ok\n");

  /*
   * 1.0. Check the subject name for validity
   */
  OPENER_TRACE_INFO("  . Checking subject name...");
  fflush(stdout);

  if ((ret = mbedtls_x509write_csr_set_subject_name(&req, options.subject_name)) !=
      0) {
    OPENER_TRACE_INFO(
        " failed\n  !  mbedtls_x509write_csr_set_subject_name returned %d",
        ret);
    goto exit;
  }

  OPENER_TRACE_INFO(" ok\n");

  /*
   * 1.1. Load the key
   */
  OPENER_TRACE_INFO("  . Loading the private key ...");
  fflush(stdout);

  ret = mbedtls_pk_parse_keyfile(&key, options.filename, options.password,
                                 mbedtls_ctr_drbg_random, &ctr_drbg);

  if (ret != 0) {
    OPENER_TRACE_INFO(" failed\n  !  mbedtls_pk_parse_keyfile returned %d", ret);
    goto exit;
  }

  mbedtls_x509write_csr_set_key(&req, &key);

  OPENER_TRACE_INFO(" ok\n");

  /*
   * 1.2. Writing the request
   */
  OPENER_TRACE_INFO("  . Writing the certificate request ...");
  fflush(stdout);

  if ((ret = write_certificate_request(
           &req, options.output_file, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
    OPENER_TRACE_INFO(" failed\n  !  write_certificate_request %d", ret);
    goto exit;
  }

  OPENER_TRACE_INFO(" ok\n\n");

  exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

  if (exit_code != MBEDTLS_EXIT_SUCCESS) {
#ifdef MBEDTLS_ERROR_C
    mbedtls_strerror(ret, buf, sizeof(buf));
    OPENER_TRACE_INFO(" - %s\n", buf);
#else
    OPENER_TRACE_INFO("\n");
#endif
  }

  mbedtls_x509write_csr_free(&req);
  mbedtls_pk_free(&key);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
  mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

  cur = options.san_list;
  while (cur != NULL) {
    prev = cur;
    cur = cur->next;
    mbedtls_free(prev);
  }

  return exit_code;
}