/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <trace.h>

int __errno(void)
  {
    return 0;
  }

void raise(void)
  {
    while (1)
      ;
  }

void abort(void)
  {
    while (1)
      ;
  }

void BUG(char *s)
  {
    while (1)
      ;
  }

static char dumpbuf[256];

void __assert_func(const char *__file, int __line, const char *__function,
    const char *__assertion)
  {
    snprintf(dumpbuf, 100,
        "assert(%s) failed in function %s, file %s, at line %d\n", __assertion,
        __function, __file, __line);

    OPENER_TRACE_ERR(dumpbuf);
    while (1)
      ;
  }

void dump(unsigned char *p, int size)
  {
    int i;
    char *b;

    while (size>0)
      {
        b = dumpbuf;
        for (i=0; i<16; i++)
          {
            if (i<size)
              b += sprintf(b, "%02x ", p[i]);
            else
              b += sprintf(b, "   ");
          }
        b += sprintf(b, " |");
        for (i=0; i<16; i++)
          {
            if (i<size)
              {
                if (' '<=p[i]&&p[i]<0x7f)
                  b += sprintf(b, "%c", p[i]);
                else
                  b += sprintf(b, ".");
              }
          }
        p += 16;
        size -= 16;
        OPENER_TRACE_INFO("%s\n", dumpbuf);
      }
  }
