/***************************************************************************
*
* Copyright 2010 by Sean Conner.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*************************************************************************/

#ifndef CRC_32_H
#define CRC_32_H

#define INIT_CRC32	0xFFFFFFFF

#if (UINT_MAX == 4294967295UL)
  typedef unsigned int CRC32;
#elif (ULONG_MAX == 4294967295UL)
  typedef unsigned long CRC32;
#elif (USHORT_MAX == 4294967295UL)
  typedef unsigned short CRC32;
#elif (UCHAR_MAX == 4294967295UL)
  typedef unsigned char CRC32;
#else
# error No integral type is 32 bits on this platform
#endif

CRC32	crc32	(CRC32,void const *,size_t);

#endif

