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
#include "../common/greylist.h"
#include "../common/globals.h"
#include "server.h"

/**********************************************************/

extern int   tuple_cmp_ft          (void const *,void const *);
extern int   tuple_cmp_ift         (void const *restrict,void const *restrict);
extern int   tuple_qsort_cmp       (void const *,void const *);
extern Tuple tuple_search          (Tuple,size_t *);
extern Tuple tuple_allocate        (void);
extern void  tuple_add             (Tuple,size_t);
extern void  tuple_expire          (time_t);
extern void  tuple_dump            (void);
extern void  tuple_dump_stream     (FILE *);
extern void  tuple_all_dump        (void);
extern void  tuple_all_dump_stream (FILE *);
extern void  whitelist_dump        (void);
extern void  whitelist_dump_stream (FILE *);
extern void  whitelist_load        (void);
extern void  log_tuple             (Tuple,int,int);

#endif
