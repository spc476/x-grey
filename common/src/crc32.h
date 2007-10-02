
#ifndef CRC_32_H
#define CRC_32_H

#define INIT_CRC32	0xFFFFFFFF

typedef unsigned long CRC32;

CRC32	crc32	(CRC32,const void *,size_t);

#endif

