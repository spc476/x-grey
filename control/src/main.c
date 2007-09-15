
#include <stdlib.h>
#include <stdio.h>

#include <syslog.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>

#include "../../common/src/graylist.h"
#include "../../common/src/util.h"

#include "globals.h"

/**************************************************************/

static int	cmdline		(void);
static int	send_request	(struct glmcp_request *,void *,size_t,int);
static void	handler_sigalrm	(int);
static void	show_stats	(void);
static void	show_config	(void);
static void	show_report	(int,int);
static void	help		(void);

/*************************************************************/

static volatile sig_atomic_t mf_sigalrm;
static int                   m_sock;

/************************************************************/

int main(int argc,char *argv[])
{
  MemInit();
  DdtInit();
  StreamInit();
  
  GlobalsInit(argc,argv);
  
  set_signal(SIGALRM,handler_sigalrm);
  m_sock = create_socket(c_host,c_port,SOCK_DGRAM);

  cmdline();
  
  return(EXIT_SUCCESS);
}

/***************************************************************/

static int cmdline(void)
{
  char  prompt[BUFSIZ];
  char *cmd;
  
  sprintf(prompt,"%s>",c_log_id);
  using_history();
  
  while(1)
  {
    cmd = readline(prompt);
    
    if (cmd == NULL) break;
    
    if (strcmp(cmd,"exit") == 0)
      break;
      
    if (strcmp(cmd,"quit") == 0)
      break;
      
    if (!emptynull_string(cmd))
      add_history(cmd);

    if (strcmp(cmd,"show stats") == 0)
      show_stats();
    else if (strcmp(cmd,"show config") == 0)
      show_config();
    else if (strcmp(cmd,"show iplist") == 0)
      show_report(CMD_MCP_SHOW_IPLIST,CMD_MCP_SHOW_IPLIST_RESP);
    else if (strcmp(cmd,"show tuples") == 0)
      show_report(CMD_MCP_SHOW_TUPLE_ALL,CMD_MCP_SHOW_TUPLE_ALL_RESP);
    else if (strcmp(cmd,"show tuples all") == 0)
      show_report(CMD_MCP_SHOW_TUPLE_ALL,CMD_MCP_SHOW_TUPLE_ALL_RESP);
    else if (strcmp(cmd,"show tuples whitelist") == 0)
      show_report(CMD_MCP_SHOW_WHITELIST,CMD_MCP_SHOW_WHITELIST_RESP);
    else if (strcmp(cmd,"help") == 0)
      help();
    else if (strcmp(cmd,"?") == 0)
      help();
    
    StreamFlush(StdoutStream);  
    free(cmd);
  }
  free(cmd);
  LineS(StdoutStream,"\n");
}

/****************************************************************/

static int send_request(
			 struct glmcp_request *req,
			 void                 *result,
			 size_t                size,
			 int                   expected_response
		       )

{
  struct sockaddr_in     sip;
  struct glmcp_response *gr = result;
  socklen_t              sipsize;
  ssize_t                rrc;
  
  ddt(req    != NULL);
  ddt(result != NULL);
  ddt(size   >  0);
  
  rrc = sendto(
  		m_sock,
  		req,
  		sizeof(struct glmcp_request),
  		0,
  		(struct sockaddr *)&c_raddr,
  		c_raddrsize
  	);
  if (rrc == -1)
  {
    (*cv_report)(LOG_ERR,"$","sendto() = %a",strerror(errno));
    return(ERR_ERR);
  }
  
  alarm(c_timeout);
  
  sipsize = sizeof(sip);
  rrc     = recvfrom(
  		m_sock,
  		result,
  		size,
  		0,
  		(struct sockaddr *)&sip,
  		&sipsize
  	);
  
  alarm(0);
  
  if (rrc == -1)
  {
    if (errno != EINTR)
      (*cv_report)(LOG_ERR,"$","recvfrom() = %a",strerror(errno));
    return(ERR_ERR);
  }
  
  if (ntohs(gr->version) != VERSION)
  {
    (*cv_report)(LOG_ERR,"","received response from wrong version");
    return(ERR_ERR);
  }
  
  if (ntohs(gr->MTA) != MTA_MCP)
  {
    (*cv_report)(LOG_ERR,"","we got a reponse for something else");
    return(ERR_ERR);
  }
  
  if (ntohs(gr->type) != expected_response)
  {
    (*cv_report)(LOG_ERR,"","received wrong response");
    return(ERR_ERR);
  }
  
  return(ERR_OKAY);
}

/*********************************************************************/

static void handler_sigalrm(int sig)
{
  mf_sigalrm = 1;
}

/********************************************************************/

static void show_stats(void)
{
  struct glmcp_request              request;
  struct glmcp_response_show_stats *gss;
  byte                              data[1500];
  int                               rc;
  char                             *t;

  gss             = (struct glmcp_response_show_stats *)data;
  request.version = htons(VERSION);
  request.MTA     = htons(MTA_MCP);
  request.type    = htons(CMD_MCP_SHOW_STATS);

  rc = send_request(&request,&data,sizeof(data),CMD_MCP_SHOW_STATS_RESP);
  
  if (rc != ERR_OKAY)
  {
    LineS(StdoutStream,"bad response\n");
    return;
  }

  t = report_time(ntohl(gss->starttime),ntohl(gss->nowtime));
      
  LineSFormat(
      	StdoutStream,
      	"$ L L L L L L",
      	"\n"
      	"%a\n"
      	"Tuples:            %b\n"
      	"IPs:               %c\n"
      	"Graylisted:        %d\n"
      	"Whitelisted:       %e\n"
      	"Graylist-Expired:  %f\n"
      	"Whitelist-Expired: %g\n"
      	"\n",
        t,
        (unsigned long)ntohl(gss->tuples),
        (unsigned long)ntohl(gss->ips),
        (unsigned long)ntohl(gss->graylisted),
        (unsigned long)ntohl(gss->whitelisted),
        (unsigned long)ntohl(gss->graylist_expired),
        (unsigned long)ntohl(gss->whitelist_expired)
       );
  MemFree(t);
}

/******************************************************************/

static void show_config(void)
{
  struct glmcp_request               request;
  struct glmcp_response_show_config *gsc;
  byte                               data[1500];
  int                                rc;
  char                              *cleanup;
  char                              *embargo;
  char                              *graylist;
  char                              *whitelist;
  
  gsc = (struct glmcp_response_show_config *)data;
  request.version = htons(VERSION);
  request.MTA     = htons(MTA_MCP);
  request.type    = htons(CMD_MCP_SHOW_CONFIG);
  
  rc = send_request(&request,&data,sizeof(data),CMD_MCP_SHOW_CONFIG_RESP);
  
  if (rc != ERR_OKAY)
  {
    LineS(StdoutStream,"bad response\n");
    return;
  }
  
  cleanup   = report_delta(ntohl(gsc->timeout_cleanup));
  embargo   = report_delta(ntohl(gsc->timeout_embargo));
  graylist  = report_delta(ntohl(gsc->timeout_gray));
  whitelist = report_delta(ntohl(gsc->timeout_white));
  
  LineSFormat(
  	StdoutStream,
  	"L L $ $ $ $",
  	"\n"
  	"Max-Tuples:        %a\n"
  	"Max-IP:            %b\n"
  	"Timeout-Cleanup:   %c\n"
  	"Timeout-Embargo:   %d\n"
  	"Timeout-Graylist:  %e\n"
  	"Timeout-Whitelist: %f\n"
  	"\n",
  	(unsigned long)ntohl(gsc->max_tuples),
  	(unsigned long)ntohl(gsc->max_ips),
  	cleanup,
  	embargo,
  	graylist,
  	whitelist
    );

  MemFree(whitelist);
  MemFree(graylist);
  MemFree(embargo);
  MemFree(cleanup);
}

/*************************************************************/

static void show_report(int req,int resp)
{
  struct glmcp_request   request;
  struct glmcp_response *gr;
  int                    conn;
  Stream                 in;
  byte                   data[1500];
  char                  *line;
  int                    rc;
  
  gr = (struct glmcp_response *)data;
  
  request.version = htons(VERSION);
  request.MTA     = htons(MTA_MCP);
  request.type    = htons(req);
  
  rc = send_request(&request,&data,sizeof(data),resp);
  
  if (rc != ERR_OKAY)
  {
    LineS(StdoutStream,"bad response\n");
    return;
  }
  
  conn = create_socket(c_host,c_port,SOCK_STREAM);
  if (conn == -1)
  {
    LineS(StdoutStream,"cannot connect to gld");
    return;
  }
  
  rc = connect(conn,(struct sockaddr *)&c_raddr,c_raddrsize);
  if (rc == -1)
  {
    LineS(StdoutStream,"cannot connect to gld");
    return;
  }
  
  in = FHStreamRead(conn);
  
  StreamWrite(StdoutStream,'\n');
  
  while(!StreamEOF(in))
  {
    line = LineSRead(in);
    if (empty_string(line)) continue;
    LineS(StdoutStream,line);
    StreamWrite(StdoutStream,'\n');
    MemFree(line);
  }
  
  StreamWrite(StdoutStream,'\n');
  StreamFree(in);
}

/************************************************************/

static void help(void)
{
  LineS(
  	StdoutStream,
  	"exit | quit\t\tquit program\n"
  	"show config\t\tshow configuration settings\n"
  	"show stats\t\tshow current statistics\n"
  	"show tuples [all]\tshow all tuples\n"
  	"show tuples whitelist\tshow tuples on whitelist\n"
  	"show iplist\t\tshow accepted/rejected IP addresses\n"
  	"help | ?\t\tthis text\n"
  );
}

/**************************************************************/
