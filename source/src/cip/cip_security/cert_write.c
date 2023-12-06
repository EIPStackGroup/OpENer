/*
 *  Certificate generation and signing
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
 * @brief Certificate generation and signing
 *
 * ********************************************************************
 * include files
 */
#include "mbedtls/build_info.h"

#include "mbedtls/platform.h"
/* md.h is included this early since MD_CAN_XXX macros are defined there. */
#include "mbedtls/md.h"

#include "mbedtls/x509_crt.h"
#include "mbedtls/x509_csr.h"
#include "mbedtls/oid.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "cert_write.h"
#include "trace.h"

// oid - object identifier 
#define SET_OID(x, oid) \
    do { x.len = MBEDTLS_OID_SIZE(oid); x.p = (unsigned char *) oid; } while (0)

#if defined(MBEDTLS_X509_CSR_PARSE_C)
#define USAGE_CSR                                                           \
    "    request_file=%%s         default: (empty)\n"                           \
    "                            If request_file is specified, subject_key,\n"  \
    "                            subject_pwd and subject_name are ignored!\n"
#else
#define USAGE_CSR ""
#endif /* MBEDTLS_X509_CSR_PARSE_C */

#define FORMAT_PEM              0
#define FORMAT_DER              1

#define DFL_ISSUER_CRT          ""
#define DFL_REQUEST_FILE        ""
#define DFL_SUBJECT_KEY         RSA_KEY_FILE_LOCATION // "subject.key"
#define DFL_ISSUER_KEY          RSA_KEY_FILE_LOCATION // "ca.key"
#define DFL_SUBJECT_PWD         ""
#define DFL_ISSUER_PWD          ""
#define DFL_OUTPUT_FILENAME     FILE_OBJECT_CERTIFICATE_FILE_LOCATION // "cert.crt"
#define SUBJECT_NAME_TEMPLATE   "CN=%s,O=%s,OU=%s,L=%s,ST=%s,C=%s,R=%s"
//#define DFL_SUBJECT_NAME      "CN=CA,O=mbed TLS,C=UK"
#define DFL_ISSUER_NAME         SUBJECT_NAME_TEMPLATE//"CN=CA,O=mbed TLS,C=UK"
#define DFL_NOT_BEFORE          "20010101000000"
#define DFL_NOT_AFTER           "20301231235959"
#define DFL_SERIAL              "1"
#define DFL_SERIAL_HEX          "1"
#define DFL_SELFSIGN            1 // 1: self-signed
#define DFL_IS_CA               0
#define DFL_MAX_PATHLEN         -1
#define DFL_SIG_ALG             MBEDTLS_MD_SHA256
#define DFL_KEY_USAGE           0
#define DFL_EXT_KEY_USAGE       NULL
#define DFL_NS_CERT_TYPE        0
#define DFL_VERSION             3
#define DFL_AUTH_IDENT          1
#define DFL_SUBJ_IDENT          1
#define DFL_CONSTRAINTS         1
#define DFL_DIGEST              MBEDTLS_MD_SHA256
#define DFL_FORMAT              FORMAT_PEM

typedef enum {
    SERIAL_FRMT_UNSPEC,
    SERIAL_FRMT_DEC,
    SERIAL_FRMT_HEX
} serial_format_t;

/*
 * global options
 */
struct options {
    const char *issuer_crt;     /* filename of the issuer certificate   */
    const char *request_file;   /* filename of the certificate request  */
    const char *subject_key;    /* filename of the subject key file     */
    const char *issuer_key;     /* filename of the issuer key file      */
    const char *subject_pwd;    /* password for the subject key file    */
    const char *issuer_pwd;     /* password for the issuer key file     */
    const char *output_file;    /* where to store the constructed CRT   */
    const char *subject_name;   /* subject name for certificate         */
    const char *issuer_name;    /* issuer name for certificate          */
    const char *not_before;     /* validity period not before           */
    const char *not_after;      /* validity period not after            */
    const char *serial;         /* serial number string (decimal)       */
    const char *serial_hex;     /* serial number string (hex)           */
    int selfsign;               /* selfsign the certificate             */
    int is_ca;                  /* is a CA certificate                  */
    int max_pathlen;            /* maximum CA path length               */
    int authority_identifier;   /* add authority identifier to CRT      */
    int subject_identifier;     /* add subject identifier to CRT        */
    int basic_constraints;      /* add basic constraints ext to CRT     */
    int version;                /* CRT version                          */
    mbedtls_md_type_t md;       /* Hash used for signing                */
    unsigned char key_usage;    /* key usage flags                      */
    mbedtls_asn1_sequence *ext_key_usage; /* extended key usages        */
    unsigned char ns_cert_type; /* NS cert type                         */
    int format;                 /* format                               */
} options;


/** @brief  write an X.509 certificate to a PEM or DER file format
 * 
 *  @param crt certificate structure
 *  @param output_file  output file pointer
 *  @param f_rng random number generator function
 *  @param p_rng random number generator param
 *  @return status
 */
int write_certificate(mbedtls_x509write_cert *crt, const char *output_file,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng)
{
    int ret;
    FILE *file;
    unsigned char output_buf[4096];
    unsigned char *output_start;
    size_t len = 0;

    memset(output_buf, 0, 4096);
    if (options.format == FORMAT_DER) {
        ret = mbedtls_x509write_crt_der(crt, output_buf, 4096,
                                        f_rng, p_rng);
        if (ret < 0) {
            return ret;
        }

        len = (size_t)ret;
        output_start = output_buf + 4096 - len;
    } else {
        ret = mbedtls_x509write_crt_pem(crt, output_buf, 4096,
                                        f_rng, p_rng);
        if (ret < 0) {
            return ret;
        }

        len = strlen((char *) output_buf);
        output_start = output_buf;
    }

    if ((file = fopen(output_file, "w")) == NULL) {
        return -1;
    }

    if (fwrite(output_start, 1, len, file) != len) {
        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}

/** @brief  parse and convert a decimal string format to its equivalent binary representation
 * 
 *  @param obuf  output buffer
 *  @param obufmax 
 *  @param ibuf  input buffer
 *  @param len  
 *  @return status
 */
int parse_serial_decimal_format(unsigned char *obuf, size_t obufmax,
                                const char *ibuf, size_t *len)
{
    unsigned long long int dec;
    unsigned int remaining_bytes = sizeof(dec);
    unsigned char *p = obuf;
    unsigned char val;
    char *end_ptr = NULL;

    errno = 0;
    dec = strtoull(ibuf, &end_ptr, 10);

    if ((errno != 0) || (end_ptr == ibuf)) {
        return -1;
    }

    *len = 0;

    while (remaining_bytes > 0) {
        if (obufmax < (*len + 1)) {
            return -1;
        }

        val = (dec >> ((remaining_bytes - 1) * 8)) & 0xFF;

        /* Skip leading zeros */
        if ((val != 0) || (*len != 0)) {
            *p = val;
            (*len)++;
            p++;
        }

        remaining_bytes--;
    }

    return 0;
}

/* function called in OpENer certificatemanagement */
int MbedtlsGenerateCertificate(char *subject_name_input[], char *serial_number_input)
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_x509_crt issuer_crt;
    mbedtls_pk_context loaded_issuer_key;
    mbedtls_pk_context loaded_subject_key;
    mbedtls_pk_context *issuer_key = &loaded_issuer_key;
    mbedtls_pk_context *subject_key = &loaded_subject_key;
    char buf[1024];
    char issuer_name[256];

#if defined(MBEDTLS_X509_CSR_PARSE_C)
    char subject_name[256]; 
    mbedtls_x509_csr csr;
#endif
    mbedtls_x509write_cert crt;
    //serial_format_t serial_frmt = SERIAL_FRMT_UNSPEC;
    unsigned char serial[MBEDTLS_X509_RFC5280_MAX_SERIAL_LEN];
    size_t serial_len;
    //mbedtls_asn1_sequence *ext_key_usage;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg; //ctr_drbg - Counter Mode Deterministic Random Bit Generator

    /*
     * Set to sane values
     */
    mbedtls_x509write_crt_init(&crt);
    mbedtls_pk_init(&loaded_issuer_key);
    mbedtls_pk_init(&loaded_subject_key);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
#if defined(MBEDTLS_X509_CSR_PARSE_C)
    mbedtls_x509_csr_init(&csr);
#endif
    mbedtls_x509_crt_init(&issuer_crt);
    memset(buf, 0, sizeof(buf));
    memset(serial, 0, sizeof(serial));

#if defined(MBEDTLS_USE_PSA_CRYPTO)
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        mbedtls_fprintf(stderr, "Failed to initialize PSA Crypto implementation: %d\n",
                        (int) status);
        goto exit;
    }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

/* create issuerName string from input params */
    char issuerName[500];
    snprintf(issuerName, sizeof(issuerName), SUBJECT_NAME_TEMPLATE, subject_name_input[0],
                                                                      subject_name_input[1],
                                                                      subject_name_input[2],
                                                                      subject_name_input[3],
                                                                      subject_name_input[4],
                                                                      subject_name_input[5],
                                                                      subject_name_input[6]);

    options.issuer_crt          = DFL_ISSUER_CRT;
    options.request_file        = DFL_REQUEST_FILE;
    options.subject_key         = DFL_SUBJECT_KEY;
    options.issuer_key          = DFL_ISSUER_KEY;
    options.subject_pwd         = DFL_SUBJECT_PWD;
    options.issuer_pwd          = DFL_ISSUER_PWD;
    options.output_file         = DFL_OUTPUT_FILENAME;
    options.subject_name        = SUBJECT_NAME_TEMPLATE;  // same as issuer_name for self-signed certificate
    options.issuer_name         = issuerName;
    options.not_before          = DFL_NOT_BEFORE;
    options.not_after           = DFL_NOT_AFTER;
    options.serial              = serial_number_input;
    options.serial_hex          = DFL_SERIAL_HEX;
    options.selfsign            = DFL_SELFSIGN;
    options.is_ca               = DFL_IS_CA;
    options.max_pathlen         = DFL_MAX_PATHLEN;
    options.key_usage           = DFL_KEY_USAGE;
    options.ext_key_usage       = DFL_EXT_KEY_USAGE;
    options.ns_cert_type        = DFL_NS_CERT_TYPE;
    options.version             = DFL_VERSION - 1;
    options.md                  = DFL_DIGEST;
    options.subject_identifier   = DFL_SUBJ_IDENT;
    options.authority_identifier = DFL_AUTH_IDENT;
    options.basic_constraints    = DFL_CONSTRAINTS;
    options.format              = DFL_FORMAT;

    OPENER_TRACE_INFO("\n");

    /*
     * 0. Seed the PRNG
     */
    OPENER_TRACE_INFO("  . Seeding the random number generator...");
    fflush(stdout);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     NULL,
                                     0)) != 0) {
        mbedtls_strerror(ret, buf, sizeof(buf));
        OPENER_TRACE_INFO(" failed\n  !  mbedtls_ctr_drbg_seed returned %d - %s\n",
                       ret, buf);
        goto exit;
    }

    OPENER_TRACE_INFO(" ok\n");

    // Parse serial to MPI
    OPENER_TRACE_INFO("  . Reading serial number...");
    fflush(stdout);

    ret = parse_serial_decimal_format(serial, sizeof(serial), options.serial,
                                      &serial_len);

    if (ret != 0) {
        OPENER_TRACE_INFO(" failed\n  !  Unable to parse serial\n");
        goto exit;
    }

    OPENER_TRACE_INFO(" ok\n");

    // Parse issuer certificate if present
    //
    if (!options.selfsign && strlen(options.issuer_crt)) {
        /*
         * 1.0.a. Load the certificates
         */
        OPENER_TRACE_INFO("  . Loading the issuer certificate ...");
        fflush(stdout);

        if ((ret = mbedtls_x509_crt_parse_file(&issuer_crt, options.issuer_crt)) != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509_crt_parse_file "
                           "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
            goto exit;
        }

        ret = mbedtls_x509_dn_gets(issuer_name, sizeof(issuer_name),
                                   &issuer_crt.subject);
        if (ret < 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509_dn_gets "
                           "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
            goto exit;
        }

        options.issuer_name = issuer_name;

        OPENER_TRACE_INFO(" ok\n");
    }

#if defined(MBEDTLS_X509_CSR_PARSE_C)
    // Parse certificate request if present
    //
    if (!options.selfsign && strlen(options.request_file)) {
        /*
         * 1.0.b. Load the CSR
         */
        OPENER_TRACE_INFO("  . Loading the certificate request ...");
        fflush(stdout);

        if ((ret = mbedtls_x509_csr_parse_file(&csr, options.request_file)) != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509_csr_parse_file "
                           "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
            goto exit;
        }

        ret = mbedtls_x509_dn_gets(subject_name, sizeof(subject_name),
                                   &csr.subject);
        if (ret < 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509_dn_gets "
                           "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
            goto exit;
        }

        options.subject_name = subject_name;
        subject_key = &csr.pk;

        OPENER_TRACE_INFO(" ok\n");
    }
#endif /* MBEDTLS_X509_CSR_PARSE_C */

    /*
     * 1.1. Load the keys
     */
    if (!options.selfsign && !strlen(options.request_file)) {
        OPENER_TRACE_INFO("  . Loading the subject key ...");
        fflush(stdout);

        ret = mbedtls_pk_parse_keyfile(&loaded_subject_key, options.subject_key,
                                       options.subject_pwd, mbedtls_ctr_drbg_random, &ctr_drbg);
        if (ret != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_pk_parse_keyfile "
                           "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
            goto exit;
        }

        OPENER_TRACE_INFO(" ok\n");
    }

    OPENER_TRACE_INFO("  . Loading the issuer key ...");
    fflush(stdout);

    ret = mbedtls_pk_parse_keyfile(&loaded_issuer_key, options.issuer_key,
                                   options.issuer_pwd, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        mbedtls_strerror(ret, buf, sizeof(buf));
        OPENER_TRACE_INFO(" failed\n  !  mbedtls_pk_parse_keyfile "
                       "returned -x%02x - %s\n\n", (unsigned int) -ret, buf);
        goto exit;
    }

    // Check if key and issuer certificate match
    //
    if (strlen(options.issuer_crt)) {
        if (mbedtls_pk_check_pair(&issuer_crt.pk, issuer_key,
                                  mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
            OPENER_TRACE_INFO(" failed\n  !  issuer_key does not match "
                           "issuer certificate\n\n");
            goto exit;
        }
    }

    OPENER_TRACE_INFO(" ok\n");

    if (options.selfsign) {
        options.subject_name = options.issuer_name;
        subject_key = issuer_key;
    }

    mbedtls_x509write_crt_set_subject_key(&crt, subject_key);
    mbedtls_x509write_crt_set_issuer_key(&crt, issuer_key);

    /*
     * 1.0. Check the names for validity
     */
    if ((ret = mbedtls_x509write_crt_set_subject_name(&crt, options.subject_name)) != 0) {
        mbedtls_strerror(ret, buf, sizeof(buf));
        OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509write_crt_set_subject_name "
                       "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
        goto exit;
    }

    if ((ret = mbedtls_x509write_crt_set_issuer_name(&crt, options.issuer_name)) != 0) {
        mbedtls_strerror(ret, buf, sizeof(buf));
        OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509write_crt_set_issuer_name "
                       "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
        goto exit;
    }

    OPENER_TRACE_INFO("  . Setting certificate values ...");
    fflush(stdout);

    mbedtls_x509write_crt_set_version(&crt, options.version);
    mbedtls_x509write_crt_set_md_alg(&crt, options.md);

    ret = mbedtls_x509write_crt_set_serial_raw(&crt, serial, serial_len);
    if (ret != 0) {
        mbedtls_strerror(ret, buf, sizeof(buf));
        OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509write_crt_set_serial_raw "
                       "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
        goto exit;
    }

    ret = mbedtls_x509write_crt_set_validity(&crt, options.not_before, options.not_after);
    if (ret != 0) {
        mbedtls_strerror(ret, buf, sizeof(buf));
        OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509write_crt_set_validity "
                       "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
        goto exit;
    }

    OPENER_TRACE_INFO(" ok\n");

    if (options.version == MBEDTLS_X509_CRT_VERSION_3 &&
        options.basic_constraints != 0) {
        OPENER_TRACE_INFO("  . Adding the Basic Constraints extension ...");
        fflush(stdout);

        ret = mbedtls_x509write_crt_set_basic_constraints(&crt, options.is_ca,
                                                          options.max_pathlen);
        if (ret != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  x509write_crt_set_basic_constraints "
                           "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
            goto exit;
        }

        OPENER_TRACE_INFO(" ok\n");
    }

#if defined(MBEDTLS_MD_CAN_SHA1)
    if (options.version == MBEDTLS_X509_CRT_VERSION_3 &&
        options.subject_identifier != 0) {
        OPENER_TRACE_INFO("  . Adding the Subject Key Identifier ...");
        fflush(stdout);

        ret = mbedtls_x509write_crt_set_subject_key_identifier(&crt);
        if (ret != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509write_crt_set_subject"
                           "_key_identifier returned -0x%04x - %s\n\n",
                           (unsigned int) -ret, buf);
            goto exit;
        }

        OPENER_TRACE_INFO(" ok\n");
    }

    if (options.version == MBEDTLS_X509_CRT_VERSION_3 &&
        options.authority_identifier != 0) {
        OPENER_TRACE_INFO("  . Adding the Authority Key Identifier ...");
        fflush(stdout);

        ret = mbedtls_x509write_crt_set_authority_key_identifier(&crt);
        if (ret != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509write_crt_set_authority_"
                           "key_identifier returned -0x%04x - %s\n\n",
                           (unsigned int) -ret, buf);
            goto exit;
        }

        OPENER_TRACE_INFO(" ok\n");
    }
#endif /* MBEDTLS_MD_CAN_SHA1 */

    if (options.version == MBEDTLS_X509_CRT_VERSION_3 &&
        options.key_usage != 0) {
        OPENER_TRACE_INFO("  . Adding the Key Usage extension ...");
        fflush(stdout);

        ret = mbedtls_x509write_crt_set_key_usage(&crt, options.key_usage);
        if (ret != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509write_crt_set_key_usage "
                           "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
            goto exit;
        }

        OPENER_TRACE_INFO(" ok\n");
    }

    if (options.ext_key_usage) {
        OPENER_TRACE_INFO("  . Adding the Extended Key Usage extension ...");
        fflush(stdout);

        ret = mbedtls_x509write_crt_set_ext_key_usage(&crt, options.ext_key_usage);
        if (ret != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(
                " failed\n  !  mbedtls_x509write_crt_set_ext_key_usage returned -0x%02x - %s\n\n",
                (unsigned int) -ret,
                buf);
            goto exit;
        }

        OPENER_TRACE_INFO(" ok\n");
    }

    if (options.version == MBEDTLS_X509_CRT_VERSION_3 &&
        options.ns_cert_type != 0) {
        OPENER_TRACE_INFO("  . Adding the NS Cert Type extension ...");
        fflush(stdout);

        ret = mbedtls_x509write_crt_set_ns_cert_type(&crt, options.ns_cert_type);
        if (ret != 0) {
            mbedtls_strerror(ret, buf, sizeof(buf));
            OPENER_TRACE_INFO(" failed\n  !  mbedtls_x509write_crt_set_ns_cert_type "
                           "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
            goto exit;
        }

        OPENER_TRACE_INFO(" ok\n");
    }

    /*
     * 1.2. Writing the certificate
     */
    OPENER_TRACE_INFO("  . Writing the certificate...");
    fflush(stdout);

    if ((ret = write_certificate(&crt, options.output_file,
                                 mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
        mbedtls_strerror(ret, buf, sizeof(buf));
        OPENER_TRACE_INFO(" failed\n  !  write_certificate -0x%04x - %s\n\n",
                       (unsigned int) -ret, buf);
        goto exit;
    }

    OPENER_TRACE_INFO(" ok\n\n");

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:
#if defined(MBEDTLS_X509_CSR_PARSE_C)
    mbedtls_x509_csr_free(&csr);
#endif /* MBEDTLS_X509_CSR_PARSE_C */
    mbedtls_x509_crt_free(&issuer_crt);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&loaded_subject_key);
    mbedtls_pk_free(&loaded_issuer_key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    //mbedtls_exit(exit_code);
    return exit_code;
}
