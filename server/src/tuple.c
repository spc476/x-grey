
#include <string.h>
#include <stdlib.h>

#include <syslog.h>

#include <cgilib/ddt.h>

#include "../../common/src/graylist.h"
#include "graylist.h"
#include "globals.h"

/**********************************************************************/

int tuple_cmp(const void *left,const void *right)
{
  const struct tuple *l = left;
  const struct tuple *r = right;
  int                 rc;
  
  ddt(left  != NULL);
  ddt(right != NULL);
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
  
  return(tuple_cmp(*l,*r));
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
    mid   = (low + high) / 2;
    q     = tuple_cmp(key,g_tuplespace[mid]);
    
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
      (*cv_report)(LOG_ERR,"","too many requests-cleaning house failed-starting over");
      g_poolnum = 0;
    }
  }
  
  D(g_pool[g_poolnum].pad = 0xDECAFBAD;)
  return(&g_pool[g_poolnum]);
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
  	(g_poolnum - index /*+ 1*/) * sizeof(Tuple)
    );

  g_tuplespace[index] = rec;
  g_poolnum++;  
}

/*******************************************************************/

void tuple_expire(time_t Tao)
{
  size_t i;
  size_t j;

  for (i = j = 0 ; i < g_poolnum ; i++)
  {
    if ((g_pool[i].f & F_WHITELIST))
    {
      if (difftime(Tao,g_pool[i].atime) < c_timeout_white)
        g_pool[j++] = g_pool[i];
      else
        g_whitelist_expired++;
      continue;
    }
    
    if (difftime(Tao,g_pool[i].atime) < c_timeout_gray)
    {
      g_pool[j++] = g_pool[i];
      continue;
    }
    
    g_graylist_expired++;
  }
  
  g_poolnum = j;
  
  for (i = 0 ; i < g_poolnum ; i++)
    g_tuplespace[i] = &g_pool[i];
  
  qsort(g_tuplespace,g_poolnum,sizeof(Tuple *),tuple_qsort_cmp);
}

/**********************************************************************/

