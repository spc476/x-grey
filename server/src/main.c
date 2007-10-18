
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>
#include <cgilib/types.h>

#include "../../common/src/graylist.h"
#include "../../common/src/util.h"
#include "../../common/src/crc32.h"

#include "tuple.h"
#include "globals.h"
#include "signals.h"
#include "util.h"
#include "server.h"
#include "iplist.h"
#include "emaildomain.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

/********************************************************************/

void 		 type_graylist		(struct request *);

static void	 mainloop		(int);
static void	 send_reply		(struct request *,int,int);
static void	 send_packet		(struct request *,void *,size_t);
static void	 cmd_mcp_report		(struct request *,int (*)(Stream),int);
static void	 cmd_mcp_tofrom		(struct request *,int,int);

/********************************************************************/

int main(int argc,char *argv[])
{
  int sock;
  
  MemInit();
  DdtInit();
  StreamInit();

  /*----------------------------------------------------
  ; A bit of a hack to set the close on exec flags for
  ; various streams opened up by the CGILIB.
  ;----------------------------------------------------*/

  GlobalsInit(argc,argv);

  sock = create_socket(c_host,c_port,SOCK_DGRAM);
  
  {
    char *t = timetoa(c_starttime);
    (*cv_report)(LOG_INFO,"$","start time: %a",t);
    MemFree(t);
  }

  mainloop(sock);
  
  return(EXIT_SUCCESS);
}

/********************************************************************/

static void mainloop(int sock)
{
  struct request req;
  ssize_t        rrc;
  CRC32          crc;
  
  while(1)
  {
    check_signals();

    memset(&req.remote,0,sizeof(req.remote));
    req.rsize = sizeof(req.remote);

    rrc = recvfrom(
    	sock,
    	req.packet,
    	sizeof(req.packet),
    	0,
    	&req.remote,
    	&req.rsize
    );
    
    if (rrc == -1)
    {
      if (errno != EINTR)
        (*cv_report)(LOG_ERR,"$","recvfrom() = %a",strerror(errno));
      continue;
    }
    
    req.now  = time(NULL);
    req.sock = sock;
    req.glr  = (struct graylist_request *)req.packet;
    req.size = rrc;
    
    crc = crc32(INIT_CRC32,&req.glr->version,req.size - sizeof(CRC32));
    crc = crc32(crc,c_secret,c_secretsize);
    
    if (crc != ntohl(req.glr->crc))
    {
      (*cv_report)(LOG_DEBUG,"U","bad CRC-skipping packet %a",(unsigned long)crc);
      continue;
    }
    
    if (ntohs(req.glr->version) != VERSION)
    {
      (*cv_report)(LOG_DEBUG,"","received bad version");
      send_reply(&req,CMD_NONE_RESP,GLERR_VERSION_NOT_SUPPORTED);
      continue;
    }
    
    switch(ntohs(req.glr->type))
    {
      case CMD_NONE:
           send_reply(&req,CMD_NONE_RESP,GLERR_OKAY);
           break;
      case CMD_GRAYLIST:
           type_graylist(&req);
           break;
      case CMD_MCP_SHOW_STATS:
           {
             struct glmcp_response_show_stats stats;
             
	     stats.crc = stats.pad   = htons(0);	/* VVV */
             stats.version           = htons(VERSION);
             stats.MTA               = req.glr->MTA;
             stats.type              = htons(CMD_MCP_SHOW_STATS_RESP);
             stats.starttime         = htonl(c_starttime);
             stats.nowtime           = htonl(req.now);
             stats.tuples            = htonl(g_poolnum);
	     stats.ips               = htonl(g_ipcnt);
             stats.graylisted        = htonl(g_graylisted);
             stats.whitelisted       = htonl(g_whitelisted);
             stats.graylist_expired  = htonl(g_graylist_expired);
             stats.whitelist_expired = htonl(g_whitelist_expired);
             stats.requests          = htonl(g_requests);
             stats.requests_cleanup  = htonl(g_req_cu);
             
             send_packet(&req,&stats,sizeof(stats)); /* VVV - uninit mem */
           }
           break;
      case CMD_MCP_SHOW_CONFIG:
           {
             struct glmcp_response_show_config config;
             
             config.version         = htons(VERSION);
             config.MTA             = req.glr->MTA;
             config.type            = htons(CMD_MCP_SHOW_CONFIG_RESP);
             config.max_tuples      = htonl(c_poolmax);
             config.timeout_cleanup = htonl(c_time_cleanup);
             config.timeout_embargo = htonl(c_timeout_embargo);
             config.timeout_gray    = htonl(c_timeout_gray);
             config.timeout_white   = htonl(c_timeout_white);
             
             send_packet(&req,&config,sizeof(config));
           }
           break;
      case CMD_MCP_SHOW_IPLIST:
           cmd_mcp_report(&req,ip_print,CMD_MCP_SHOW_IPLIST_RESP);
           break;
      case CMD_MCP_SHOW_TUPLE_ALL:
           cmd_mcp_report(&req,tuple_dump_stream,CMD_MCP_SHOW_TUPLE_ALL_RESP);
           break;
      case CMD_MCP_SHOW_WHITELIST:
           cmd_mcp_report(&req,whitelist_dump_stream,CMD_MCP_SHOW_WHITELIST_RESP);
           break;
      case CMD_MCP_SHOW_TO:
           cmd_mcp_report(&req,to_dump_stream,CMD_MCP_SHOW_TO_RESP);
           break;
      case CMD_MCP_SHOW_TO_DOMAIN:
           cmd_mcp_report(&req,tod_dump_stream,CMD_MCP_SHOW_TO_DOMAIN_RESP);
           break;
      case CMD_MCP_SHOW_FROM:
           cmd_mcp_report(&req,from_dump_stream,CMD_MCP_SHOW_FROM_RESP);
           break;
      case CMD_MCP_SHOW_FROM_DOMAIN:
           cmd_mcp_report(&req,fromd_dump_stream,CMD_MCP_SHOW_FROM_DOMAIN_RESP);
           break;
      case CMD_MCP_IPLIST:
           {
             struct glmcp_request_iplist *pip = (struct glmcp_request_iplist *)req.glr;
             int   cmd  = ntohs(pip->cmd);
             int   mask = ntohs(pip->mask);
	     char *t;
             
             if (ntohs(pip->ipsize) != 4)
             {
               send_reply(&req,CMD_NONE_RESP,GLERR_IPADDR_NOT_SUPPORTED);
               break;
             }

	     t = ipv4(pip->data);
	     (*cv_report)(LOG_DEBUG,"$ i i","about to add %a/%b %c",t,mask,cmd);
             
             ip_add_sm(pip->data,4,mask,cmd);
             send_reply(&req,CMD_MCP_IPLIST_RESP,0);
           }
           break;
      case CMD_MCP_TO:
           cmd_mcp_tofrom(&req,CMD_MCP_TO,CMD_MCP_TO_RESP);
           break;
      case CMD_MCP_TO_DOMAIN:
           cmd_mcp_tofrom(&req,CMD_MCP_TO_DOMAIN,CMD_MCP_TO_DOMAIN_RESP);
           break;
      case CMD_MCP_FROM:
           cmd_mcp_tofrom(&req,CMD_MCP_FROM,CMD_MCP_FROM_RESP);
           break;
      case CMD_MCP_FROM_DOMAIN:
           cmd_mcp_tofrom(&req,CMD_MCP_FROM_DOMAIN,CMD_MCP_FROM_DOMAIN_RESP);
           break;
      default:
           send_reply(&req,CMD_NONE_RESP,GLERR_TYPE_NOT_SUPPORTED);
           break;
    }
  }
}

/*******************************************************************/

void type_graylist(struct request *req)
{
  struct graylist_request *glr;
  struct tuple             tuple;
  struct emaildomain       edkey;
  EDomain                  edvalue;
  Tuple                    stored;
  size_t                   rsize;
  size_t                   idx;
  byte                    *p;
  char                    *at;
  int                      rc;

  ddt(req != NULL);

  /*---------------------------------------------------
  ; sanitize the request, and form a tuple from it.
  ;---------------------------------------------------*/
  
  glr = req->glr;
  p   = glr->data;

  glr->ipsize    = ntohs(glr->ipsize);
  glr->fromsize  = ntohs(glr->fromsize);
  glr->tosize    = ntohs(glr->tosize);

  if ((glr->ipsize != 4) && (glr->ipsize != 16))
  {
    send_reply(req,CMD_NONE_RESP,GLERR_BAD_DATA);
    return;
  }

  rsize = glr->ipsize 
        + glr->fromsize 
	+ glr->tosize 
	+ sizeof(struct graylist_request) 
	- 4;	/* empiracally found --- this is bad XXX */

  if (rsize > req->size)
  {
    (*cv_report)(LOG_DEBUG,"L L","bad size, expected %a got %b",(unsigned long)req->size,(unsigned long)rsize);
    send_reply(req,CMD_NONE_RESP,GLERR_BAD_DATA);
    return;
  }
  
  memset(&tuple,0,sizeof(struct tuple));
  D(tuple.pad    = 0xDECAFBAD;)
  tuple.ctime    = tuple.atime = req->now;
  tuple.fromsize = min(sizeof(tuple.from) - 1,glr->fromsize);
  tuple.tosize   = min(sizeof(tuple.to)   - 1,glr->tosize);
  tuple.f        = 0;

  memcpy(tuple.ip,  p,glr->ipsize);    p += glr->ipsize;
  memcpy(tuple.from,p,tuple.fromsize); p += glr->fromsize;
  memcpy(tuple.to,  p,tuple.tosize);

  tuple.from[tuple.fromsize] = '\0';
  tuple.to  [tuple.tosize]   = '\0';

  if (tuple.fromsize <  glr->fromsize) tuple.f |= F_TRUNCFROM;
  if (tuple.tosize   <  glr->tosize)   tuple.f |= F_TRUNCTO;
  if (glr->ipsize    == 16)            tuple.f |= F_IPv6;

  (*cv_report)(
  	LOG_INFO,
  	"$ $ $ $ $",
  	"tuple: [%a , %b , %c]%d%e",
  	ipv4(tuple.ip),
  	tuple.from,
  	tuple.to,
  	(tuple.f & F_TRUNCFROM) ? " Tf" : "",
  	(tuple.f & F_TRUNCTO)   ? " Tt" : ""
  );

  /*------------------------------------------------
  ; we have a valid GREYLIST request.  Count it.
  ;------------------------------------------------*/
  
  g_requests++;
  g_req_cucurrent++;
  
  /*----------------------------------------------------
  ; check IP address
  ;----------------------------------------------------*/
  
  rc = ip_match(tuple.ip,4);
  
  if (rc == IPCMD_ACCEPT)
  {
    send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
    return;
  }
  else if (rc == IPCMD_REJECT)
  {
    send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
    return;
  }
  
  /*-------------------------------------------------------
  ; check the from and fromdomain lists.
  ;------------------------------------------------------*/
  
  edkey.text  = tuple.from;
  edkey.tsize = tuple.fromsize;
  edvalue     = edomain_search(&edkey,&idx,g_from,g_sfrom);
  
  if (edvalue != NULL)
  {
    edvalue->count++;
    
    if (edvalue->cmd == IPCMD_ACCEPT)
    {
      send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
      return;
    }
    else if (edvalue->cmd == IPCMD_REJECT)
    {
      send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
      return;
    }
    else if (edvalue->cmd == IPCMD_GRAYLIST)
      goto type_graylist_check_to;
    else
      ddt(0);
  }
  else 
  {
    g_fromc++;
    if (g_deffrom == IPCMD_ACCEPT)
    {
      send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
      return;
    }
    else if (g_deffrom == IPCMD_REJECT)
    {
      send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
      return;
    }
  }
  
  at = strchr(tuple.from,'@');
  if (at)
  {
    edkey.text  = at + 1;
    edkey.tsize = strlen(edkey.text);
    edvalue     = edomain_search(&edkey,&idx,g_fromd,g_sfromd);
    if (edvalue != NULL)
    {
      edvalue->count++;
      
      if (edvalue->cmd == IPCMD_ACCEPT)
      {
        send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
        return;
      }
      else if (edvalue->cmd == IPCMD_REJECT)
      {
        send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
        return;
      }
      else if (edvalue->cmd != IPCMD_GRAYLIST)
        ddt(0);
    }
    else 
    {
      g_fromdomainc++;
      if (g_deffromdomain == IPCMD_ACCEPT)
      {
        send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
	return;
      }
      else if (g_deffromdomain == IPCMD_REJECT)
      { 
        send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
	return;
      }
    }
  }
  
  /*-----------------------------------------------------
  ; check the to and todomain lists.
  ;-----------------------------------------------------*/

type_graylist_check_to:

  edkey.text  = tuple.to;
  edkey.tsize = tuple.tosize;
  edvalue     = edomain_search(&edkey,&idx,g_to,g_sto);
  
  if (edvalue != NULL)
  {
    edvalue->count++;
    
    if (edvalue->cmd == IPCMD_ACCEPT)
    {
      send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
      return;
    }
    else if (edvalue->cmd == IPCMD_REJECT)
    {
      send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
      return;
    }
    else if (edvalue->cmd == IPCMD_GRAYLIST)
      goto type_graylist_check_tuple;
    else
      ddt(0);
  }
  else 
  {
    g_toc++;
    if (g_defto == IPCMD_ACCEPT)
    {
      send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
      return;
    }
    else if (g_defto == IPCMD_REJECT)
    {
      send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
      return;
    }
  }
  
  at = strchr(tuple.to,'@');
  if (at)
  {
    edkey.text  = at + 1;
    edkey.tsize = strlen(edkey.text);
    edvalue     = edomain_search(&edkey,&idx,g_tod,g_stod);
    if (edvalue != NULL)
    {
      edvalue->count++;
      
      if (edvalue->cmd == IPCMD_ACCEPT)
      {
        send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
        return;
      }
      else if (edvalue->cmd == IPCMD_REJECT)
      {
        send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
        return;
      }
      else if (edvalue->cmd != IPCMD_GRAYLIST)
        ddt(0);
    }
    else 
    {
      g_todomainc++;
      if (g_deftodomain == IPCMD_ACCEPT)
      {
        send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
	return;
      }
      else if (g_deftodomain == IPCMD_REJECT)
      {
        send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_NAY);
        return;
      }
    }
  }

  /*-------------------------------------------------------
  ; Look up the tuple.  If not found, add it, else update the access time,
  ; and if less than the embargo period, return LATER, else accept.
  ;---------------------------------------------------------*/
  
type_graylist_check_tuple:

  stored = tuple_search(&tuple,&idx);
  
  if (stored == NULL)
  {
    stored  = tuple_allocate();
    memcpy(stored,&tuple,sizeof(struct tuple));
    tuple_add(stored,idx);
    g_graylisted++;
    send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_LATER);
    return;
  }
  
  stored->atime = req->now;

  if ((stored->f & F_WHITELIST))
  {
    send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
    return;
  }

  if (difftime(req->now,stored->ctime) < c_timeout_embargo)
  {
    send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_LATER);
    return;
  }
  
  if ((stored->f & F_WHITELIST) == 0)
  {
    stored->f |= F_WHITELIST;
    g_whitelisted++;
  }
    
  send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
}

/*********************************************************************/

static void send_reply(struct request *req,int type,int response)
{
  struct graylist_response resp;
  
  ddt(req      != NULL);
  ddt(type     >= 0);
  ddt(response >= 0);
  
  resp.version  = htons(VERSION);
  resp.MTA      = req->glr->MTA;
  resp.type     = htons(type);
  resp.response = htons(response);
  
  send_packet(req,&resp,sizeof(struct graylist_response));
}

/********************************************************************/

static void send_packet(struct request *req,void *packet,size_t size)
{
  struct graylist_response *glr = packet;
  ssize_t                   rrc;
  CRC32                     crc;
  
  ddt(req    != NULL);
  ddt(packet != NULL);
  ddt(size   >  0);
  
  crc      = crc32(INIT_CRC32,&glr->version,size - sizeof(unet32));
  crc      = crc32(crc,c_secret,c_secretsize);
  glr->crc = htonl(crc);
  
  do
  {
    rrc = sendto(
    		req->sock,
    		packet,
    		size,
    		0,
    		&req->remote,
    		req->rsize
    	);
    
    if (rrc == -1)
    {
      if (errno == EINTR)
        continue;
      (*cv_report)(LOG_ERR,"$","sendto() = %a",strerror(errno));
      if (errno == EINVAL)
        break;
      sleep(1);
    }
  } while (rrc == -1);
}

/*********************************************************************/

static void cmd_mcp_report(struct request *req,int (*cb)(Stream),int resp)
{
  struct sockaddr remote;
  socklen_t       rsize;
  pid_t           pid;
  int             tcp;
  int             conn;
  Stream          out;
  
  ddt(req != NULL);
  
  memcpy(&remote,&req->remote,sizeof(struct sockaddr));
  tcp = create_socket(c_host,c_port,SOCK_STREAM);

  if (tcp == -1)
  {
    (*cv_report)(LOG_ERR,"","could not create socket for report");
    return;
  }
    
  listen(tcp,5);

  pid = gld_fork();
  if (pid == -1)
  {
    close(tcp);
    send_reply(req,CMD_NONE_RESP,GLERR_CANT_GENERATE_REPORT);
    return;
  }
  else if (pid > 0)	/* parent process */
  {
    send_reply(req,resp,0);
    close(tcp);
    return;
  }

  rsize = sizeof(struct sockaddr);
  conn  = accept(tcp,&remote,&rsize);
  if (conn == -1)
  {
    (*cv_report)(LOG_ERR,"$","accept() = %a",strerror(errno));
    return;
  }
  
  close(tcp);
  out = FHStreamWrite(conn);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"","out of memory?");
    close(conn);
    _exit(0);
  }
  
  (*cb)(out);
  
  StreamFree(out);
  _exit(0);
}

/****************************************************************/

static void cmd_mcp_tofrom(struct request *req,int cmd,int resp)
{
  struct glmcp_request_tofrom *ptf;
  struct emaildomain           edkey;
  EDomain                      value;
  size_t                       index;
  int                          def;
  
  ddt(req != NULL);
  
  ptf = (struct glmcp_request_tofrom *)req->packet;
  
  edkey.text  = (char *)ptf->data;
  edkey.tsize = ntohs(ptf->size);
  edkey.count = 0;
  edkey.cmd   = ntohs(ptf->cmd);

  if(
      (strcmp(edkey.text,"default") == 0)
      || (strcmp(edkey.text,"DEFAULT") == 0)
    )
  {
    def = TRUE;
  }
  else
  {
    def = FALSE;
  }

  (*cv_report)(LOG_DEBUG,"$ i","adding %a as %b",edkey.text,edkey.cmd);
  
  switch(cmd)
  {
    case CMD_MCP_TO:
         if (def)
         {
           g_defto = edkey.cmd;
           g_toc   = edkey.count;
         }
         else
         {
           value = edomain_search(&edkey,&index,g_to,g_sto);
           if (value == NULL)
           {
             edkey.text = dup_string(edkey.text);
             edomain_add_to(&edkey,index);
           }
	   else
	   {
	     value->cmd   = edkey.cmd;
	     value->count = 0;
	   }
	 }
         break;
    case CMD_MCP_TO_DOMAIN:
         if (def)
         {
           g_deftodomain = edkey.cmd;
           g_todomainc   = edkey.count;
         }
         else
         {
           value = edomain_search(&edkey,&index,g_tod,g_stod);
           if (value == NULL)
           {
             edkey.text = dup_string(edkey.text);
             edomain_add_tod(&edkey,index);
           }
	   else
	   {
	     value->cmd   = edkey.cmd;
	     value->count = 0;
	   }
	 }
         break;
    case CMD_MCP_FROM:
         if (def)
         {
           g_deffrom = edkey.cmd;
           g_fromc   = edkey.count;
         }
         else
         {
           value = edomain_search(&edkey,&index,g_from,g_sfrom);
           if (value == NULL)
           {
             edkey.text = dup_string(edkey.text);
             edomain_add_from(&edkey,index);
           }
	   else
	   {
	     value->cmd   = edkey.cmd;
	     value->count = 0;
	   }
	 }
         break;
    case CMD_MCP_FROM_DOMAIN:
         if (def)
         {
           g_deffromdomain = edkey.cmd;
           g_fromdomainc   = edkey.count;
         }
         else
         {
           value = edomain_search(&edkey,&index,g_fromd,g_sfromd);
           if (value == NULL)
           {
             edkey.text = dup_string(edkey.text);
             edomain_add_fromd(&edkey,index);
           }
	   else
	   {
	     value->cmd   = edkey.cmd;
	     value->count = 0;
	   }
	 }
         break;
    default:
         ddt(0);
  }
  
  send_reply(req,resp,0);
}

/*************************************************************************/

