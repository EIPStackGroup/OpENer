/*
 *  Key generation application
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
 *******************************************************************************
 *  Modifications copyright (C) 2023, Rockwell Automation, Inc.
 *  All rights reserved.
 ******************************************************************************/

/** @file
 * @brief Key generation application
 *
 * ********************************************************************
 * include files
 */
#include "mbedtls/build_info.h"

#include "mbedtls/platform.h"

#if defined(MBEDTLS_PK_WRITE_C) && defined(MBEDTLS_FS_IO) && \
    defined(MBEDTLS_ENTROPY_C) && defined(MBEDTLS_CTR_DRBG_C)
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gen_key.h"
#include "trace.h"

#if !defined(_WIN32)
#include <unistd.h>

#define DEV_RANDOM_THRESHOLD 32

#define PRINT_KEY 0 // print key in the terminal

/** @brief gather additional randomness
 * 
 *  @param data user-specific context
 *  @param output  buffer for random data
 *  @param len length of buffer
 *  @param olen length of output buffer - number of random bytes
 *  @return status
 */
int dev_random_entropy_poll(void *data, unsigned char *output, size_t len,
                            size_t *olen) {
  FILE *file;
  size_t ret = len;
  size_t left = len;
  unsigned char *p = output;
  ((void)data);

  *olen = 0;

  file = fopen("/dev/random", "rb");
  if (file == NULL) {
    return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
  }

  while (left > 0) {
    /* /dev/random can return much less than requested. If so, try again */
    ret = fread(p, 1, left, file);
    if (ret == 0 && ferror(file)) { 
      fclose(file);
      return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    } 

    p += ret;
    left -= ret;
    sleep(1);
  }
  fclose(file);
  *olen = len;

  return 0;
}
#endif /* !_WIN32 */
#endif

#if defined(MBEDTLS_ECP_C)
#define DFL_EC_CURVE mbedtls_ecp_curve_list()->grp_id // list of supported elliptic curves
#else
#define DFL_EC_CURVE 0
#endif

#if !defined(_WIN32) && defined(MBEDTLS_FS_IO)
#define USAGE_DEV_RANDOM "    use_dev_random=0|1    default: 0\n"
#else
#define USAGE_DEV_RANDOM ""
#endif /* !_WIN32 && MBEDTLS_FS_IO */

#define FORMAT_PEM 0
#define FORMAT_DER 1

#define DFL_TYPE            MBEDTLS_PK_RSA
#define DFL_RSA_KEYSIZE     2048
#define DFL_FILENAME        RSA_KEY_FILE_LOCATION  // "keyfile.key"
#define DFL_FORMAT          FORMAT_PEM
#define DFL_USE_DEV_RANDOM  0

/*
 * global options
 */
struct options {
  int type;             /* the type of key to generate          */
  unsigned int rsa_keysize;    /* length of key in bits         */
  int ec_curve;         /* curve identifier for EC keys         */
  const char *filename; /* filename of the key file             */
  int format;           /* the output format to use             */
  int use_dev_random;   /* use /dev/random as entropy source    */
} options;

/** @brief  write a private key to a file in a particular format
 * 
 *  @param key private key
 *  @param output_file  output file pointer
 *  @return status
 */
static int write_private_key(mbedtls_pk_context *key, const char *output_file) {
  int ret;
  FILE *file;
  unsigned char output_buf[16000];
  unsigned char *c = output_buf;
  size_t len = 0;

  memset(output_buf, 0, 16000);
  if (options.format == FORMAT_PEM) {
    if ((ret = mbedtls_pk_write_key_pem(key, output_buf, 16000)) != 0) {
      return ret;
    }

    len = strlen((char *)output_buf);
  } else {
    if ((ret = mbedtls_pk_write_key_der(key, output_buf, 16000)) < 0) {
      return ret;
    }

    len = (size_t)ret;
    c = output_buf + sizeof(output_buf) - len;
  }

  if ((file = fopen(output_file, "wb")) == NULL) {
    return -1;
  }

  if (fwrite(c, 1, len, file) != len) {
    fclose(file);
    return -1;
  }

  fclose(file);

  return 0;
}

/* function called in OpENer certificatemanagement */
int MbedtlsGenerateKey(void)
{
  int ret = 1;
  int exit_code = MBEDTLS_EXIT_FAILURE;
  mbedtls_pk_context key;
  char buf[1024];

  mbedtls_mpi N, P, Q, D, E, DP, DQ, QP; //multi precision integer structure
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg; //ctr_drbg - Counter Mode Deterministic Random Bit Generator
  const char *personalization = "gen_key"; // The personalization string is a small protection against a lack of startup entropy and ensures each application has at least a different starting point.

  /*
   * Set to sane values
   */

  mbedtls_mpi_init(&N); //multi precision integer structure
  mbedtls_mpi_init(&P);
  mbedtls_mpi_init(&Q);
  mbedtls_mpi_init(&D);
  mbedtls_mpi_init(&E);
  mbedtls_mpi_init(&DP);
  mbedtls_mpi_init(&DQ);
  mbedtls_mpi_init(&QP);

  mbedtls_pk_init(&key); //pk - public key
  mbedtls_ctr_drbg_init(&ctr_drbg);
  memset(buf, 0, sizeof(buf));

#if defined(MBEDTLS_USE_PSA_CRYPTO)
  psa_status_t status = psa_crypto_init();
  if (status != PSA_SUCCESS) {
    OPENER_TRACE_INFO(stderr,
                    "Failed to initialize PSA Crypto implementation: %d\n",
                    (int)status);
    goto exit;
  }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

  options.type = DFL_TYPE;
  options.rsa_keysize = DFL_RSA_KEYSIZE;
  options.ec_curve = DFL_EC_CURVE;
  options.filename = DFL_FILENAME;
  options.format = DFL_FORMAT;
  options.use_dev_random = DFL_USE_DEV_RANDOM;

  OPENER_TRACE_INFO("\n  . Seeding the random number generator...");
  fflush(stdout);

  mbedtls_entropy_init(&entropy);
#if !defined(_WIN32) && defined(MBEDTLS_FS_IO)
  if (options.use_dev_random) {
    if ((ret = mbedtls_entropy_add_source(
             &entropy, dev_random_entropy_poll, NULL, DEV_RANDOM_THRESHOLD,
             MBEDTLS_ENTROPY_SOURCE_STRONG)) != 0) {
      OPENER_TRACE_INFO(
          " failed\n  ! mbedtls_entropy_add_source returned -0x%04x\n",
          (unsigned int)-ret);
      goto exit;
    }

    OPENER_TRACE_INFO("\n    Using /dev/random, so can take a long time! ");
    fflush(stdout);
  }
#endif /* !_WIN32 && MBEDTLS_FS_IO */

  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char *)personalization,
                                   strlen(personalization))) != 0) {
    OPENER_TRACE_INFO(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%04x\n",
                   (unsigned int)-ret);
    goto exit;
  }

  OPENER_TRACE_INFO(" ok");

  /*
   * 1.1. Generate the key
   */
  OPENER_TRACE_INFO("\n  . Generating the private key ...");
  fflush(stdout);

  if ((ret = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(
                                        (mbedtls_pk_type_t)options.type))) != 0) {
    OPENER_TRACE_INFO(" failed\n  !  mbedtls_pk_setup returned -0x%04x",
                   (unsigned int)-ret);
    goto exit;
  }

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_GENPRIME)
  if (options.type == MBEDTLS_PK_RSA) {
    ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random,
                              &ctr_drbg, options.rsa_keysize, 65537);
    if (ret != 0) {
      OPENER_TRACE_INFO(" failed\n  !  mbedtls_rsa_gen_key returned -0x%04x",
                     (unsigned int)-ret);
      goto exit;
    }
  } else
#endif /* MBEDTLS_RSA_C */
#if defined(MBEDTLS_ECP_C)
      if (options.type == MBEDTLS_PK_ECKEY) {
    ret = mbedtls_ecp_gen_key((mbedtls_ecp_group_id)options.ec_curve,
                              mbedtls_pk_ec(key), mbedtls_ctr_drbg_random,
                              &ctr_drbg);
    if (ret != 0) {
      OPENER_TRACE_INFO(" failed\n  !  mbedtls_ecp_gen_key returned -0x%04x",
                     (unsigned int)-ret);
      goto exit;
    }
  } else
#endif /* MBEDTLS_ECP_C */
  {
    OPENER_TRACE_INFO(" failed\n  !  key type not supported\n");
    goto exit;
  }

   OPENER_TRACE_INFO(" ok");

#if(PRINT_KEY)
  /*
   * 1.2 Print the key - OPTIONAL
   */
  OPENER_TRACE_INFO("\n  . Key information:\n");

#if defined(MBEDTLS_RSA_C)
  if (mbedtls_pk_get_type(&key) == MBEDTLS_PK_RSA) {
    mbedtls_rsa_context *rsa = mbedtls_pk_rsa(key);

    if ((ret = mbedtls_rsa_export(rsa, &N, &P, &Q, &D, &E)) != 0 ||
        (ret = mbedtls_rsa_export_crt(rsa, &DP, &DQ, &QP)) != 0) {
      OPENER_TRACE_INFO(" failed\n  ! could not export RSA parameters\n\n");
      goto exit;
    }

    mbedtls_mpi_write_file("N:  ", &N, 16, NULL); // mpi - multi precision integer structure
    mbedtls_mpi_write_file("E:  ", &E, 16, NULL);
    mbedtls_mpi_write_file("D:  ", &D, 16, NULL);
    mbedtls_mpi_write_file("P:  ", &P, 16, NULL);
    mbedtls_mpi_write_file("Q:  ", &Q, 16, NULL);
    mbedtls_mpi_write_file("DP: ", &DP, 16, NULL);
    mbedtls_mpi_write_file("DQ:  ", &DQ, 16, NULL);
    mbedtls_mpi_write_file("QP:  ", &QP, 16, NULL);
  } else
#endif
#if defined(MBEDTLS_ECP_C)
      if (mbedtls_pk_get_type(&key) == MBEDTLS_PK_ECKEY) {
    mbedtls_ecp_keypair *ecp = mbedtls_pk_ec(key);
    OPENER_TRACE_INFO(
        "curve: %s\n",
        mbedtls_ecp_curve_info_from_grp_id(ecp->MBEDTLS_PRIVATE(grp).id)->name);
    mbedtls_mpi_write_file(
        "X_Q:   ", &ecp->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X), 16, NULL);
    mbedtls_mpi_write_file(
        "Y_Q:   ", &ecp->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Y), 16, NULL);
    mbedtls_mpi_write_file("D:     ", &ecp->MBEDTLS_PRIVATE(d), 16, NULL);
  } else
#endif
    OPENER_TRACE_INFO("  ! key type not supported\n");

#endif /* PRINT_KEY */  

  /*
   * 1.3 Export key
   */
  OPENER_TRACE_INFO("\n  . Writing key to file... ");

  if ((ret = write_private_key(&key, options.filename)) != 0) {
    OPENER_TRACE_INFO(" failed\n");
    goto exit;
  }

  OPENER_TRACE_INFO(" ok\n");

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

  mbedtls_mpi_free(&N);  // mpi - multi precision integer structure
  mbedtls_mpi_free(&P);
  mbedtls_mpi_free(&Q);
  mbedtls_mpi_free(&D);
  mbedtls_mpi_free(&E);
  mbedtls_mpi_free(&DP);
  mbedtls_mpi_free(&DQ);
  mbedtls_mpi_free(&QP);

  mbedtls_pk_free(&key);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
  mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

  // mbedtls_exit(exit_code)
  return exit_code;
}
