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

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>
#include <cgilib/types.h>

#include "../../common/src/greylist.h"
#include "../../common/src/globals.h"
#include "../../common/src/util.h"
#include "../../common/src/crc32.h"

#include "tuple.h"
#include "globals.h"
#include "signals.h"
#include "server.h"
#include "iplist.h"
#include "emaildomain.h"

/********************************************************************/

void 		 type_greylist		(struct request *,int);
void		 type_tuple_remove	(struct request *);
static void	 mainloop		(int);
static void	 send_reply		(struct request *,int,int,int);
static void	 send_packet		(struct request *,void *,size_t);
static void	 cmd_mcp_report		(struct request *,int (*)(Stream),int);
static void	 cmd_mcp_tofrom		(struct request *,int,int);
static int	 greylist_sanitize_req	(struct tuple *,struct request *);
static void	 log_tuple		(struct tuple *,int,int);

/********************************************************************/

int main(int argc,char *argv[])
{
  int sock;
  
  MemInit();
  DdtInit();
  StreamInit();

  GlobalsInit(argc,argv);

  sock = create_socket(c_host,c_port,SOCK_DGRAM);
  if (sock == -1)
    exit(EXIT_FAILURE); 
    
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
    rrc       = recvfrom(
    	sock,
    	req.packet.data,
    	sizeof(req.packet.data),
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
    req.glr  = &req.packet.req;
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
      send_reply(&req,CMD_NONE_RESP,GLERR_VERSION_NOT_SUPPORTED,REASON_NONE);
      continue;
    }
    
    req.glr->type = ntohs(req.glr->type);

    switch(req.glr->type)
    {
      case CMD_NONE:
           send_reply(&req,CMD_NONE_RESP,GLERR_OKAY,REASON_NONE);
           break;
      case CMD_GREYLIST:
           type_greylist(&req,CMD_GREYLIST_RESP);
           break;
      case CMD_WHITELIST:
           type_greylist(&req,CMD_WHITELIST_RESP);
	   break;
      case CMD_TUPLE_REMOVE:
           type_tuple_remove(&req);
           break;
      case CMD_MCP_SHOW_STATS:
           {
             struct glmcp_response_show_stats stats;
	     size_t ave;

	     if (g_cleanup_count)
	       ave = (size_t)(((double)(g_requests - g_req_cucurrent)
	     		   / (double)(g_cleanup_count)) + 0.5);
	     else
	       ave = 0;
             
	     stats.crc = stats.pad     = htons(0);	/* VVV */
             stats.version             = htons(VERSION);
             stats.MTA                 = req.glr->MTA;
             stats.type                = htons(CMD_MCP_SHOW_STATS_RESP);
             stats.starttime           = htonl(c_starttime);
             stats.nowtime             = htonl(req.now);
             stats.tuples              = htonl(g_poolnum);
	     stats.ips                 = htonl(g_ipcnt);
	     stats.ip_greylist         = htonl(g_ip_cmdcnt[IFT_GREYLIST]);
	     stats.ip_accept           = htonl(g_ip_cmdcnt[IFT_ACCEPT]);
	     stats.ip_reject           = htonl(g_ip_cmdcnt[IFT_REJECT]);
             stats.greylisted          = htonl(g_greylisted);
             stats.whitelisted         = htonl(g_whitelisted);
             stats.greylist_expired    = htonl(g_greylist_expired);
             stats.whitelist_expired   = htonl(g_whitelist_expired);
             stats.requests            = htonl(g_requests);
             stats.requests_cu         = htonl(g_req_cu);
	     stats.requests_cu_max     = htonl(g_req_cumax);
	     stats.requests_cu_ave     = htonl(ave);
	     stats.tuples_read         = htonl(g_tuples_read);
	     stats.tuples_read_cu      = htonl(g_tuples_read_cu);
	     stats.tuples_read_cu_max  = htonl(g_tuples_read_cumax);
	     stats.tuples_read_cu_ave  = htonl(0);
	     stats.tuples_write        = htonl(g_tuples_write);
	     stats.tuples_write_cu     = htonl(g_tuples_write_cu);
	     stats.tuples_write_cu_max = htonl(g_tuples_write_cumax);
	     stats.tuples_write_cu_ave = htonl(0);
	     stats.tuples_low          = htonl(g_tuples_low);
	     stats.tuples_high         = htonl(g_tuples_high);
	     stats.from                = htonl(g_sfrom  + 1);
	     stats.from_greylist       = htonl(g_from_cmdcnt[IFT_GREYLIST]);
	     stats.from_accept         = htonl(g_from_cmdcnt[IFT_ACCEPT]);
	     stats.from_reject         = htonl(g_from_cmdcnt[IFT_REJECT]);
	     stats.fromd               = htonl(g_sfromd + 1);
	     stats.fromd_greylist      = htonl(g_fromd_cmdcnt[IFT_GREYLIST]);
	     stats.fromd_accept        = htonl(g_fromd_cmdcnt[IFT_ACCEPT]);
	     stats.fromd_reject        = htonl(g_fromd_cmdcnt[IFT_REJECT]);
	     stats.to                  = htonl(g_sto    + 1);
	     stats.to_greylist         = htonl(g_to_cmdcnt[IFT_GREYLIST]);
	     stats.to_accept           = htonl(g_to_cmdcnt[IFT_ACCEPT]);
	     stats.to_reject           = htonl(g_to_cmdcnt[IFT_REJECT]);
	     stats.tod                 = htonl(g_stod   + 1);
	     stats.tod_greylist        = htonl(g_tod_cmdcnt[IFT_GREYLIST]);
	     stats.tod_accept          = htonl(g_tod_cmdcnt[IFT_ACCEPT]);
	     stats.tod_reject          = htonl(g_tod_cmdcnt[IFT_REJECT]);
             
             send_packet(&req,&stats,sizeof(stats)); /* VVV - uninit mem */
           }
           break;
      case CMD_MCP_SHOW_CONFIG:
           {
             struct glmcp_response_show_config config;
             
	     config.crc = config.pad = htons(0);
             config.version          = htons(VERSION);
             config.MTA              = req.glr->MTA;
             config.type             = htons(CMD_MCP_SHOW_CONFIG_RESP);
             config.max_tuples       = htonl(c_poolmax);
             config.timeout_cleanup  = htonl(c_time_cleanup);
             config.timeout_embargo  = htonl(c_timeout_embargo);
             config.timeout_grey     = htonl(c_timeout_grey);
             config.timeout_white    = htonl(c_timeout_white);
             
             send_packet(&req,&config,sizeof(config));
           }
           break;
      case CMD_MCP_SHOW_IPLIST:
           cmd_mcp_report(&req,ip_print,CMD_MCP_SHOW_IPLIST_RESP);
           break;
      case CMD_MCP_SHOW_TUPLE:
           cmd_mcp_report(&req,tuple_dump_stream,CMD_MCP_SHOW_TUPLE_RESP);
           break;
      case CMD_MCP_SHOW_TUPLE_ALL:
           cmd_mcp_report(&req,tuple_all_dump_stream,CMD_MCP_SHOW_TUPLE_ALL_RESP);
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
               send_reply(&req,CMD_NONE_RESP,GLERR_IPADDR_NOT_SUPPORTED,REASON_NONE);
               break;
             }

	     t = ipv4(pip->data);
	     (*cv_report)(LOG_DEBUG,"$ i i","about to add %a/%b %c",t,mask,cmd);
             
             ip_add_sm(pip->data,4,mask,cmd);
             send_reply(&req,CMD_MCP_IPLIST_RESP,0,REASON_NONE);
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
           send_reply(&req,CMD_NONE_RESP,GLERR_TYPE_NOT_SUPPORTED,REASON_NONE);
           break;
    }
  }
}

/*******************************************************************/

void type_tuple_remove(struct request *req)
{
  struct tuple tuple;
  Tuple        stored;
  int          rc;
  size_t       idx;
  
  ddt(req != NULL);
  
  rc = greylist_sanitize_req(&tuple,req);
  if (rc != ERR_OKAY) return;

  stored = tuple_search(&tuple,&idx);

  if (stored != NULL)
    stored->f |= F_REMOVE;

  log_tuple(&tuple,IFT_REMOVE,REASON_GREYLIST);
  send_reply(req,CMD_TUPLE_REMOVE_RESP,0,REASON_GREYLIST);
}

/********************************************************************/

void type_greylist(struct request *req,int response)
{
  struct tuple             tuple;
  struct emaildomain       edkey;
  EDomain                  edvalue;
  Tuple                    stored;
  size_t                   idx;
  char                    *at;
  int                      rc;

  ddt(req != NULL);

  rc = greylist_sanitize_req(&tuple,req);
  if (rc != ERR_OKAY) return;
  
  g_requests++;
  g_req_cucurrent++;
  
  /*----------------------------------------------------
  ; check IP address
  ;----------------------------------------------------*/

  rc = ip_match(tuple.ip,4);
  g_ip_cmdcnt[rc]++;
  
  if (rc != IFT_GREYLIST)
  {
    log_tuple(&tuple,rc,REASON_IP);
    send_reply(req,response,rc,REASON_IP);
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
    g_from_cmdcnt[edvalue->cmd]++;

    if (edvalue->cmd == IFT_GREYLIST)
      goto type_greylist_check_to;
    else
    {
      log_tuple(&tuple,edvalue->cmd,REASON_FROM);
      send_reply(req,response,edvalue->cmd,REASON_FROM);
      return;
    }
  }
  else 
  {
    g_fromc++;
    g_from_cmdcnt[g_deffrom]++;
    if (g_deffrom != IFT_GREYLIST)
    {
      log_tuple(&tuple,g_deffrom,REASON_FROM);
      send_reply(req,response,g_deffrom,REASON_FROM);
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
      g_fromd_cmdcnt[edvalue->cmd]++; 

      if (edvalue->cmd != IFT_GREYLIST)
      {
        log_tuple(&tuple,edvalue->cmd,REASON_FROM_DOMAIN);
        send_reply(req,response,edvalue->cmd,REASON_FROM_DOMAIN);
	return;
      }
    }
    else 
    {
      g_fromdomainc++;
      g_fromd_cmdcnt[g_deffromdomain]++;
      if (g_deffromdomain != IFT_GREYLIST)
      {
        log_tuple(&tuple,g_deffromdomain,REASON_FROM_DOMAIN);
        send_reply(req,response,g_deffromdomain,REASON_FROM_DOMAIN);
	return;
      }
    }
  }
  
  /*-----------------------------------------------------
  ; check the to and todomain lists.
  ;-----------------------------------------------------*/

type_greylist_check_to:

  edkey.text  = tuple.to;
  edkey.tsize = tuple.tosize;
  edvalue     = edomain_search(&edkey,&idx,g_to,g_sto);
  
  if (edvalue != NULL)
  {
    edvalue->count++;
    g_to_cmdcnt[edvalue->cmd]++;

    if (edvalue->cmd == IFT_GREYLIST)
      goto type_greylist_check_tuple;
    else
    {
      log_tuple(&tuple,edvalue->cmd,REASON_TO);
      send_reply(req,response,edvalue->cmd,REASON_TO);
      return;
    }
  }
  else 
  {
    g_toc++;
    g_to_cmdcnt[g_defto]++;
    if (g_defto != IFT_GREYLIST)
    {
      log_tuple(&tuple,g_defto,REASON_TO);
      send_reply(req,response,g_defto,REASON_TO);
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
      g_tod_cmdcnt[edvalue->cmd]++;
      
      if (edvalue->cmd != IFT_GREYLIST)
      {
        log_tuple(&tuple,edvalue->cmd,REASON_TO_DOMAIN);
        send_reply(req,response,edvalue->cmd,REASON_TO_DOMAIN);
	return;
      }
    }
    else 
    {
      g_todomainc++;
      g_tod_cmdcnt[g_deftodomain]++;

      if (g_deftodomain != IFT_GREYLIST)
      {
        log_tuple(&tuple,g_deftodomain,REASON_TO_DOMAIN);
        send_reply(req,response,g_deftodomain,REASON_TO_DOMAIN);
	return;
      }
    }
  }

  /*-------------------------------------------------------
  ; Look up the tuple.  If not found, add it, else update the access time,
  ; and if less than the embargo period, return LATER, else accept.
  ;---------------------------------------------------------*/

type_greylist_check_tuple:

  stored = tuple_search(&tuple,&idx);
  
  if (stored == NULL)
  {
    stored  = tuple_allocate();
    
    /*--------------------------------------------------------------------
    ; quick bug fix here---if we've gotten too many requests, we simple
    ; reset the pool to 0.  But the rest of the code doesn't know that
    ; (until now).  So a quick hack to fix that problem.  Basically, if
    ; g_poolnum is 0, then idx *has* to be 0.  That only happens in two
    ; cases---when the program first start ups, and when the program has
    ; received too many requests in a short period of time and just gives
    ; up and resets back to 0.
    ;--------------------------------------------------------------------*/
    
    if (g_poolnum == 0) idx = 0;
    
    memcpy(stored,&tuple,sizeof(struct tuple));
    tuple_add(stored,idx);
    g_greylisted++;
    
    if (req->glr->type == CMD_WHITELIST)
    {
      stored->f |= F_WHITELIST;
      g_whitelisted++;
      log_tuple(&tuple,IFT_ACCEPT,REASON_WHITELIST);
      send_reply(req,response,IFT_ACCEPT,REASON_WHITELIST);
    }
    else
    {
      log_tuple(&tuple,IFT_GREYLIST,REASON_GREYLIST);
      send_reply(req,response,IFT_GREYLIST,REASON_GREYLIST);
    }
    
    return;
  }
  
  stored->atime = req->now;
  
  if (req->glr->type == CMD_WHITELIST)
  {
    if ((stored->f & F_WHITELIST) == 0)
    {
      stored->f |= F_WHITELIST;
      g_whitelisted++;
    }
  }
  
  if ((stored->f & F_WHITELIST))
  {
    log_tuple(&tuple,IFT_ACCEPT,REASON_WHITELIST);
    send_reply(req,response,IFT_ACCEPT,REASON_WHITELIST);
    return;
  }

  if (difftime(req->now,stored->ctime) < c_timeout_embargo)
  {
    log_tuple(&tuple,IFT_GREYLIST,REASON_GREYLIST);
    send_reply(req,response,IFT_GREYLIST,REASON_GREYLIST);
    return;
  }
  
  if ((stored->f & F_WHITELIST) == 0)
  {
    stored->f |= F_WHITELIST;
    g_whitelisted++;
  }
  
  log_tuple(&tuple,IFT_ACCEPT,REASON_WHITELIST);
  send_reply(req,response,IFT_ACCEPT,REASON_WHITELIST);
}

/*********************************************************************/

static int greylist_sanitize_req(struct tuple *tuple,struct request *req)
{
  struct greylist_request *glr;
  byte                    *p;
  size_t                   rsize;
  
  ddt(tuple != NULL);
  ddt(req   != NULL);
  
  glr = req->glr;
  p   = glr->data;

  glr->ipsize    = ntohs(glr->ipsize);
  glr->fromsize  = ntohs(glr->fromsize);
  glr->tosize    = ntohs(glr->tosize);

  if ((glr->ipsize != 4) && (glr->ipsize != 16))
  {
    send_reply(req,CMD_NONE_RESP,GLERR_BAD_DATA,REASON_NONE);
    return(ERR_ERR);
  }

  rsize = glr->ipsize 
        + glr->fromsize 
	+ glr->tosize 
	+ sizeof(struct greylist_request) 
	- 4;	/* empiracally found --- this is bad XXX */

  if (rsize > req->size)
  {
    (*cv_report)(LOG_DEBUG,"L L","bad size, expected %a got %b",(unsigned long)req->size,(unsigned long)rsize);
    send_reply(req,CMD_NONE_RESP,GLERR_BAD_DATA,REASON_NONE);
    return(ERR_ERR);
  }
  
  memset(tuple,0,sizeof(struct tuple));
  D(tuple->pad    = 0xDECAFBAD;)
  tuple->ctime    = tuple->atime = req->now;
  tuple->fromsize = min(sizeof(tuple->from) - 1,glr->fromsize);
  tuple->tosize   = min(sizeof(tuple->to)   - 1,glr->tosize);
  tuple->f        = 0;

  memcpy(tuple->ip,  p,glr->ipsize);     p += glr->ipsize;
  memcpy(tuple->from,p,tuple->fromsize); p += glr->fromsize;
  memcpy(tuple->to,  p,tuple->tosize);

  tuple->from[tuple->fromsize] = '\0';
  tuple->to  [tuple->tosize]   = '\0';

  if (tuple->fromsize <  glr->fromsize) tuple->f |= F_TRUNCFROM;
  if (tuple->tosize   <  glr->tosize)   tuple->f |= F_TRUNCTO;
  if (glr->ipsize    == 16)             tuple->f |= F_IPv6;

  return(ERR_OKAY);
}

/*****************************************************************************/

static void send_reply(struct request *req,int type,int response,int why)
{
  struct greylist_response resp;
  
  ddt(req      != NULL);
  ddt(type     >= 0);
  ddt(response >= 0);
  
  resp.crc      = htons(0);
  resp.version  = htons(VERSION);
  resp.MTA      = req->glr->MTA;
  resp.type     = htons(type);
  resp.response = htons(response);
  resp.why      = htons(why);
  
  send_packet(req,&resp,sizeof(struct greylist_response));
}

/********************************************************************/

static void send_packet(struct request *req,void *packet,size_t size)
{
  struct greylist_response *glr = packet;
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
    send_reply(req,CMD_NONE_RESP,GLERR_CANT_GENERATE_REPORT,REASON_NONE);
    (*cv_report)(LOG_ERR,"","could not create socket for report");
    return;
  }

  /* listen(tcp,5); */
    
  pid = gld_fork();
  if (pid == -1)
  {
    close(tcp);
    send_reply(req,CMD_NONE_RESP,GLERR_CANT_GENERATE_REPORT,REASON_NONE);
    return;
  }
  else if (pid > 0)	/* parent process */
  {
    send_reply(req,resp,0,REASON_NONE);
    close(tcp);
    return;
  }

  listen(tcp,5);
  alarm(10);

  rsize = sizeof(struct sockaddr);
  conn  = accept(tcp,&remote,&rsize);
  
  /*-----------------------------------------------------
  ; used to loop around on EINTR.  No longer, as if we're
  ; interrupted, it's probably because we want to kill the
  ; process, or the accept() is taking too darned long.
  ;------------------------------------------------------*/

  if (conn == -1)
  {
    (*cv_report)(LOG_ERR,"$","accept() = %a",strerror(errno));
    _exit(0);
  }
  
  alarm(0);
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
  
  ptf = &req->packet.mcp_tofrom;
  
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
  
  /*-------------------------------------------------------
  ; basically, if the command is IFT_REMOVE and the context
  ; doesn't make sense (like trying to remove an entry that
  ; doesn't exist, or trying to remove the default setting)
  ; we silently ignore the condition.  Why be overly chatty
  ; about this?  See?  I'm fully justified in this regard.
  ;--------------------------------------------------------*/
  
  switch(cmd)
  {
    case CMD_MCP_TO:
         if (def)
         {
           if (edkey.cmd != IFT_REMOVE)
           {
             g_defto = edkey.cmd;
             g_toc   = edkey.count;
           }
         }
         else
         {
           value = edomain_search(&edkey,&index,g_to,g_sto);
           if (value == NULL)
           {
             if (edkey.cmd != IFT_REMOVE)
             {
               edkey.text = dup_string(edkey.text);
               edomain_add_to(&edkey,index);
             }
           }
	   else
	   {
	     if (edkey.cmd == IFT_REMOVE)
	       edomain_remove_to(index);
	     else
	     {
	       value->cmd   = edkey.cmd;
	       value->count = 0;
	     }
	   }
	 }
         break;
    case CMD_MCP_TO_DOMAIN:
         if (def)
         {
           if (edkey.cmd != IFT_REMOVE)
           {
             g_deftodomain = edkey.cmd;
             g_todomainc   = edkey.count;
           }
         }
         else
         {
           value = edomain_search(&edkey,&index,g_tod,g_stod);
           if (value == NULL)
           {
             if (edkey.cmd != IFT_REMOVE)
             {
               edkey.text = dup_string(edkey.text);
               edomain_add_tod(&edkey,index);
             }
           }
	   else
	   {
	     if (edkey.cmd == IFT_REMOVE)
	       edomain_remove_tod(index);
	     else
	     {
	       value->cmd   = edkey.cmd;
	       value->count = 0;
	     }
	   }
	 }
         break;
    case CMD_MCP_FROM:
         if (def)
         {
           if (edkey.cmd != IFT_REMOVE)
           {
             g_deffrom = edkey.cmd;
             g_fromc   = edkey.count;
           }
         }
         else
         {
           value = edomain_search(&edkey,&index,g_from,g_sfrom);
           if (value == NULL)
           {
             if (edkey.cmd != IFT_REMOVE)
             {
               edkey.text = dup_string(edkey.text);
               edomain_add_from(&edkey,index);
             }
           }
	   else
	   {
	     if (edkey.cmd == IFT_REMOVE)
	       edomain_remove_from(index);
	     else
	     {
	       value->cmd   = edkey.cmd;
	       value->count = 0;
	     }
	   }
	 }
         break;
    case CMD_MCP_FROM_DOMAIN:
         if (def)
         {
           if (edkey.cmd != IFT_REMOVE)
           {
             g_deffromdomain = edkey.cmd;
             g_fromdomainc   = edkey.count;
           }
         }
         else
         {
           value = edomain_search(&edkey,&index,g_fromd,g_sfromd);
           if (value == NULL)
           {
             if (edkey.cmd != IFT_REMOVE)
             {
               edkey.text = dup_string(edkey.text);
               edomain_add_fromd(&edkey,index);
             }
           }
	   else
	   {
	     if (edkey.cmd == IFT_REMOVE)
	       edomain_remove_fromd(index);
	     else
	     {
	       value->cmd   = edkey.cmd;
	       value->count = 0;
	     }
	   }
	 }
         break;
    default:
         ddt(0);
  }
  
  send_reply(req,resp,0,REASON_NONE);
}

/*************************************************************************/

static void log_tuple(struct tuple *tuple,int rc,int why)
{
  (*cv_report)(
  	LOG_INFO,
  	"$ $ $ $ $ $ $",
  	"tuple: [%a , %b , %c]%d%e %f %g",
  	ipv4(tuple->ip),
  	tuple->from,
  	tuple->to,
  	(tuple->f & F_TRUNCFROM) ? " Tf" : "",
  	(tuple->f & F_TRUNCTO)   ? " Tt" : "",
  	ci_map_chars(rc, c_ift,   C_IFT),
  	ci_map_chars(why,c_reason,C_REASONS)
  );
}

/************************************************************************/

