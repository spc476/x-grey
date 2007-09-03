
/**********************************************************************/

int tuple_cmp(const void *left,const void *right)
{
  Tuple l = left;
  Tuple r = right;
  
  ddt(left  != NULL);
  ddt(right != NULL);
  
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
  
  return(tuple_cmp(*l,*r);
}

/**************************************************************************/

Tuple tuple_search(Tuple key,size_t *pidx)
{
  size_t low;
  size_t high;
  size_t delta;
  size_t mid;
  int    q;
  
  ddt(key  != NULL);
  ddt(pidx != NULL);
  
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
    q     = tuple_cmp(key,g_pool[mid]);
    
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
      return(g_pool[mid]);
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
  ddt(g_poolnum < c_poolmax);
  return(&g_pool[g_poolnum]);
}

/*****************************************************************/

void tuple_add(Tuple rec,size_t index)
{
  ddt(index                  <  g_poolnum + 2);
  ddt(rec                    != NULL);
  ddt(rec                    >= &g_pool[0]);
  ddt(rec                    <= &g_pool[c_poolmax - 1]);
  ddt(g_poolnum - index + 1) <= c_poolmax);
  
  memmove(
  	&g_tuplespace[index + 1],
  	&g_tuplespace[index],
  	(g_poolnum - index + 1) * sizeof(Tuple)
  );
  
  g_tuplespace[index] = rec;
  g_poolnum++;
  
  if (g_poolnum == c_poolmax)
  {
    /* error condition --- possibly remove some entries */
  }
}

/*******************************************************************/

void tuple_expire(time_t Tao)
{
  size_t i;
  size_t count;
  
  for (i = count = 0 ; count < g_poolnum ; count++)
  {
    if ((g_pool[count].f & F_GRAYLIST))
    {
      if (difftime(Tao,g_pool[count].atime) < c_timeout_gray)
      {
        g_pool[i++] = g_pool[count];
        g_graycount_waiting++;
      }
      else
      {
        g_graycount_waiting--;
        g_graycount_expired++;
      }
    }
    
    if ((g_pool[count].f & F_WHITELIST) == F_WHITELIST)
    {
      if (difftime(Tao,g_pool[count].atime) < c_timeout_white)
      {
        g_pool[i++] = g_pool[count];
        g_whitecount_waiting++;
      }
      else
      {
        g_whitecount_waiting--;
        g_whitecount_expired++;
      }
    }    
  }

  g_poolnum = i;

  for (count = 0 ; count < g_poolnum ; count++)
    g_tuplespace[count] = &g_pool[count];
  
  qsort(g_tuplespace,g_poolnum,sizeof(Tuple *),tuple_qsort_cmp);
}

/**********************************************************************/

