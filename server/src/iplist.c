
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <syslog.h>

#include <cgilib/stream.h>
#include <cgilib/memory.h>
#include <cgilib/util.h>
#include <cgilib/errors.h>
#include <cgilib/ddt.h>

#include "../../common/src/util.h"
#include "globals.h"
#include "iplist.h"

/*****************************************************************/

typedef byte maskval[4];

/********************************************************************/

static int	iplist_cmp	(const void *,const void *);

/********************************************************************/

static const char    m_accept[]   = "ACCEPT";
static const char    m_reject[]   = "REJECT";
static const maskval m_masks [33] = 
{
  { 0x00 , 0x00 , 0x00 , 0x00 } ,
  { 0x80 , 0x00 , 0x00 , 0x00 } ,
  { 0xC0 , 0x00 , 0x00 , 0x00 } ,
  { 0xE0 , 0x00 , 0x00 , 0x00 } ,
  { 0xF0 , 0x00 , 0x00 , 0x00 } ,
  { 0xF8 , 0x00 , 0x00 , 0x00 } ,
  { 0xFC , 0x00 , 0x00 , 0x00 } ,
  { 0xFE , 0x00 , 0x00 , 0x00 } ,
  { 0xFF , 0x00 , 0x00 , 0x00 } ,
  { 0xFF , 0x80 , 0x00 , 0x00 } ,
  { 0xFF , 0xC0 , 0x00 , 0x00 } ,
  { 0xFF , 0xE0 , 0x00 , 0x00 } ,
  { 0xFF , 0xF0 , 0x00 , 0x00 } ,
  { 0xFF , 0xF8 , 0x00 , 0x00 } ,
  { 0xFF , 0xFC , 0x00 , 0x00 } ,
  { 0xFF , 0xFE , 0x00 , 0x00 } ,
  { 0xFF , 0xFF , 0x00 , 0x00 } ,
  { 0xFF , 0xFF , 0x80 , 0x00 } ,
  { 0xFF , 0xFF , 0xC0 , 0x00 } ,
  { 0xFF , 0xFF , 0xE0 , 0x00 } ,
  { 0xFF , 0xFF , 0xF0 , 0x00 } ,
  { 0xFF , 0xFF , 0xF8 , 0x00 } ,
  { 0xFF , 0xFF , 0xFC , 0x00 } ,
  { 0xFF , 0xFF , 0xFE , 0x00 } ,
  { 0xFF , 0xFF , 0xFF , 0x00 } ,
  { 0xFF , 0xFF , 0xFF , 0x80 } ,
  { 0xFF , 0xFF , 0xFF , 0xC0 } ,
  { 0xFF , 0xFF , 0xFF , 0xE0 } ,
  { 0xFF , 0xFF , 0xFF , 0xF0 } ,
  { 0xFF , 0xFF , 0xFF , 0xF8 } ,
  { 0xFF , 0xFF , 0xFF , 0xFC } ,
  { 0xFF , 0xFF , 0xFF , 0xFE } ,
  { 0xFF , 0xFF , 0xFF , 0xFF }
};

/*******************************************************************/

int iplist_read(const char *fname)
{
  Stream         in;
  char          *line;
  char          *tline;
  int            lcount = 0;
  size_t         octetcount;
  int            mask;
  size_t         i;
  
  ddt(fname != NULL);
  
  in = FileStreamRead(fname);
  if (in == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",fname);
    return(ERR_ERR);
  }
  
  while(!StreamEOF(in))
  {
    if (g_ipcnt == c_ipmax)
    {
      (*cv_report)(LOG_WARNING,"L","IP list is limited to %a entries",(unsigned long)c_ipmax);
      break;
    }
    
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
      g_iplist[g_ipcnt].addr[octetcount] = strtoul(tline,&tline,10);
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
      g_iplist[g_ipcnt].cmd = IPCMD_ACCEPT;
    else if (strncmp(tline,m_reject,6) == 0)
      g_iplist[g_ipcnt].cmd = IPCMD_REJECT;
    else
    {
      (*cv_report)(LOG_ERR,"$ i $","syntax error on %a(%b)-needs to be ACCEPT or REJECT but not %c",fname,lcount,tline);
      MemFree(line);
      StreamFree(in);
      return(ERR_ERR);
    }

    g_iplist[g_ipcnt].size = 4;
    memcpy(g_iplist[g_ipcnt].mask,m_masks[mask],4);
      
    g_ipcnt++;
    MemFree(line);
  }
  
  StreamFree(in);
  qsort(g_iplist,g_ipcnt,sizeof(struct ipblock),iplist_cmp);

  for (i = 0 ; i < g_ipcnt ; i++)
  {
    char tip  [20];
    char tmask[20];
    char *t;
      
    t = ipv4(g_iplist[i].addr);
    strcpy(tip,t);
    t = ipv4(g_iplist[i].mask);
    strcpy(tmask,t);
      
    (*cv_report)(
      		LOG_DEBUG,
      		"$ $ $",
      		"%a %b %c",
      		(g_iplist[i].cmd == IPCMD_ACCEPT)
      			? m_accept
      			: m_reject,
		tip,
		tmask
    	);
  }

  return(ERR_OKAY);
}

/**************************************************************/

static int iplist_cmp(const void *left,const void *right)
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
  size_t i;
  IP     a;
  IP     mask;
  IP     net;
  
  ddt(addr != NULL);
  ddt(size == 4);
  
  a = *(IP *)addr;
  
  for (i = 0 ; i < g_ipcnt ; i++)
  {
    mask = *(IP *)&g_iplist[i].mask;
    net  = *(IP *)&g_iplist[i].addr;

    if ((a & mask) == net)
      return(g_iplist[i].cmd);
  }
  
  return (IPCMD_DUNNO);
}

/*****************************************************************/

int whitelist_dump(void)
{
  Stream out;
  size_t i;
  
  out = FileStreamWrite(c_whitefile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_whitefile);
    return(ERR_ERR);
  }
  
  for (i = 0 ; i < g_poolnum ; i++)
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
  
  StreamFree(out);
  return(ERR_OKAY);
}

/********************************************************************/

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

