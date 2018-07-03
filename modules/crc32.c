/*************************************************************************//*!

\file crc32.c
\brief <b>compute the CRC-32 of a data stream</b>

\copyright (c) 1995-2005 Mark Adler

\author Mark Adler
\author Hans Schmidts

$Revision: 1.1 $
$Date: 2012/02/23 15:06:30 $
$Source: /BSK/C/modules/crc32.c,v $

*//***************************************************************************

History:
V1.1  2012-02-05  BSK/HS
- Initial revision for BSK, based on module from zlib:
  Copyright (C) 1995-2005 Mark Adler
  For conditions of distribution and use, see copyright notice in zlib.h

  Thanks to Rodney Brown <rbrown64@csc.com.au> for his contribution of faster
  CRC methods: exclusive-oring 32 bits of data at a time, and pre-computing
  tables for updating the shift register in one step with three exclusive-ors
  instead of four steps with four exclusive-ors.  This results in about a
  factor of two increase in speed on a Power PC G4 (PPC7455) using gcc -O3.
*/

#include <stddef.h>
#include <limits.h>
#ifndef NDEBUG
  #include <assert.h>
#endif

#include "crc32.h"

#define Z_NULL ((void*)0)
typedef unsigned long uLong;
typedef size_t z_off_t;
#ifndef local
  #define local static
#endif

/* Find a four-byte integer type for crc32_little() and crc32_big(). */
#if (UINT_MAX == 0xffffffffUL)
   typedef unsigned int u4;
#else
  #if (ULONG_MAX == 0xffffffffUL)
     typedef unsigned long u4;
  #else
    #if (USHRT_MAX == 0xffffffffUL)
       typedef unsigned short u4;
    #else
      #error "can't find a four-byte integer type!"
    #endif
  #endif
#endif

#define TBLS 8

/* ========================================================================
 * Tables of CRC-32s of all single-byte values
 */
#include "crc32tab.h"


/* ========================================================================= */
#define DOLIT4 c ^= *buf4++; \
        c = crc_table[3][c & 0xff] ^ crc_table[2][(c >> 8) & 0xff] ^ \
            crc_table[1][(c >> 16) & 0xff] ^ crc_table[0][c >> 24]
#define DOLIT32 DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4

/* ========================================================================= */
unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned len) {
    register u4 c;
    register const u4 *buf4;

    #ifndef NDEBUG
      u4 endian;
      assert (sizeof(void *) == sizeof(ptrdiff_t));
      endian = 1;
      assert (*((unsigned char *)(&endian)));  /* function for little endian systems only */
    #endif


    c = (u4)crc;
    c = ~c;
    while (len && ((ptrdiff_t)buf & 3)) {
        c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
        len--;
    }

    buf4 = (const u4 *)(const void *)buf;
    while (len >= 32) {
        DOLIT32;
        len -= 32;
    }
    while (len >= 4) {
        DOLIT4;
        len -= 4;
    }
    buf = (const unsigned char *)buf4;

    if (len) do {
        c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
    } while (--len);
    c = ~c;
    return (unsigned long)c;
} /* crc32 */


/* eof crc32.c */
