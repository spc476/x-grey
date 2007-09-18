
#include <stdlib.h>
#include <stdio.h>

#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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
static int	send_request	(void *,size_t,void *,size_t,int);
static void	handler_sigalrm	(int);
static void	show_stats	(void);
static void	show_config	(void);
static void	show_report	(int,int);
static void	help		(void);
static void	pager		(int);
static void	iplist		(char *);

/*************************************************************/

extern char **environ;

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
    else if (strncmp(cmd,"iplist ",7) == 0)
      iplist(&cmd[7]);
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
                         void   *req,
                         size_t  reqsize,
			 void   *result,
			 size_t  size,
			 int     expected_response
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
  		reqsize,
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

  rc = send_request(&request,sizeof(request),&data,sizeof(data),CMD_MCP_SHOW_STATS_RESP);
  
  if (rc != ERR_OKAY)
  {
    LineS(StdoutStream,"bad response\n");
    return;
  }

  t = report_time(ntohl(gss->starttime),ntohl(gss->nowtime));
      
  LineSFormat(
      	StdoutStream,
      	"$ L10 L10 L10 L10 L10 L10",
      	"\n"
      	"%a\n"
      	"IPs:               %c\n"
      	"Tuples:            %b\n"
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
  
  rc = send_request(&request,sizeof(request),&data,sizeof(data),CMD_MCP_SHOW_CONFIG_RESP);
  
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
  	"L10 $ $ $ $",
  	"\n"
  	"Max-Tuples:        %a\n"
  	"Timeout-Cleanup:   %b\n"
  	"Timeout-Embargo:   %c\n"
  	"Timeout-Graylist:  %d\n"
  	"Timeout-Whitelist: %e\n"
  	"\n",
  	(unsigned long)ntohl(gsc->max_tuples),
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
  
  rc = send_request(&request,sizeof(request),&data,sizeof(data),resp);
  
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

  StreamWrite(StdoutStream,'\n');
  pager(conn);
  close(conn); 
  StreamWrite(StdoutStream,'\n');
}

/************************************************************/

static void help(void)
{
  LineS(
  	StdoutStream,
  	"exit | quit\t\tquit program\n"
  	"iplist accept a.b.c.d/M\taccept IP addresses\n"
  	"iplist reject a.b.c.d/M\treject IP addresses\n"
  	"iplist graylist a.b.c.d/M\tgreylist IP addresses\n"
  	"show config\t\tshow configuration settings\n"
  	"show stats\t\tshow current statistics\n"
  	"show tuples [all]\tshow all tuples\n"
  	"show tuples whitelist\tshow tuples on whitelist\n"
  	"show iplist\t\tshow accepted/rejected IP addresses\n"
  	"help | ?\t\tthis text\n"
  );
}

/**************************************************************/

static void pager(int fh)
{
  pid_t  child;
  pid_t  pid;
  char  *argv[2];
  int    status;
  int    rc;
  
  ddt(fh >= 0);
  
  pid = fork();
  if (pid == -1)	/* error */
    (*cv_report)(LOG_ERR,"$","fork() = %a",strerror(errno));
  else if (pid > 0)	/* parent */
  {
    child = waitpid(pid,&status,0);
    if (child == (pid_t)-1)
      (*cv_report)(LOG_ERR,"$","waitpid() = %a",strerror(errno));
  }
  else			/* child */
  {
    rc = dup2(fh,STDIN_FILENO);
    if (rc == -1)
      _exit(EXIT_FAILURE);
    
    argv[0] = (char *)c_pager;
    argv[1] = NULL;
    
    execve(c_pager,argv,environ);
    _exit(EXIT_FAILURE);
  }
}

/**********************************************************/

static void iplist(char *cmd)
{
  struct glmcp_request_iplist  ipr;
  byte                         data[1500];
  size_t                       octet;
  char                        *p;
  int                          rc;
  
  ddt(cmd != NULL);
  
  memset(ipr.data,0,16);
  
  ipr.version = htons(VERSION);
  ipr.MTA     = htons(MTA_MCP);
  ipr.type    = htons(CMD_MCP_IPLIST);
  ipr.ipsize  = htons(4);
  ipr.mask    = htons(32);	/* default mask */
  
  p = strchr(cmd,' ');
  if (p == NULL)
    return;

  *p++ = '\0';
  
  if (strcmp(cmd,"accept") == 0)
    ipr.cmd = htons(IPCMD_ACCEPT);
  else if (strcmp(cmd,"reject") == 0)
    ipr.cmd = htons(IPCMD_REJECT);
  else if (strcmp(cmd,"graylist") == 0)
    ipr.cmd = htons(IPCMD_GRAYLIST);
  else
    return;

  for (octet = 0 ; octet < 4 ; octet++)
  {
    ipr.data[octet] = strtoul(p,&p,10);
    if (*p != '.') break;
    p++;
  }
  
  if (*p == '/')
    ipr.mask = htons(strtoul(p+1,NULL,10));
  
  rc = send_request(&ipr,sizeof(ipr),&data,sizeof(data),CMD_MCP_IPLIST_RESP);
  
  if (rc != ERR_OKAY)
  LineS(StdoutStream,"bad response\n");
}

/******************************************************************/

