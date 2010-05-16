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

#include <stdio.h>
#include <stdlib.h>

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
void		to_dump			(void);
void		tod_dump		(void);
void		from_dump		(void);
void		fromd_dump		(void);
void		to_dump_stream		(FILE *);
void		tod_dump_stream		(FILE *);
void		from_dump_stream	(FILE *);
void		fromd_dump_stream	(FILE *);
void		to_read			(void);
void		tod_read		(void);
void		from_read		(void);
void		fromd_read		(void);
		
#endif
