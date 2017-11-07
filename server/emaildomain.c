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
#include <assert.h>
#include <errno.h>

#include <syslog.h>

#include <cgilib6/util.h>

#include "../common/globals.h"
#include "../common/util.h"
#include "../common/bisearch.h"
#include "emaildomain.h"
#include "globals.h"

#define ED_DELTA        100

static void file_dump           (const char *,void (*)(FILE *));
static void tofrom_dump_stream  (FILE *,EDomain,size_t);
static void tofrom_read         (
                                  const char  *,
                                  EDomain    (*)(EDomain,size_t *),
                                  void       (*)(EDomain,size_t),
                                  int         *,
                                  size_t      *
                                );
static void tofrom_read_stream  (
                                  const char  *,
                                  FILE        *,
                                  EDomain    (*)(EDomain,size_t *),
                                  void       (*)(EDomain,size_t),
                                  int         *,
                                  size_t      *
                                );
                                
/******************************************************************/

int edomain_cmp(EDomain key,EDomain node)
{
  assert(key  != NULL);
  assert(node != NULL);
  
  return strcmp(key->text,node->text);
}

/*********************************************************************/

static int edomain_look(const void *restrict key,const void *restrict values)
{
  const struct emaildomain *k = key;
  const struct emaildomain *v = values;
  
  return strcmp(k->text,v->text);
}

/*********************************************************************/

EDomain edomain_search_to(EDomain key,size_t *pidx)
{
  assert(key  != NULL);
  assert(pidx != NULL);
  
  return edomain_search(key,pidx,g_to,g_sto);
}

/***********************************************************************/

EDomain edomain_search_tod(EDomain key,size_t *pidx)
{
  assert(key  != NULL);
  assert(pidx != NULL);
  
  return edomain_search(key,pidx,g_tod,g_stod);
}

/********************************************************************/

EDomain edomain_search_from(EDomain key,size_t *pidx)
{
  assert(key  != NULL);
  assert(pidx != NULL);
  
  return edomain_search(key,pidx,g_from,g_sfrom);
}

/******************************************************************/

EDomain edomain_search_fromd(EDomain key,size_t *pidx)
{
  assert(key  != NULL);
  assert(pidx != NULL);
  
  return edomain_search(key,pidx,g_fromd,g_sfromd);
}

/*******************************************************************/

EDomain edomain_search(EDomain key,size_t *pidx,EDomain array,size_t len)
{
  bisearch__t item;
  
  item  = bisearch(key,array,len,sizeof(struct emaildomain),edomain_look);
  *pidx = item.idx;
  return (item.datum == NULL) ? NULL : item.datum;
}

/*****************************************************************/

void edomain_add_from(EDomain rec,size_t index)
{
  assert(rec   != NULL);
  assert(index <= g_sfrom);
  
  if (g_sfrom == g_smaxfrom)
  {
    g_smaxfrom += ED_DELTA;
    g_from      = realloc(g_from,g_smaxfrom * sizeof(struct emaildomain));
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
  
  assert(index <= g_sfrom);
  
  if (g_sfrom == 0) return;
  free(g_from[index].text);
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
  assert(rec   != NULL);
  assert(index <= g_sfromd);
  
  if (g_sfromd == g_smaxfromd)
  {
    g_smaxfromd += ED_DELTA;
    g_fromd      = realloc(g_fromd,g_smaxfromd * sizeof(struct emaildomain));
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
  
  assert(index <= g_sfromd);
  
  if (g_sfromd == 0) return;
  free(g_fromd[index].text);
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
  assert(rec   != NULL);
  assert(index <= g_sto);
  
  if (g_sto == g_smaxto)
  {
    g_smaxto += ED_DELTA;
    g_to      = realloc(g_to,g_smaxto * sizeof(struct emaildomain));
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
  
  assert(index <= g_sto);
  
  if (g_sto == 0) return;
  free(g_to[index].text);
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
  assert(rec   != NULL);
  assert(index <= g_stod);
  
  if (g_stod == g_smaxtod)
  {
    g_smaxtod += ED_DELTA;
    g_tod      = realloc(g_tod,g_smaxtod * sizeof(struct emaildomain));
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
  
  assert(index <= g_stod);
  
  if (g_stod == 0) return;
  free(g_tod[index].text);
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

static void file_dump(const char *fname,void (*function)(FILE *))
{
  FILE *fpout;
  
  assert(fname    != NULL);
  assert(function != NULL);
  
  fpout = fopen(fname,"w");
  if (fpout)
  {
    (*function)(fpout);
    fclose(fpout);
  }
  else
    syslog(LOG_ERR,"fopen(%s,WRITE) = %s",(char *)fname,strerror(errno));
}

/**********************************************************************/

void to_dump(void)
{
  file_dump(c_tofile,to_dump_stream);
}

/******************************************************************/

void tod_dump(void)
{
  file_dump(c_todfile,tod_dump_stream);
}

/******************************************************************/

void from_dump(void)
{
  file_dump(c_fromfile,from_dump_stream);
}

/******************************************************************/

void fromd_dump(void)
{
  file_dump(c_fromdfile,fromd_dump_stream);
}

/****************************************************************/

void to_dump_stream(FILE *out)
{
  const char *cmd;
  
  assert(out != NULL);
  
  tofrom_dump_stream(out,g_to,g_sto);
  cmd = ci_map_chars(g_defto,c_ift,C_IFT);
  fprintf(out,"%10lu %8.8s DEFAULT\n",(unsigned long)g_toc,cmd);
}

/*********************************************************************/

void tod_dump_stream(FILE *out)
{
  const char *cmd;
  
  assert(out);
  
  tofrom_dump_stream(out,g_tod,g_stod);
  cmd = ci_map_chars(g_deftodomain,c_ift,C_IFT);
  fprintf(out,"%10lu %8.8s DEFAULT\n",(unsigned long)g_todomainc,cmd);
}

/********************************************************************/

void from_dump_stream(FILE *out)
{
  const char *cmd;
  
  assert(out);
  
  tofrom_dump_stream(out,g_from,g_sfrom);
  cmd = ci_map_chars(g_deffrom,c_ift,C_IFT);
  fprintf(out,"%10lu %8.8s DEFAULT\n",(unsigned long)g_fromc,cmd);
}

/********************************************************************/

void fromd_dump_stream(FILE *out)
{
  const char *cmd;
  
  assert(out);
  
  tofrom_dump_stream(out,g_fromd,g_sfromd);
  cmd = ci_map_chars(g_deffromdomain,c_ift,C_IFT);
  fprintf(out,"%10lu %8.8s DEFAULT\n",(unsigned long)g_fromdomainc,cmd);
}

/*******************************************************************/

static void tofrom_dump_stream(FILE *out,EDomain list,size_t size)
{
  size_t      i;
  const char *cmd;
  
  assert(out  != NULL);
  
  for (i = 0 ; i < size ; i++)
  {
    cmd = ci_map_chars(list[i].cmd,c_ift,C_IFT);
    fprintf(out,"%10lu %8.8s %s\n",(unsigned long)list[i].count,cmd,list[i].text);
  }
}

/*********************************************************************/

void to_read(void)
{
  tofrom_read(c_tofile,edomain_search_to,edomain_add_to,&g_defto,&g_toc);
}

/*******************************************************************/

void tod_read(void)
{
  tofrom_read(c_todfile,edomain_search_tod,edomain_add_tod,&g_deftodomain,&g_todomainc);
}

/******************************************************************/

void from_read(void)
{
  tofrom_read(c_fromfile,edomain_search_from,edomain_add_from,&g_deffrom,&g_fromc);
}

/*****************************************************************/

void fromd_read(void)
{
  tofrom_read(c_fromdfile,edomain_search_fromd,edomain_add_fromd,&g_deffromdomain,&g_fromdomainc);
}

/******************************************************************/

static void tofrom_read(
                const char  *fname,
                EDomain    (*search)(EDomain,size_t *),
                void       (*add)   (EDomain,size_t),
                int         *pdef,
                size_t      *pcount
        )
{
  FILE *in;
  
  assert(fname  != NULL);
  assert(search);
  assert(add);
  assert(pdef   != NULL);
  assert(pcount != NULL);
  
  in = fopen(fname,"r");
  if (in)
  {
    tofrom_read_stream(fname,in,search,add,pdef,pcount);
    fclose(in);
  }
  else
    syslog(LOG_ERR,"fopen(%s,READ) = %s",(char *)fname,strerror(errno));
}

/*******************************************************************/

static void tofrom_read_stream(
                const char  *fname,
                FILE        *in,
                EDomain    (*search)(EDomain,size_t *),
                void       (*add)   (EDomain,size_t),
                int         *pdef,
                size_t      *pcount
        )
{
  struct emaildomain  ed;
  EDomain             value;
  String             *fields;
  size_t              fsize;
  char               *linebuff;
  char               *line;
  size_t              linesize;
  size_t              idx;
  unsigned long       linecnt;
  
  assert(fname  != NULL);
  assert(in     != NULL);
  assert(search);
  assert(add);
  assert(pdef   != NULL);
  assert(pcount != NULL);
  
  linebuff = NULL;
  linesize = 0;
  linecnt  = 0;
  fields   = malloc(sizeof(String));
  
  while(!feof(in))
  {
    free(fields);
    if (getline(&linebuff,&linesize,in) <= 0)
      break;
      
    linecnt++;
    
    if (empty_string(linebuff))
      continue;
      
    line = trim_space(linebuff);
    
    if (line[0] == '#')
      continue;
      
    fields = split(&fsize,line);
    if (fsize < 3)
    {
      syslog(LOG_ERR,"%s(%lu): bad input",(char *)fname,linecnt);
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
    
    syslog(LOG_DEBUG,"adding %s as %d",ed.text,ed.cmd);
    
    value = (*search)(&ed,&idx);
    
    if (value == NULL)
    {
      ed.text = strdup(ed.text);
      (*add)(&ed,idx);
    }
  }
  
  free(linebuff);
}

/******************************************************************/

