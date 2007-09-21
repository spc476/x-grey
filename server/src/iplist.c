
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <syslog.h>

#include <cgilib/stream.h>
#include <cgilib/memory.h>
#include <cgilib/util.h>
#include <cgilib/errors.h>
#include <cgilib/ddt.h>

#include "../../common/src/globals.h"
#include "../../common/src/util.h"
#include "globals.h"
#include "iplist.h"
#include "iptable.h"

/*****************************************************************/

static const char    m_dunno[]    = "GRAYLIST";
static const char    m_accept[]   = "ACCEPT";
static const char    m_reject[]   = "REJECT";

/*******************************************************************/

int iplist_read(const char *fname)
{
  byte           ip   [16];
  Stream         in;
  char          *line;
  char          *tline;
  int            lcount = 0;
  size_t         octetcount;
  int            mask;
  int            cmd;
  
  ddt(fname != NULL);
  
  in = FileStreamRead(fname);
  if (in == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",fname);
    return(ERR_ERR);
  }
  
  while(!StreamEOF(in))
  {
    line  = LineSRead(in);
    tline = trim_space(line);
    
    lcount++;

    if (empty_string(tline)) 
    {
      MemFree(line);
      continue;    
    }
    
    if (tline[0] == '#') 
    {
      MemFree(line);
      continue;	/* comments */
    }
    
    up_string(tline);
    
    if (!isdigit(tline[0]))
    {
      (*cv_report)(LOG_ERR,"$ i","syntax error on %a(%b)-needs to be an IP address",fname,lcount);
      MemFree(line);
      StreamFree(in);
      return(ERR_ERR);
    }
    
    octetcount  = 0;  
    
    do
    {
      ip[octetcount] = strtoul(tline,&tline,10);
      if (tline[0] == '/') break;
      if (isspace(tline[0])) break;
      if (tline[0] != '.')
      {
        (*cv_report)(LOG_ERR,"$ i","syntax error on %a(%b)-needs to be an IP address",fname,lcount);
        MemFree(line);
        StreamFree(in);
        return(ERR_ERR);
      }
      
      octetcount ++;
      tline++;
    } while(octetcount < 4);
    
    if (*tline++ == '/')
      mask = strtoul(tline,&tline,10);
    else 
      mask = 32;
    
    if (mask > 32)
    {
      (*cv_report)(LOG_ERR,"$ i i","syntax error on %a(%b)-bad mask value %c",fname,lcount,mask);
      MemFree(line);
      StreamFree(in);
      return(ERR_ERR);
    }
    
    for ( ; *tline && isspace(*tline) ; tline++)
      ;

    if (strncmp(tline,m_accept,6) == 0)
      cmd = IPCMD_ACCEPT;
    else if (strncmp(tline,m_reject,6) == 0)
      cmd = IPCMD_REJECT;
    else if (strncmp(tline,m_dunno,5) == 0)
      cmd = IPCMD_GRAYLIST;
    else
    {
      (*cv_report)(LOG_ERR,"$ i $","syntax error on %a(%b)-needs to be ACCEPT or REJECT but not %c",fname,lcount,tline);
      MemFree(line);
      StreamFree(in);
      return(ERR_ERR);
    }

    ip_add_sm(ip,4,mask,cmd);
    MemFree(line);
  }
  
  StreamFree(in);

  if (cf_debug)
  {
    struct ipblock *iplist;
    size_t          ipsize;
    size_t          i;
    
    iplist = ip_table(&ipsize);

    for (i = 0 ; i < ipsize ; i++)
    {
      char tip  [20];
      char tmask[20];
      char *t;
      char *cmd;
      
      t = ipv4(iplist[i].addr);
      strcpy(tip,t);
      t = ipv4(iplist[i].mask);
      strcpy(tmask,t);
      
      switch(iplist[i].cmd)
      {
        case IPCMD_NONE:     cmd = "NONE (this is a bug)"; break;
        case IPCMD_ACCEPT:   cmd = "ACCEPT";   break;
        case IPCMD_REJECT:   cmd = "REJECT";   break;
        case IPCMD_GRAYLIST: cmd = "GRAYLIST"; break;
        default: ddt(0);
      }
      
      (*cv_report)(
      		LOG_DEBUG,
      		"$8.8l $15.15l $15.15l",
      		"%a %b %c",
      		cmd,
		tip,
		tmask
    	);
    }

    MemFree(iplist);
  }
  
  return(ERR_OKAY);
}

/**************************************************************/

int iplist_cmp(const void *left,const void *right)
{
  const struct ipblock *l = left;
  const struct ipblock *r = right;
  int                   rc;
  
  ddt(left    != NULL);
  ddt(right   != NULL);
  ddt(l->size == 4);
  ddt(r->size == 4);

  if ((rc = memcmp(l->mask,r->mask,l->size)) != 0) return(-rc);
  if ((rc = memcmp(l->addr,r->addr,l->size)) != 0) return(rc);

  return(0);
}

/******************************************************************/

int iplist_check(byte *addr,size_t size)
{
  return ip_match(addr,size);
}

/*****************************************************************/

int iplist_dump(void)
{
  Stream out;

  out = FileStreamWrite(c_iplistfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_iplistfile);
    return(ERR_ERR);
  }
  
  iplist_dump_stream(out);
  
  StreamFree(out);
  return(ERR_OKAY);
}

/*****************************************************************/

int iplist_dump_stream(Stream out)
{
  struct ipblock *array;
  size_t          asize;
  size_t          i;
  char            ipaddr[20];
  
  array = ip_table(&asize);
  
  for (i = 0 ; i < asize ; i++)
  {
    sprintf(ipaddr,"%s/%d",ipv4(array[i].addr),array[i].smask);

    LineSFormat(
    	out,
    	"$18.18l $",
    	"%a %b\n",
	ipaddr,
    	ci_map_chars(array[i].cmd,c_ipcmds,C_IPCMDS)
    );
  }

  MemFree(array);  
  return(ERR_OKAY);
}

/*************************************************************/

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
    if ((g_tuplespace[i]->f & F_WHITELIST))
    {
      LineSFormat(
      		out,
      		"$ $ $",
      		"%a %b %c\n",
      		ipv4(g_tuplespace[i]->ip),
      		g_tuplespace[i]->from,
      		g_tuplespace[i]->to
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
    LineSFormat(
        out,
        "$ $ $ $ $ $ $ $ L L",
        "%a %b %c %d%e%f%g%h %i %j\n",
        ipv4(g_tuplespace[i]->ip),
        g_tuplespace[i]->from,
        g_tuplespace[i]->to,
        (g_tuplespace[i]->f & F_WHITELIST) ? "W" : "-",
        (g_tuplespace[i]->f & F_GRAYLIST)  ? "G" : "-",
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
    if (empty_string(line)) continue;
    
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
    
    strncpy(tuple.from,p,sizeof(tuple.from) - 1);
    strncpy(tuple.to,  n,sizeof(tuple.to)   - 1);
    tuple.fromsize = strlen(tuple.from);
    tuple.tosize   = strlen(tuple.to);
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

