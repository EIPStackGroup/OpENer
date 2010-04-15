/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include "endianconv.h"

/* THESE ROUTINES MODIFY THE BUFFER POINTER*/

/* little-endian-to-host unsigned 16 bit*/

EIP_UINT16 ltohs(EIP_UINT8 ** pa_buf)
  {
    unsigned char *p = (unsigned char *)*pa_buf;
    EIP_UINT16 data = p[0] | p[1]<<8;
    *pa_buf += 2;
    return data;
  }

/*   UINT32 ltohl(INT8 **pa_buf)
 *   reeds UINT32 from *pa_buf and converts little endian to host.
 *      *pa_buf pointer where data should be reed. 
 *  return value
 */
EIP_UINT32 ltohl(EIP_UINT8 ** pa_buf)
  {
    unsigned char *p = (unsigned char *)*pa_buf;
    EIP_UINT32 data = p[0] | p[1]<<8 | p[2]<<16 | p[3]<<24;
    *pa_buf += 4;
    return data;
  }

/*   void htols(UINT16 data, UINT8 **pa_buf)
 *   converts UINT16 data from host to little endian an writes it to pa_buf.
 *      data value to be written
 *      *pa_buf pointer where data should be written.
 */
void htols(EIP_UINT16 data, EIP_UINT8 ** pa_buf)
  {
    unsigned char *p = (unsigned char *)*pa_buf;

    p[0] = (unsigned char)data;
    p[1] = (unsigned char)(data >> 8);
    *pa_buf += 2;
  }

/*   void htoll(UINT32 data, INT8 **pa_buf)
 *   converts UINT32 data from host to little endian an writes it to pa_buf.
 *      data value to be written
 *      *pa_buf pointer where data should be written.
 */
void htoll(EIP_UINT32 data, EIP_UINT8 ** pa_buf)
  {
    unsigned char *p = (unsigned char *)*pa_buf;

    p[0] = (unsigned char)data;
    p[1] = (unsigned char)(data >> 8);
    p[2] = (unsigned char)(data >> 16);
    p[3] = (unsigned char)(data >> 24);
    *pa_buf += 4;
  }
