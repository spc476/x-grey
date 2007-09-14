
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
  m_sock = create_socket(c_host,c_port);

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
      
    if (!emptynull_string(cmd))
      add_history(cmd);

    if (strcmp(cmd,"show stats") == 0)
    {
      struct glmcp_request              request;
      struct glmcp_response_show_stats *gss;
      byte                              data[1500];
      int                               rc;
      char                             *t;
      
      gss             = (struct glmcp_response_show_stats *)&data;
      request.version = htons(VERSION);
      request.MTA     = htons(MTA_MCP);
      request.type    = htons(CMD_MCP_SHOW_STATS);
      
      rc = send_request(&request,&data,sizeof(data),CMD_MCP_SHOW_STATS_RESP);
  
      if (rc != ERR_OKAY)
      {
        LineS(StdoutStream,"bad response\n");
        continue;
      }
      
      t = report_time(ntohl(gss->starttime),ntohl(gss->nowtime));
      
      LineSFormat(
      	StdoutStream,
      	"$ L L L L L",
      	"%a\n"
      	"Tuples:            %b\n"
      	"Graylisted:        %c\n"
      	"Whitelisted:       %d\n"
      	"Graylist-Expired:  %e\n"
      	"Whitelist-Expired: %f\n"
      	"\n",
        t,
        (unsigned long)ntohl(gss->tuples),
        (unsigned long)ntohl(gss->graylisted),
        (unsigned long)ntohl(gss->whitelisted),
        (unsigned long)ntohl(gss->graylist_expired),
        (unsigned long)ntohl(gss->whitelist_expired)
       );
      MemFree(t);
    }
    else if (strcmp(cmd,"show config") == 0)
    {
    }
      
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

