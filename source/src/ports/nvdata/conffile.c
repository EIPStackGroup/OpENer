/*******************************************************************************
 * Copyright (c) 2019, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

/** @file nvqos.c
 *  @brief This file implements the functions to handle QoS object's NV data.
 *
 *  This is only proof-of-concept code. Don't use it in a real product.
 *  Please think about atomic update of the external file and doing something
 *  like fsync() to flush the data really to disk.
 */
/* COMPILATION SWITCHES */
#define ENABLE_VERBOSE  0   /* Enable this to observe internal operation */

/*   INCLUDES          */
#include "conffile.h"

#include <errno.h>
#include <string.h>

#if defined _WIN32
  #include <direct.h>
#else
  #include <sys/stat.h>
  #include <sys/types.h>
#endif /* if defined _WIN32 */

#include "trace.h"
#include "opener_error.h"

/** Base path for configuration file store */
#define CFG_BASE  "nvdata/"

#if ENABLE_VERBOSE != 0
#define VERBOSE(pFile, pFmt, ...)   do { fprintf(pFile, pFmt, ## __VA_ARGS__); \
} while (0)
#else
#define VERBOSE(pFile, pFmt, ...)
#endif


/** @brief Portable wrapper for mkdir(). Internally used by RecMkdir()
 *
 * @param[in] path the full path of the directory to create
 * @return zero on success, otherwise -1 and errno set
 */
static inline int Mkdir(const char *path) {
#ifdef _WIN32
  return _mkdir(path);
#else
  /* The mode of 0777 will be constrained by umask. */
  return mkdir(path, 0777);
#endif
}


static void RecMkdir(char *const p_path) {
  char *sep = strrchr(p_path, '/' );
  if(sep && p_path != sep) {  /* "p_path != sep" avoids mkdir("/")! */
    *sep = '\0';
    RecMkdir(p_path);
    *sep = '/';
  }
  VERBOSE(stdout, " ->mkdir('%s')", p_path);
  if(Mkdir(p_path) && EEXIST != errno) {
    char *error_message = GetErrorMessage(errno);
    OPENER_TRACE_ERR("error while trying to create '%s', %d - %s\n",
                     p_path, errno, error_message);
    FreeErrorMessage(error_message);
  }
}


static FILE *FopenMkdir(char *p_path,
                        char *mode) {
  char *sep = strrchr(p_path, '/' );
  /* In write mode create missing directories. */
  if(sep && 'w' == *mode) {
    *sep = '\0';
    RecMkdir(p_path);
    *sep = '/';
    VERBOSE(stdout, "%s", "\n");
  }

/* Disable VS fopen depreciation warning. */
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif /* _MSC_VER */

  return fopen(p_path, mode);

/* Restore default depreciation warning behavior. */
#ifdef _MSC_VER
#pragma warning(default : 4996)
#endif /* _MSC_VER */
}


/** @brief Open the named configuration file possibly for write operation
 *
 *  @param  write   Open for write?
 *  @param  p_name  pointer to file name string

 *  @return valid file handle: success; NULL: failure, errno set
 *
 *  This function open a configuration file, possibly for write operation,
 *  in the NV data storage directory.
 */

FILE *ConfFileOpen(const bool write,
                   const char *const p_name) {
  char path_buf[64];
  int rc;

  rc = snprintf(path_buf, sizeof path_buf, "%s%s", CFG_BASE, p_name);
  if (rc > 0) {
    return FopenMkdir(path_buf, write ? "w" : "r");
  }
  return NULL;
}

/** @brief Close the configuration file associated with the FILE* given
 *
 *  @param  p_filep pointer to FILE* to close
 *  @return kEipStatusOk: success; kEipStatusError: failure and errno set
 *
 *  Closes the configuration file associated to p_filep. No data
 *  synchronization to disk yet.
 */
EipStatus ConfFileClose(FILE **p_filep) {
  EipStatus eip_status = kEipStatusOk;
  if( 0 != fclose(*p_filep) ) {
    eip_status = kEipStatusError;
  }
  *p_filep = NULL;
  return eip_status;
}
