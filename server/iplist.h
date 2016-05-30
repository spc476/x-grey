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

#ifndef IPLIST_H
#define IPLIST_H

#include <stdio.h>
#include <stddef.h>
#include "../common/greylist.h"

struct ipblock
{
  size_t  size;
  uint8_t addr[16];
  uint8_t mask[16];
  int     smask;
  size_t  count;
  int     cmd;
};

struct ipnode
{
  struct ipnode *parent;
  struct ipnode *zero;
  struct ipnode *one;
  size_t         count;
  int            match;
};

/******************************************************/

int		 iplist_read		(const char *);
int		 iplist_check		(uint8_t *,size_t);
void		 iplist_dump		(void);
void		 iplist_dump_stream	(FILE *);
struct ipblock	*ip_table		(size_t *);
int		 ip_match		(uint8_t *,size_t);
int		 ip_add_sm		(uint8_t *,size_t,int,int);
void		 ip_print		(FILE *);

#endif
