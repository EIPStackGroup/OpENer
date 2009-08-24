/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

static char dumpbuf[256];

void dump(unsigned char *p, int size)
  {
    int i;
    char *b;

    while (size > 0)
      {
        b = dumpbuf;
        // b += sprintf(b,"%08x: ",(int)p);
        for (i = 0; i < 16; i++)
          {
            if (i < size)
              b += sprintf(b, "%02x ", p[i]);
            else
              b += sprintf(b, "   ");
          }
        b += sprintf(b, " |");
        for (i = 0; i < 16; i++)
          {
            if (i < size)
              {
                if (' ' <= p[i] && p[i] < 0x7f)
                  b += sprintf(b, "%c", p[i]);
                else
                  b += sprintf(b, ".");
              }
          }
        p += 16;
        size -= 16;
        printf("%s\n", dumpbuf);
      }
  }
