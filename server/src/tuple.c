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

#include <string.h>
#include <stdlib.h>

#include <syslog.h>

#include <cgilib/stream.h>
#include <cgilib/errors.h>
#include <cgilib/util.h>
#include <cgilib/memory.h>
#include <cgilib/ddt.h>

#include "../../common/src/greylist.h"
#include "../../common/src/util.h"
#include "tuple.h"
#include "globals.h"

/**********************************************************************/

int tuple_cmp_ft(const void *left,const void *right)
{
  const struct tuple *l = left;
  const struct tuple *r = right;
  int                 rc;
  
  ddt(left   != NULL);
  ddt(right  != NULL);
  ddt(l->pad == 0xDECAFBAD);
  ddt(r->pad == 0xDECAFBAD);

  if (l->fromsize < r->fromsize)
    return(-1);
  else if (l->fromsize > r->fromsize)
    return(1);
  
  if ((rc = memcmp(l->from,r->from,l->fromsize)) != 0) return(rc);
  
  if (l->tosize < r->tosize)
    return(-1);
  else if (l->tosize > r->tosize)
    return(1);
  
  rc = memcmp(l->to,r->to,l->tosize);
  return(rc);
}

/*********************************************************************/
  
int tuple_cmp_ift(const void *left,const void *right)
{
  const struct tuple *l = left;
  const struct tuple *r = right;
  int                 rc;
  
  ddt(left   != NULL);
  ddt(right  != NULL);
  ddt(l->pad == 0xDECAFBAD);
  ddt(r->pad == 0xDECAFBAD);
  
  if ((rc = memcmp(l->ip,r->ip,sizeof(l->ip))) != 0) return(rc);
  
  if (l->fromsize < r->fromsize)
    return(-1);
  else if (l->fromsize > r->fromsize)
    return(1);
  
  if ((rc = memcmp(l->from,r->from,l->fromsize)) != 0) return(rc);
  
  if (l->tosize < r->tosize)
    return(-1);
  else if (l->tosize > r->tosize)
    return(1);
  
  rc = memcmp(l->to,r->to,l->tosize);
  return(rc);
}

/*************************************************************************/

int tuple_qsort_cmp(const void *left, const void *right)
{
  const Tuple *l = left;
  const Tuple *r = right;
  
  ddt(left  != NULL);
  ddt(right != NULL);
  
  return(tuple_cmp_ift(*l,*r));
}

/**************************************************************************/

Tuple tuple_search(Tuple key,size_t *pidx)
{
  size_t low;
  size_t high;
  size_t delta;
  size_t mid;
  int    q;
  
  ddt(key      != NULL);
  ddt(pidx     != NULL);
  ddt(key->pad == 0xDECAFBAD);
  
  if (g_poolnum == 0)
  {
    *pidx = 0;
    return(NULL);
  }
  
  low = 0;
  high = g_poolnum - 1;
  
  while(1)
  {
    ddt(high >= low);
    ddt(high <  g_poolnum);
    
    delta = high - low;
    mid   = low + (delta / 2);
    q     = tuple_cmp_ift(key,g_tuplespace[mid]);
    
    if (q < 0)
    {
      if (delta == 0)
      {
        *pidx = mid;
        return(NULL);
      }
      
      high = mid - 1;
      if (high > g_poolnum)
      {
        *pidx = mid;
        return(NULL);
      }
      
      if (high < low)
      {
        *pidx = mid;
        return(NULL);
      }
    }
    else if (q == 0)
    {
      *pidx = mid;
      ddt(g_tuplespace[mid]->pad == 0xDECAFBAD);
      return(g_tuplespace[mid]);
    }
    else
    {
      if (delta == 0)
      {
        *pidx = mid + 1;
        return(NULL);
      }
      
      low = mid + 1;
      
      if (low > high)
      {
        *pidx = mid;
        return(NULL);
      }
    }
  }
}

/******************************************************************/

Tuple tuple_allocate(void)
{
  ddt(g_poolnum <= c_poolmax);
  
  if (g_poolnum == c_poolmax)
  {
    (*cv_report)(LOG_ERR,"","too many requests-attempting to clean house");
    tuple_expire(time(NULL));
    if (g_poolnum == c_poolmax)
    {
      size_t i;
      
      (*cv_report)(LOG_ERR,"","too many requests-cleaning house failed-starting over");
      g_poolnum = 0;
      
      for (i = 0 ; i < c_poolmax ; i++)
        g_tuplespace[i] = &g_pool[i];
    }
  }
  
  return g_tuplespace[g_poolnum];  
}

/*****************************************************************/

void tuple_add(Tuple rec,size_t index)
{
  ddt(g_poolnum <  c_poolmax);
  ddt(index     <= g_poolnum);
  ddt(rec       != NULL);
  ddt(rec       >= &g_pool[0]);
  ddt(rec       <= &g_pool[c_poolmax - 1]);
  ddt(rec->pad  == 0xDECAFBAD);
  
  memmove(
  	&g_tuplespace[index + 1],
  	&g_tuplespace[index],
  	(g_poolnum - index) * sizeof(Tuple)
    );

  g_tuplespace[index] = rec;
  g_poolnum++;  
}

/*******************************************************************/

void tuple_expire(time_t Tao)
{
  size_t i;
  size_t j;

  for (i = 0 ; i < g_poolnum ; i++)
  {
    if (g_tuplespace[i]->f & F_WHITELIST)
    {
      if (difftime(Tao,g_tuplespace[i]->atime) < c_timeout_white)
        g_tuplespace[i]->f |= F_REMOVE;
    }
    else
    {
      if (difftime(Tao,g_tuplespace[i]->atime) < c_timeout_grey)
        g_tuplespace[i]->f |= F_REMOVE;
    }
  }
  
  for (i = j = 0 ; i < g_poolnum ; i++)
  {
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
  
  for (i = 0 ; i < c_poolmax ; i++)
  {
    if (g_tuplespace[i]->f & F_REMOVE)
    {
      g_tuplespace[j++] = &g_pool[i];
      ddt(j < c_poolmax);
    }
  } 
}

/**********************************************************************/

int whitelist_dump(void)
{
  Stream out;
  
  out = FileStreamWrite(c_whitefile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_whitefile);
    return(ERR_ERR);
  }

  whitelist_dump_stream(out);
  
  StreamFree(out);
  return(ERR_OKAY);
}

/******************************************************************/

int whitelist_dump_stream(Stream out)
{
  size_t i;

  ddt(out != NULL);
  
  for (i = 0 ; (!StreamEOF(out)) && (i < g_poolnum) ; i++)
  {
    if ((g_tuplespace[i]->f & (F_WHITELIST | F_REMOVE)) == F_WHITELIST)
    {
      LineSFormat(
      		out,
      		"$ $ $",
      		"%a %b %c\n",
      		ipv4(g_tuplespace[i]->ip),
      		(g_tuplespace[i]->fromsize) ? g_tuplespace[i]->from : "-",
      		(g_tuplespace[i]->tosize)   ? g_tuplespace[i]->to   : "-"
      	);
    }
  }
  return(ERR_OKAY);
}

/********************************************************************/

int tuple_dump(void)
{
  Stream out;
  
  out = FileStreamWrite(c_dumpfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_dumpfile);
    return(ERR_ERR);
  }
  
  tuple_dump_stream(out);
  StreamFree(out);
  return(ERR_OKAY);
}

/*******************************************************************/

int tuple_dump_stream(Stream out)
{
  size_t i;
  
  ddt(out != NULL);
  
  for (i = 0 ; (!StreamEOF(out)) && (i < g_poolnum) ; i++)
  {
    if ((!g_tuplespace[i]->f & F_WHITELIST))
    {
      LineSFormat(
      	out,
        "$ $ $ $ $ $ $ $ L L",
        "%a %b %c %d%e%f%g%h %i %j\n",
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
  return(ERR_OKAY);
}

/******************************************************************/

int tuple_all_dump(void)
{
  Stream out;
  
  out = FileStreamWrite(c_dumpfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_dumpfile);
    return(ERR_ERR);
  }
  
  tuple_all_dump_stream(out);
  StreamFree(out);
  return(ERR_OKAY);
}

/******************************************************************/

int tuple_all_dump_stream(Stream out)
{
  size_t i;

  ddt(out != NULL);

  for (i = 0 ; (!StreamEOF(out)) && (i < g_poolnum) ; i++)
  {
    LineSFormat(
        out,
        "$ $ $ $ $ $ $ $ L L",
        "%a %b %c %d%e%f%g%h %i %j\n",
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
  return(ERR_OKAY);
}

/******************************************************************/

int whitelist_load(void)
{
  struct tuple tuple;
  Stream       in;
  time_t       now;
  
  now = time(NULL);
  in  = FileStreamRead(c_whitefile);
  if (in == NULL)	/* normal condition, if it doesn't exist */
    return(ERR_OKAY);	/* don't sweat about it */
    
  while(!StreamEOF(in))
  {
    Tuple   stored;
    size_t  idx;
    char   *line;
    char   *p;
    char   *n;
    
    memset(&tuple,0,sizeof(struct tuple));
    
    line = LineSRead(in);
    if (empty_string(line)) 
    {
      MemFree(line);
      continue;
    }

    D(tuple.pad = 0xDECAFBAD);
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
    {
      MemFree(line);
      continue;
    }
    
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
	"$ $ $",
	"Adding [%a , %b , %c]",
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
      (*cv_report)(LOG_DEBUG,"","FOUND!");
      stored->atime = now;
      
      if ((stored->f & F_WHITELIST) == 0)
      {
        stored->f |= F_WHITELIST;
        g_whitelisted++;
      }
    }
    
    MemFree(line);
  }
  
  StreamFree(in);
  return(ERR_OKAY);
}

/**********************************************************************/

