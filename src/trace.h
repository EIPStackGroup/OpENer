/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef TRACE_H_
#define TRACE_H_

#include <opener_user_conf.h>

/*!\file
 * \brief Tracing infrastructure for OpENer
 */

#ifdef OPENER_WITH_TRACES

/*! Enable tracing of error messages. This is the default if no trace level is given*/
#define OPENER_TRACE_LEVEL_ERROR    0x01

/*! Enable tracing of warning messages*/
#define OPENER_TRACE_LEVEL_WARNING  0x02

/*! Enable tracing of state messages*/
#define OPENER_TRACE_LEVEL_STATE    0x04

/*! Enable tracing of info messages*/
#define OPENER_TRACE_LEVEL_INFO     0x08

#ifndef OPENER_TRACE_LEVEL
#warning OPENER_TRACE_LEVEL was not defined setting it to OPENER_TRACE_LEVEL_ERROR
#define OPENER_TRACE_LEVEL OPENER_TRACE_LEVEL_ERROR
#endif

#define OPENER_TRACE_ENABLED      /* can be used for conditional code compilation */

/*! Trace error messages.
 *  In order to activate this trace level set the OPENER_TRACE_LEVEL_ERROR flag
 *  in OPENER_TRACE_LEVEL.
 */
#define OPENER_TRACE_ERR(args...)   do { if (OPENER_TRACE_LEVEL_ERROR & OPENER_TRACE_LEVEL) \
      LOG_TRACE(args); } while(0)

/*! Trace warning messages.
 *  In order to activate this trace level set the OPENER_TRACE_LEVEL_WARNING flag
 *  in OPENER_TRACE_LEVEL.
 */
#define OPENER_TRACE_WARN(args...)  do { if (OPENER_TRACE_LEVEL_WARNING & OPENER_TRACE_LEVEL) \
      LOG_TRACE(args); } while(0)

/*! Trace state messages.
 *  In order to activate this trace level set the OPENER_TRACE_LEVEL_STATE flag
 *  in OPENER_TRACE_LEVEL.
 */
#define OPENER_TRACE_STATE(args...) do { if (OPENER_TRACE_LEVEL_STATE & OPENER_TRACE_LEVEL) \
      LOG_TRACE(args); } while(0)

/*! Trace information messages.
 *  In order to activate this trace level set the OPENER_TRACE_LEVEL_INFO flag
 *  in OPENER_TRACE_LEVEL.
 */
#define OPENER_TRACE_INFO(args...)  do { if (OPENER_TRACE_LEVEL_INFO & OPENER_TRACE_LEVEL) \
      LOG_TRACE(args); } while(0)

#else
/* define the tracing macros empty in order to save space */

#define OPENER_TRACE_ERR(args...)
#define OPENER_TRACE_WARN(args...)
#define OPENER_TRACE_STATE(args...)
#define OPENER_TRACE_INFO(args...)
#endif
/* TRACING *******************************************************************/

#endif /*TRACE_H_*/
