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

#ifndef EMAIL_DOMAIN_H
#define EMAIL_DOMAIN_H

#include <stdlib.h>

#include <cgilib/stream.h>

/******************************************************************/

typedef struct emaildomain
{
  char   *text;
  size_t  tsize;
  size_t  count;
  int     cmd;
} *EDomain;

/******************************************************************/

int		edomain_cmp		(EDomain,EDomain);
EDomain		edomain_search_to	(EDomain,size_t *);
EDomain		edomain_search_tod	(EDomain,size_t *);
EDomain		edomain_search_from	(EDomain,size_t *);
EDomain		edomain_search_fromd	(EDomain,size_t *);
EDomain		edomain_search		(EDomain,size_t *,EDomain,size_t);
void		edomain_add_from	(EDomain,size_t);
void		edomain_remove_from	(size_t);
void		edomain_add_fromd	(EDomain,size_t);
void		edomain_remove_fromd	(size_t);
void		edomain_add_to		(EDomain,size_t);
void		edomain_remove_to	(size_t);
void		edomain_add_tod		(EDomain,size_t);
void		edomain_remove_tod	(size_t);
int		to_dump			(void);
int		tod_dump		(void);
int		from_dump		(void);
int		fromd_dump		(void);
int		to_dump_stream		(Stream);
int		tod_dump_stream		(Stream);
int		from_dump_stream	(Stream);
int		fromd_dump_stream	(Stream);
int		to_read			(void);
int		tod_read		(void);
int		from_read		(void);
int		fromd_read		(void);
		
#endif

