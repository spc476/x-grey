
#include <string.h>

#include <syslog.h>

#include <cgilib/memory.h>
#include <cgilib/stream.h>
#include <cgilib/ddt.h>

#include "../../common/src/globals.h"
#include "../../common/src/util.h"

#include "emaildomain.h"
#include "globals.h"

#define ED_DELTA	100

static void tofrom_dump_stream	(Stream,EDomain,size_t);

/******************************************************************/

int edomain_cmp(EDomain key,EDomain node)
{
  ddt(key  != NULL);
  ddt(node != NULL);
  
#if 0
  if (key->tsize < node->tsize)
    return(-1);
  else if (key->tsize > node->tsize)
    return( 1);
#endif

  return(strcmp(key->text,node->text));
}

/*********************************************************************/

EDomain edomain_search(EDomain key,size_t *pidx,EDomain array,size_t asize)
{
  size_t low;
  size_t high;
  size_t delta;
  size_t mid;
  int    q;
  
  ddt(key   != NULL);
  ddt(pidx  != NULL);
  
  if (asize == 0)
  {
    *pidx = 0;
    return(NULL);
  }
  
  ddt(array != NULL);
  ddt(asize >  0);

  low = 0;
  high = asize - 1;
  
  while(1)
  {
    ddt(high >= low);
    ddt(high <  asize);
    
    delta = high - low;
    mid   = (low + high) / 2;
    q     = edomain_cmp(key,&array[mid]);
    
    if (q < 0)
    {
      if (delta == 0)
      {
        *pidx = mid;
        return(NULL);
      }
      
      high = mid - 1;
      if (high > asize)
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
      return(&array[mid]);
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

/*****************************************************************/

void edomain_add_from(EDomain rec,size_t index)
{
  ddt(rec   != NULL);
  ddt(index <= g_sfrom);
  
  if (g_sfrom == g_smaxfrom)
  {
    g_smaxfrom += ED_DELTA;
    g_from      = MemResize(g_from,g_smaxfrom);
  }
  
  memmove(
  	&g_from[index + 1],
  	&g_from[index],
  	(g_sfrom - index) * sizeof(struct emaildomain)
  );
  
  g_from[index] = *rec;
  g_sfrom++;
}

/******************************************************************/

void edomain_add_fromd(EDomain rec,size_t index)
{
  ddt(rec   != NULL);
  ddt(index <= g_sfromd);
    
  if (g_sfromd == g_smaxfromd)
  {
    g_smaxfromd += ED_DELTA;
    g_fromd      = MemResize(g_fromd,g_smaxfromd);
  }
  
  memmove(
  	&g_fromd[index + 1],
  	&g_fromd[index],
  	(g_sfromd - index) * sizeof(struct emaildomain)
  );
  
  g_fromd[index] = *rec;
  g_sfromd++;
}

/******************************************************************/

void edomain_add_to(EDomain rec,size_t index)
{
  ddt(rec   != NULL);
  ddt(index <= g_sto);
  
  if (g_sto == g_smaxto)
  {
    g_smaxto += ED_DELTA;
    g_to      = MemResize(g_to,g_smaxto);
  }
  
  memmove(
  	&g_to[index + 1],
  	&g_to[index],
  	(g_sto - index) * sizeof(struct emaildomain)
  );
  
  g_to[index] = *rec;
  g_sto++;
}

/********************************************************************/

void edomain_add_tod(EDomain rec,size_t index)
{
  ddt(rec   != NULL);
  ddt(index <= g_stod);
  
  if (g_stod == g_smaxtod)
  {
    g_smaxtod += ED_DELTA;
    g_tod      = MemResize(g_tod,g_smaxtod);
  }
  
  memmove(
  	&g_tod[index + 1],
  	&g_tod[index],
  	(g_stod - index) * sizeof(struct emaildomain)
  );
  
  g_tod[index] = *rec;
  g_stod++;
}

/******************************************************************/

int to_dump(void)
{
  Stream out;
  
  out = FileStreamWrite(c_tofile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_tofile);
    return(ERR_ERR);
  }
  
  to_dump_stream(out);
  StreamFree(out);
  return(ERR_OKAY);
}

/******************************************************************/

int tod_dump(void)
{
  Stream out;
  
  out = FileStreamWrite(c_todfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_todfile);
    return (ERR_ERR);
  }
  
  tod_dump_stream(out);
  StreamFree(out);
  return(ERR_OKAY);
}

/******************************************************************/

int from_dump(void)
{
  Stream out;
  
  out = FileStreamWrite(c_fromfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_fromfile);
    return(ERR_ERR);
  }
  
  from_dump_stream(out);
  StreamFree(out);
  return(ERR_OKAY);
}

/******************************************************************/

int fromd_dump(void)
{
  Stream out;
  
  out = FileStreamWrite(c_fromdfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_fromdfile);
    return(ERR_ERR);
  }
  
  fromd_dump_stream(out);
  StreamFree(out);
  return(ERR_OKAY);
}

/****************************************************************/

int to_dump_stream(Stream out)
{
  ddt(out != NULL);
  
  tofrom_dump_stream(out,g_to,g_sto);
  return(ERR_OKAY);
}

/*********************************************************************/

int tod_dump_stream(Stream out)
{
  ddt(out);
  
  tofrom_dump_stream(out,g_tod,g_stod);
  return(ERR_OKAY);
}

/********************************************************************/

int from_dump_stream(Stream out)
{
  ddt(out);
  
  tofrom_dump_stream(out,g_from,g_sfrom);
  return(ERR_OKAY);
}

/********************************************************************/

int fromd_dump_stream(Stream out)
{
  ddt(out);
  
  tofrom_dump_stream(out,g_fromd,g_sfromd);
  return(ERR_OKAY);
}

/*******************************************************************/

static void tofrom_dump_stream(Stream out,EDomain list,size_t size)
{
  size_t      i;
  const char *cmd;

  ddt(out  != NULL);

  for (i = 0 ; i < size ; i++)
  {
    cmd = ci_map_chars(list[i].cmd,c_ipcmds,C_IPCMDS);
    LineSFormat(out,"L10 $8.8l $","%a %b %c\n",list[i].count,cmd,list[i].text);
  }
}

/*********************************************************************/
