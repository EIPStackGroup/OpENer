/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include "endianconv.h"

int g_nOpENerPlatformEndianess = -1;

/* THESE ROUTINES MODIFY THE BUFFER POINTER*/

/* little-endian-to-host unsigned 16 bit*/

EIP_UINT16
ltohs(EIP_UINT8 ** pa_buf)
{
  unsigned char *p = (unsigned char *) *pa_buf;
  EIP_UINT16 data = p[0] | p[1] << 8;
  *pa_buf += 2;
  return data;
}

/*   UINT32 ltohl(INT8 **pa_buf)
 *   reeds UINT32 from *pa_buf and converts little endian to host.
 *      *pa_buf pointer where data should be reed. 
 *  return value
 */EIP_UINT32
ltohl(EIP_UINT8 ** pa_buf)
{
  unsigned char *p = (unsigned char *) *pa_buf;
  EIP_UINT32 data = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
  *pa_buf += 4;
  return data;
}

/*   void htols(UINT16 data, UINT8 **pa_buf)
 *   converts UINT16 data from host to little endian an writes it to pa_buf.
 *      data value to be written
 *      *pa_buf pointer where data should be written.
 */
void
htols(EIP_UINT16 data, EIP_UINT8 ** pa_buf)
{
  unsigned char *p = (unsigned char *) *pa_buf;

  p[0] = (unsigned char) data;
  p[1] = (unsigned char) (data >> 8);
  *pa_buf += 2;
}

/*   void htoll(UINT32 data, INT8 **pa_buf)
 *   converts UINT32 data from host to little endian an writes it to pa_buf.
 *      data value to be written
 *      *pa_buf pointer where data should be written.
 */
void
htoll(EIP_UINT32 data, EIP_UINT8 ** pa_buf)
{
  unsigned char *p = (unsigned char *) *pa_buf;

  p[0] = (unsigned char) data;
  p[1] = (unsigned char) (data >> 8);
  p[2] = (unsigned char) (data >> 16);
  p[3] = (unsigned char) (data >> 24);
  *pa_buf += 4;
}

#ifdef OPENER_SUPPORT_64BIT_DATATYPES

EIP_UINT64
ltoh64(EIP_UINT8 ** pa_pnBuf)
{
  EIP_UINT8 *pnBuffer = *pa_pnBuf;
  EIP_UINT64 unData =
      ((((EIP_UINT64) pnBuffer[0]) << 56) & 0xFF00000000000000LL)
          + ((((EIP_UINT64) pnBuffer[1]) << 48) & 0x00FF000000000000LL)
          + ((((EIP_UINT64) pnBuffer[2]) << 40) & 0x0000FF0000000000LL)
          + ((((EIP_UINT64) pnBuffer[3]) << 32) & 0x000000FF00000000LL)
          + ((((EIP_UINT64) pnBuffer[4]) << 24) & 0x00000000FF000000)
          + ((((EIP_UINT64) pnBuffer[5]) << 16) & 0x0000000000FF0000)
          + ((((EIP_UINT64) pnBuffer[6]) << 8) & 0x000000000000FF00)
          + (((EIP_UINT64) pnBuffer[7]) & 0x00000000000000FF);
  (*pa_pnBuf) += 8;
  return unData;
}

void
htol64(EIP_UINT64 pa_unData, EIP_UINT8 ** pa_pnBuf)
{
  EIP_UINT8 *pnBuffer = *pa_pnBuf;
  pnBuffer[0] = (EIP_UINT8) (pa_unData >> 56) & 0xFF;
  pnBuffer[1] = (EIP_UINT8) (pa_unData >> 48) & 0xFF;
  pnBuffer[2] = (EIP_UINT8) (pa_unData >> 40) & 0xFF;
  pnBuffer[3] = (EIP_UINT8) (pa_unData >> 32) & 0xFF;
  pnBuffer[4] = (EIP_UINT8) (pa_unData >> 24) & 0xFF;
  pnBuffer[5] = (EIP_UINT8) (pa_unData >> 16) & 0xFF;
  pnBuffer[6] = (EIP_UINT8) (pa_unData >> 8) & 0xFF;
  pnBuffer[7] = (EIP_UINT8) (pa_unData) & 0xFF;
  (*pa_pnBuf) += 8;
}

#endif

void
encapsulateIPAdress(EIP_UINT16 pa_unPort, EIP_UINT32 pa_unAddr,
    EIP_BYTE *pa_acCommBuf)
{

  if (OPENER_LITTLE_ENDIAN_PLATFORM == g_nOpENerPlatformEndianess)
    {
      htols(htons(AF_INET), &pa_acCommBuf);
      htols(htons(pa_unPort), &pa_acCommBuf);
      htoll(pa_unAddr, &pa_acCommBuf);

    }
  else
    {
      if (OPENER_BIG_ENDIAN_PLATFORM == g_nOpENerPlatformEndianess)
        {
          pa_acCommBuf[0] = (unsigned char) (AF_INET >> 8);
          pa_acCommBuf[1] = (unsigned char) AF_INET;
          pa_acCommBuf += 2;

          pa_acCommBuf[0] = (unsigned char) (pa_unPort >> 8);
          pa_acCommBuf[1] = (unsigned char) pa_unPort;
          pa_acCommBuf += 2;

          pa_acCommBuf[3] = (unsigned char) pa_unAddr;
          pa_acCommBuf[2] = (unsigned char) (pa_unAddr >> 8);
          pa_acCommBuf[1] = (unsigned char) (pa_unAddr >> 16);
          pa_acCommBuf[0] = (unsigned char) (pa_unAddr >> 24);
        }
    }
}

void
determineEndianess()
{
  int i = 1;
  char *p = (char *) &i;
  if (p[0] == 1)
    {
      g_nOpENerPlatformEndianess = OPENER_LITTLE_ENDIAN_PLATFORM;
    }
  else
    {
      g_nOpENerPlatformEndianess = OPENER_BIG_ENDIAN_PLATFORM;
    }
}

int
getEndianess()
{
  return g_nOpENerPlatformEndianess;
}
