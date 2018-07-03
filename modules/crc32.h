/*************************************************************************//*!

\file crc32.h
\brief <b>CRC32 calculation</b>

\copyright (c) 2012 BSK Datentechnik GmbH

\author Hans Schmidts

$Revision: 1.1 $
$Date: 2012/02/23 15:06:30 $
$Source: /BSK/C/modules/crc32.h,v $

*//***************************************************************************

History:
V1.1  2012-02-05  BSK/HS
- Initial revision
*/

#ifndef _CRC32_H
#define _CRC32_H

#ifdef __cplusplus
extern "C" {
#endif


/* ========================================================================= */
unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned len);


#undef ext
#ifdef __cplusplus
}
#endif
#endif /* _CRC32_H */
