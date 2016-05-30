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

#ifndef SERVER_H
#define SERVER_H

#include <time.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../common/greylist.h"

/************************************************************/

#define F_WHITELIST     (1uL << 0)
#define F_REMOVE        (1uL << 1)
#define F_TRUNCFROM     (1uL << 2)
#define F_TRUNCTO       (1uL << 3)
#define F_IPv6          (1uL << 4)

/************************************************************/

typedef struct tuple
{
  time_t       ctime;
  time_t       atime;
  size_t       fromsize;
  size_t       tosize;
  unsigned int pad;
  flags        f;
  uint8_t      ip  [16];
  char         from[108];
  char         to  [108];
} *Tuple;

struct request
{
  int                         sock;
  time_t                      now;
  struct sockaddr             remote;
  socklen_t                   rsize;
  union greylist_all_packets  packet;
  struct greylist_request    *glr;
  size_t                      size;
};

#endif

