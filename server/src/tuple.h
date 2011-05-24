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

#ifndef GREYLIST_H
#define GREYLIST_H

#include <stdio.h>
#include <time.h>
#include "../../common/src/greylist.h"
#include "server.h"

/**********************************************************/

int	 tuple_cmp_ft		(const void *,const void *);
int	 tuple_cmp_ift		(const void *restrict ,const void *restrict);
int	 tuple_qsort_cmp	(const void *,const void *);
Tuple	 tuple_search		(Tuple,size_t *);
Tuple	 tuple_allocate		(void);
void	 tuple_add		(Tuple,size_t);
void	 tuple_expire		(time_t);
void	 tuple_dump		(void);
void	 tuple_dump_stream	(FILE *);
void	 tuple_all_dump		(void);
void	 tuple_all_dump_stream	(FILE *);
void	 whitelist_dump		(void);
void	 whitelist_dump_stream	(FILE *);
void	 whitelist_load		(void);

#endif

