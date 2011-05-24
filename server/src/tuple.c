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

#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <syslog.h>

#include <cgilib6/util.h>

#include "../../common/src/greylist.h"
#include "../../common/src/util.h"
#include "../../common/src/bisearch.h"
#include "tuple.h"
#include "globals.h"

#if !defined(NDEBUG)
#  define D(x)	x
#endif

/**********************************************************************/

int tuple_look_ift(const void *restrict key,const void *restrict values)
{
  const struct tuple *k;
  const struct tuple *v;
  size_t              len;
  int                 rc;

  k  = key;
  v  = *(struct tuple *const *)values;
  
  if ((rc = memcmp(k->ip,v->ip,sizeof(k->ip))) != 0) return rc;
  
  len = (k->fromsize < v->fromsize) ? k->fromsize : v->fromsize;
  if ((rc = memcmp(k->from,v->from,len)) != 0) return rc;
  
  if (k->fromsize < v->fromsize)
    return -1;
  else if (k->fromsize > v->fromsize)
    return 1;
  
  len = (k->tosize < v->tosize) ? k->tosize : v->tosize;
  if ((rc = memcmp(k->to,v->to,len)) != 0) return rc;
  
  if (k->tosize < v->tosize)
    return -1;
  else if (k->tosize > v->tosize)
    return 1;
  else
    return 0;
}

/************************************************************************/

Tuple tuple_search(Tuple key,size_t *pidx)
{
  bisearch__t item;
  
  assert(key  != NULL);
  assert(pidx != NULL);
  
  g_tuples_read++;
  g_tuples_read_cucurrent++;
  
  item  = bisearch(key,g_tuplespace,g_poolnum,sizeof(Tuple),tuple_look_ift);
  *pidx = item.idx;
  return (item.datum == NULL) ? NULL : *(Tuple *)item.datum;
}

/******************************************************************/

Tuple tuple_allocate(void)
{
  assert(g_poolnum <= c_poolmax);
  
  if (g_poolnum == c_poolmax)
  {
    (*cv_report)(LOG_ERR,"too many requests-attempting to clean house");
    tuple_expire(time(NULL));
    if (g_poolnum == c_poolmax)
    {
      size_t i;
      
      (*cv_report)(LOG_ERR,"too many requests-cleaning house failed-starting over");
      g_poolnum    = 0;
      g_tuples_low = 0;	/* reset the low count automatically */
      
      for (i = 0 ; i < c_poolmax ; i++)
        g_tuplespace[i] = &g_pool[i];
    }
  }
  
  return g_tuplespace[g_poolnum];  
}

/*****************************************************************/

void tuple_add(Tuple rec,size_t index)
{
  assert(g_poolnum <  c_poolmax);
  assert(index     <= g_poolnum);
  assert(rec       != NULL);
  assert(rec       >= &g_pool[0]);
  assert(rec       <= &g_pool[c_poolmax - 1]);
  assert(rec->pad  == 0xDECAFBAD);
  
  g_tuples_write++;
  g_tuples_write_cucurrent++;
  
  memmove(
  	&g_tuplespace[index + 1],
  	&g_tuplespace[index],
  	(g_poolnum - index) * sizeof(Tuple)
    );

  g_tuplespace[index] = rec;
  g_poolnum++;
  
  if (g_poolnum > g_tuples_high)
    g_tuples_high = g_poolnum;
}

/*******************************************************************/

void tuple_expire(time_t Tao)
{
  size_t i;
  size_t j;

  g_tuples_write++;
  g_tuples_write_cucurrent++;
  
  for (i = j = 0 ; i < g_poolnum ; i++)
  {
    /*----------------------------------------------
    ; check for expired whitelist entries
    ;----------------------------------------------*/
    
    if (g_tuplespace[i]->f & F_WHITELIST)
    {
      if (difftime(Tao,g_tuplespace[i]->atime) > c_timeout_white)
        g_tuplespace[i]->f |= F_REMOVE;
    }
    
    /*----------------------------------------
    ; check for expired greylist entries
    ;--------------------------------------*/
    
    else
    {
      if (difftime(Tao,g_tuplespace[i]->atime) > c_timeout_grey)
        g_tuplespace[i]->f |= F_REMOVE;
    }
    
    /*-------------------------------------------------
    ; now checked for entries that are being removed,
    ; update metrics accordingly.
    ;-------------------------------------------------*/
    
    if (g_tuplespace[i]->f & F_REMOVE)
    {
      if (g_tuplespace[i]->f & F_WHITELIST)
      {
        g_whitelisted--;
        g_whitelist_expired++;
      }
      else
        g_greylist_expired++;
    }
    else
      g_tuplespace[j++] = g_tuplespace[i];
  }
  
  g_poolnum = j;

  /*------------------------------------------
  ; moved expired entries to the free pool
  ;-------------------------------------------*/
  
  for (i = 0 ; i < c_poolmax ; i++)
  {
    if (g_pool[i].f & F_REMOVE)
    {
      g_tuplespace[j++] = &g_pool[i];
      assert(j < c_poolmax);
    }
  }
  
  if (g_poolnum < g_tuples_low)  g_tuples_low  = g_poolnum;
  if (g_poolnum > g_tuples_high) g_tuples_high = g_poolnum;  
}

/**********************************************************************/

void whitelist_dump(void)
{
  FILE *out;
  
  out = fopen(c_whitefile,"w");
  if (out)
  {
    whitelist_dump_stream(out);
    fclose(out);
  }
  else
    (*cv_report)(LOG_ERR,"whitelist_dump(): fopen(%s,WRITE) = %s",c_whitefile,strerror(errno));
}

/******************************************************************/

void whitelist_dump_stream(FILE *out)
{
  assert(out != NULL);
  
  for (size_t i = 0 ; i < g_poolnum ; i++)
  {
    if ((g_tuplespace[i]->f & (F_WHITELIST | F_REMOVE)) == F_WHITELIST)
    {
      fprintf(
            out,
            "%s %s %s\n",
            ipv4(g_tuplespace[i]->ip),
            (g_tuplespace[i]->fromsize) ? g_tuplespace[i]->from : "-",
            (g_tuplespace[i]->tosize)   ? g_tuplespace[i]->to   : "-"
      	);
    }
  }
}

/********************************************************************/

void tuple_dump(void)
{
  FILE *out;
  
  out = fopen(c_dumpfile,"w");
  if (out)
  {
    tuple_dump_stream(out);
    fclose(out);
  }
  else
    (*cv_report)(LOG_ERR,"tuple_dump(): fopen(%s,WRITE) = %s",c_dumpfile,strerror(errno));
}

/*******************************************************************/

void tuple_dump_stream(FILE *out)
{
  assert(out != NULL);
  
  for (size_t i = 0 ; i < g_poolnum ; i++)
  {
    if ((!g_tuplespace[i]->f & F_WHITELIST))
    {
      fprintf(
        out,
        "%s %s %s %s%s%s%s%s %lu %lu\n",
        ipv4(g_tuplespace[i]->ip),
        (g_tuplespace[i]->fromsize) ? g_tuplespace[i]->from : "-",
        (g_tuplespace[i]->tosize)   ? g_tuplespace[i]->to   : "-",
        (g_tuplespace[i]->f & F_WHITELIST) ? "W" : "-",
        (g_tuplespace[i]->f & F_REMOVE)    ? "D" : "-",
        (g_tuplespace[i]->f & F_TRUNCFROM) ? "F" : "-",
        (g_tuplespace[i]->f & F_TRUNCTO)   ? "T" : "-",
        (g_tuplespace[i]->f & F_IPv6)      ? "6" : "-",
        (unsigned long)g_tuplespace[i]->ctime,
        (unsigned long)g_tuplespace[i]->atime
      );
    }
  }
}

/******************************************************************/

void tuple_all_dump(void)
{
  FILE *out;
  
  out = fopen(c_dumpfile,"w");
  if (out)
  {
    tuple_all_dump_stream(out);
    fclose(out);
  }
  else
    (*cv_report)(LOG_ERR,"tuple_all_dump(): fopen(%s,WRITE) = %s",c_dumpfile,strerror(errno));
}

/******************************************************************/

void tuple_all_dump_stream(FILE *out)
{
  assert(out != NULL);

  for (size_t i = 0 ; i < g_poolnum ; i++)
  {
    fprintf(
        out,
        "%s %s %s %s%s%s%s%s %lu %lu\n",
        ipv4(g_tuplespace[i]->ip),
        (g_tuplespace[i]->fromsize) ? g_tuplespace[i]->from : "-",
        (g_tuplespace[i]->tosize)   ? g_tuplespace[i]->to   : "-",
        (g_tuplespace[i]->f & F_WHITELIST) ? "W" : "-",
        (g_tuplespace[i]->f & F_REMOVE)    ? "D" : "-",
        (g_tuplespace[i]->f & F_TRUNCFROM) ? "F" : "-",
        (g_tuplespace[i]->f & F_TRUNCTO)   ? "T" : "-",
        (g_tuplespace[i]->f & F_IPv6)      ? "6" : "-",
        (unsigned long)g_tuplespace[i]->ctime,
        (unsigned long)g_tuplespace[i]->atime
    );
  }
}

/******************************************************************/

void whitelist_load(void)
{
  struct tuple  tuple;
  FILE         *in;
  time_t        now;
  char         *line;
  size_t        linesize;
  
  line     = NULL;
  linesize = 0;
  now      = time(NULL);
  in       = fopen(c_whitefile,"r");
  
  if (in == NULL)	/* normal condition, if it doesn't exist */
    return;		/* don't sweat about it */
    
  while(getline(&line,&linesize,in) > 0)
  {
    Tuple   stored;
    size_t  idx;
    char   *p;
    char   *n;
    
    memset(&tuple,0,sizeof(struct tuple));
    
    if (empty_string(line)) 
      continue;

    line = trim_space(line);
    
    D(tuple.pad = 0xDECAFBAD;)
    tuple.ip[0] = strtoul(line,&p,10); p++;
    tuple.ip[1] = strtoul(p,&p,10);    p++;
    tuple.ip[2] = strtoul(p,&p,10);    p++;
    tuple.ip[3] = strtoul(p,&p,10);    p++;
    
    n = strchr(p,' ');
    
    /*----------------------------------------------
    ; this shouldn't happen, but if it does, just skip
    ; this line and continue
    ;------------------------------------------------*/
    
    if (n == NULL)
      continue;
    
    *n++ = '\0';
   
    if (strcmp(p,"-") == 0)
    {
      tuple.from[0]   = '\0';
      tuple.fromsize = 0;
    }
    else
    {
      strncpy(tuple.from,p,sizeof(tuple.from) - 1);
      tuple.fromsize = strlen(tuple.from);
    }

    if (strcmp(n,"-") == 0)
    {
      tuple.to[0]  = '\0';
      tuple.tosize = 0;
    }
    else
    {
      strncpy(tuple.to,  n,sizeof(tuple.to)   - 1);
      tuple.tosize = strlen(tuple.to);
    }

    tuple.ctime    = tuple.atime = now;

    (*cv_report)(
        LOG_DEBUG,
        "Adding [%s , %s , %s]",
	ipv4(tuple.ip),
	tuple.from,
	tuple.to
      );
    
    stored = tuple_search(&tuple,&idx);
    
    if (stored == NULL)
    {
      stored = tuple_allocate();
      memcpy(stored,&tuple,sizeof(struct tuple));
      stored->f |= F_WHITELIST;
      tuple_add(stored,idx);
      g_whitelisted++;
    }
    else
    {
      (*cv_report)(LOG_DEBUG,"FOUND!");
      stored->atime = now;
      
      if ((stored->f & F_WHITELIST) == 0)
      {
        stored->f |= F_WHITELIST;
        g_whitelisted++;
      }
    }    
  }
  
  free(line);
  
  /*--------------------------------------------------------
  ; we've started the program, and potentially seeded the
  ; tuple space.  Set the low and high mark to our current
  ; values.
  ;----------------------------------------------------------*/
  
  g_tuples_low  = g_poolnum;
  g_tuples_high = g_poolnum;
  
  fclose(in);
}

/**********************************************************************/
