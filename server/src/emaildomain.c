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

#include <syslog.h>

#include <cgilib/memory.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>
#include <cgilib/ddt.h>

#include "../../common/src/globals.h"
#include "../../common/src/util.h"

#include "emaildomain.h"
#include "globals.h"

#define ED_DELTA	100

static void tofrom_dump_stream	(Stream,EDomain,size_t);
static int  tofrom_read		(
				  const char *,
				  EDomain (*)(EDomain,size_t *),
				  void    (*)(EDomain,size_t),
				  int      *,
				  size_t   *
				);
static void tofrom_read_stream	(
				  Stream,
				  EDomain (*)(EDomain,size_t *),
				  void    (*)(EDomain,size_t),
				  int      *,
				  size_t   *
				);

/******************************************************************/

int edomain_cmp(EDomain key,EDomain node)
{
  ddt(key  != NULL);
  ddt(node != NULL);
  
  return(strcmp(key->text,node->text));
}

/*********************************************************************/

EDomain edomain_search_to(EDomain key,size_t *pidx)
{
  ddt(key  != NULL);
  ddt(pidx != NULL);
  
  return (edomain_search(key,pidx,g_to,g_sto));
}

/***********************************************************************/

EDomain edomain_search_tod(EDomain key,size_t *pidx)
{
  ddt(key  != NULL);
  ddt(pidx != NULL);
  
  return (edomain_search(key,pidx,g_tod,g_stod));
}

/********************************************************************/

EDomain edomain_search_from(EDomain key,size_t *pidx)
{
  ddt(key  != NULL);
  ddt(pidx != NULL);
  
  return(edomain_search(key,pidx,g_from,g_sfrom));
}

/******************************************************************/

EDomain edomain_search_fromd(EDomain key,size_t *pidx)
{
  ddt(key  != NULL);
  ddt(pidx != NULL);
  
  return(edomain_search(key,pidx,g_fromd,g_sfromd));
}

/*******************************************************************/

EDomain edomain_search(EDomain key,size_t *pidx,EDomain array,size_t len)
{
  size_t first;
  size_t middle;
  size_t half;
  int    q;
  
  ddt(key  != NULL);
  ddt(pidx != NULL);
  
  first = 0;
  
  while(len > 0)
  {
    half   = len / 2;
    middle = first + half;
    q      = edomain_cmp(key,&array[middle]);
    
    if (q > 0)
    {
      first = middle + 1;
      len   = len - half - 1;
    }
    else if (q == 0)
    {
      *pidx = middle;
      return &array[middle];
    }
    else
      len = half;
  }
  
  *pidx = first;
  return NULL;
}

/*****************************************************************/

void edomain_add_from(EDomain rec,size_t index)
{
  ddt(rec   != NULL);
  ddt(index <= g_sfrom);
  
  if (g_sfrom == g_smaxfrom)
  {
    g_smaxfrom += ED_DELTA;
    g_from      = MemResize(g_from,g_smaxfrom * sizeof(struct emaildomain));
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

void edomain_remove_from(size_t index)
{
  size_t count;
  
  ddt(index <= g_sfrom);
  
  if (g_sfrom == 0) return;
  MemFree(g_from[index].text);
  count = (g_sfrom - index) - 1;
  if (count)
  {
    memmove(
    	&g_from[index],
    	&g_from[index + 1],
    	count * sizeof(struct emaildomain)
    );
  }
  
  g_sfrom--;
}

/*******************************************************************/

void edomain_add_fromd(EDomain rec,size_t index)
{
  ddt(rec   != NULL);
  ddt(index <= g_sfromd);
    
  if (g_sfromd == g_smaxfromd)
  {
    g_smaxfromd += ED_DELTA;
    g_fromd      = MemResize(g_fromd,g_smaxfromd * sizeof(struct emaildomain));
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

void edomain_remove_fromd(size_t index)
{
  size_t count;
  
  ddt(index <= g_sfromd);
  
  if (g_sfromd == 0) return;
  MemFree(g_fromd[index].text);
  count = (g_sfromd - index) - 1;
  if (count)
  {
    memmove(
    	&g_fromd[index],
    	&g_fromd[index + 1],
    	count * sizeof(struct emaildomain)
    );
  }
  g_sfromd--;
}

/*****************************************************************/

void edomain_add_to(EDomain rec,size_t index)
{
  ddt(rec   != NULL);
  ddt(index <= g_sto);
  
  if (g_sto == g_smaxto)
  {
    g_smaxto += ED_DELTA;
    g_to      = MemResize(g_to,g_smaxto * sizeof(struct emaildomain));
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

void edomain_remove_to(size_t index)
{
  size_t count;
  
  ddt(index <= g_sto);

  if (g_sto == 0) return;  
  MemFree(g_to[index].text);
  count = (g_sto - index) - 1;

  if (count)
  {
    memmove(
    	&g_to[index],
    	&g_to[index + 1],
    	count * sizeof(struct emaildomain)
    );
  }
  g_sto--;
}

/*********************************************************************/

void edomain_add_tod(EDomain rec,size_t index)
{
  ddt(rec   != NULL);
  ddt(index <= g_stod);
  
  if (g_stod == g_smaxtod)
  {
    g_smaxtod += ED_DELTA;
    g_tod      = MemResize(g_tod,g_smaxtod * sizeof(struct emaildomain));
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

void edomain_remove_tod(size_t index)
{
  size_t count;
  
  ddt(index <= g_stod);
  
  if (g_stod == 0) return;
  MemFree(g_tod[index].text);
  count = (g_stod - index) - 1;
  if (count)
  {
    memmove(
    	&g_tod[index],
    	&g_tod[index + 1],
    	count * sizeof(struct emaildomain)
    );
  }
  g_stod--;
}

/*******************************************************************/

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
  const char *cmd;
  
  ddt(out != NULL);
  
  tofrom_dump_stream(out,g_to,g_sto);
  cmd = ci_map_chars(g_defto,c_ift,C_IFT);
  LineSFormat(out,"L10 $8.8l","%a %b DEFAULT\n",g_toc,cmd);
  return(ERR_OKAY);
}

/*********************************************************************/

int tod_dump_stream(Stream out)
{
  const char *cmd;
  
  ddt(out);
  
  tofrom_dump_stream(out,g_tod,g_stod);
  cmd = ci_map_chars(g_deftodomain,c_ift,C_IFT);
  LineSFormat(out,"L10 $8.8l","%a %b DEFAULT\n",g_todomainc,cmd);
  return(ERR_OKAY);
}

/********************************************************************/

int from_dump_stream(Stream out)
{
  const char *cmd;
  
  ddt(out);
  
  tofrom_dump_stream(out,g_from,g_sfrom);
  cmd = ci_map_chars(g_deffrom,c_ift,C_IFT);
  LineSFormat(out,"L10 $8.8l","%a %b DEFAULT\n",g_fromc,cmd);
  return(ERR_OKAY);
}

/********************************************************************/

int fromd_dump_stream(Stream out)
{
  const char *cmd;
  
  ddt(out);
  
  tofrom_dump_stream(out,g_fromd,g_sfromd);
  cmd = ci_map_chars(g_deffromdomain,c_ift,C_IFT);
  LineSFormat(out,"L10 $8.8l","%a %b DEFAULT\n",g_fromdomainc,cmd);
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
    cmd = ci_map_chars(list[i].cmd,c_ift,C_IFT);
    LineSFormat(out,"L10 $8.8l $","%a %b %c\n",list[i].count,cmd,list[i].text);
  }
}

/*********************************************************************/

int to_read(void)
{
  return (tofrom_read(c_tofile,edomain_search_to,edomain_add_to,&g_defto,&g_toc));
}

/*******************************************************************/

int tod_read(void)
{
  return (tofrom_read(c_todfile,edomain_search_tod,edomain_add_tod,&g_deftodomain,&g_todomainc));
}

/******************************************************************/

int from_read(void)
{
  return (tofrom_read(c_fromfile,edomain_search_from,edomain_add_from,&g_deffrom,&g_fromc));
}

/*****************************************************************/

int fromd_read(void)
{
  return (tofrom_read(c_fromdfile,edomain_search_fromd,edomain_add_fromd,&g_deffromdomain,&g_fromdomainc));
}

/******************************************************************/

static int tofrom_read(
		const char  *fname,
		EDomain    (*search)(EDomain,size_t *),
		void       (*add)   (EDomain,size_t),
		int         *pdef,
		size_t      *pcount
	)
{
  Stream in;
  
  ddt(fname  != NULL);
  ddt(search);
  ddt(add);
  ddt(pdef   != NULL);
  ddt(pcount != NULL);
  
  in = FileStreamRead(fname);
  if (in == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",fname);
    return(ERR_ERR);
  }
  
  tofrom_read_stream(in,search,add,pdef,pcount);
  StreamFree(in);
  return(ERR_OKAY);
}

/*******************************************************************/

static void tofrom_read_stream(
		Stream    in,
		EDomain (*search)(EDomain,size_t *),
		void    (*add)   (EDomain,size_t),
		int      *pdef,
		size_t   *pcount
	)
{
  struct emaildomain  ed;
  EDomain             value;
  String             *fields;
  size_t              fsize;
  char               *line;
  size_t              idx;
  
  ddt(in     != NULL);
  ddt(search);
  ddt(add);
  ddt(pdef   != NULL);
  ddt(pcount != NULL);
  
  line    = dup_string("");
  fields  = MemAlloc(sizeof(String));
  
  while(!StreamEOF(in))
  {
    MemFree(fields);
    MemFree(line);
    
    line = LineSRead(in);
    if (empty_string(line))
      continue;
    
    line = trim_space(line);
    
    if (line[0] == '#')
      continue;
      
    fields = split(&fsize,line);
    if (fsize < 3)
    {
      (*cv_report)(LOG_ERR,"","bad input in one of the email files");
      continue;
    }
    
    if (
         (strcmp(fields[2].d,"default") == 0)
         || (strcmp(fields[2].d,"DEFAULT") == 0)
       )
    {
      *pdef   = ci_map_int(fields[1].d,c_ift,C_IFT);
      if (cf_oldcounts)
        *pcount = strtoul(fields[0].d,NULL,10);
      else
        *pcount = 0;
      continue;
    }

    ed.text  = (char *)fields[2].d;
    ed.tsize = fields[2].s;
    if (cf_oldcounts)
      ed.count = strtoul(fields[0].d,NULL,10);
    else
      ed.count = 0;
    ed.cmd   = ci_map_int(fields[1].d,c_ift,C_IFT);
    
    (*cv_report)(LOG_DEBUG,"$ i","adding %a as %b",ed.text,ed.cmd);

    value = (*search)(&ed,&idx);

    if (value == NULL)
    {
      ed.text = dup_string(ed.text);
      (*add)(&ed,idx);
    }
  }
}

/******************************************************************/

