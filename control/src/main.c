
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include <cgilib/util.h>
#include <cgilib/types.h>

#include "../../common/src/graylist.h"
#include "../../common/src/globals.h"
#include "../../common/src/util.h"

#include "globals.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

/**************************************************************/

static void	cmdline			(void);
static void	process_cmdline		(char *);
static void	show			(String *,size_t);
static void	iplist			(String *,size_t);
static void	tofrom			(String *,size_t,int,int,int);

static void	show_stats		(void);
static void	show_config		(void);
static void	show_report		(int,int);

static void	iplist_file		(char *);
static void	iplist_file_relaydelay	(char *);

static int	send_request		(void *,size_t,void *,size_t,int);
static void	handler_sigalrm		(int);
static void	help			(void);
static void	pager			(int);

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

static void cmdline(void)
{
  char  prompt[BUFSIZ];
  char *cmd;
  
  sprintf(prompt,"%s>",c_log_id);
  using_history();
  
  while(1)
  {
    cmd = readline(prompt);
    
    if (cmd == NULL) break;
    if (empty_string(cmd)) continue;
    
    trim_space(cmd);
    
    if (strcmp(cmd,"exit") == 0)
      break;
      
    if (strcmp(cmd,"quit") == 0)
      break;

    if (!emptynull_string(cmd))
      add_history(cmd);
    
    process_cmdline(cmd);
    
    StreamFlush(StdoutStream);  
    free(cmd);
  }
  free(cmd);
  LineS(StdoutStream,"\n");
}

/****************************************************************/

static void process_cmdline(char *cmd)
{
  String *cmdline;
  size_t  cmds;
  
  cmdline = split(&cmds,cmd);
  
  if (strcmp(cmdline[0].d,"show") == 0)
    show(cmdline,cmds);
  else if (strcmp(cmdline[0].d,"iplist") == 0)
    iplist(cmdline,cmds);
  else if (strcmp(cmdline[0].d,"to") == 0)
    tofrom(cmdline,cmds,CMD_MCP_TO,CMD_MCP_TO_RESP,FALSE);
  else if (strcmp(cmdline[0].d,"to-domain") == 0)
    tofrom(cmdline,cmds,CMD_MCP_TO_DOMAIN,CMD_MCP_TO_DOMAIN_RESP,TRUE);
  else if (strcmp(cmdline[0].d,"from") == 0)
    tofrom(cmdline,cmds,CMD_MCP_FROM,CMD_MCP_FROM_RESP,FALSE);
  else if (strcmp(cmdline[0].d,"from-domain") == 0)
    tofrom(cmdline,cmds,CMD_MCP_FROM_DOMAIN,CMD_MCP_FROM_DOMAIN_RESP,TRUE);
  else if (strcmp(cmdline[0].d,"help") == 0)
    help();
  else if (strcmp(cmdline[0].d,"?") == 0)
    help();

  MemFree(cmdline);
}

/**************************************************************/

static void show(String *cmdline,size_t cmds)
{
  ddt(cmdline != NULL);
  ddt(cmds    >  0);
  
  if (strcmp(cmdline[1].d,"stats") == 0)
    show_stats();
  else if (strcmp(cmdline[1].d,"config") == 0)
    show_config();
  else if (strcmp(cmdline[1].d,"iplist") == 0)
    show_report(CMD_MCP_SHOW_IPLIST,CMD_MCP_SHOW_IPLIST_RESP);
  else if (strcmp(cmdline[1].d,"tuples") == 0)
  {
    if (cmds == 2)
      show_report(CMD_MCP_SHOW_TUPLE_ALL,CMD_MCP_SHOW_TUPLE_ALL_RESP);
    else if (strcmp(cmdline[2].d,"all") == 0)
      show_report(CMD_MCP_SHOW_TUPLE_ALL,CMD_MCP_SHOW_TUPLE_ALL_RESP);
    else if (strcmp(cmdline[2].d,"whitelist") == 0)
      show_report(CMD_MCP_SHOW_WHITELIST,CMD_MCP_SHOW_WHITELIST_RESP);
  }
  else if (strcmp(cmdline[1].d,"to") == 0)
    show_report(CMD_MCP_SHOW_TO,CMD_MCP_SHOW_TO_RESP);
  else if (strcmp(cmdline[1].d,"to-domain") == 0)
    show_report(CMD_MCP_SHOW_TO_DOMAIN,CMD_MCP_SHOW_TO_DOMAIN_RESP);
  else if (strcmp(cmdline[1].d,"from") == 0)
    show_report(CMD_MCP_SHOW_FROM,CMD_MCP_SHOW_FROM_RESP);
  else if (strcmp(cmdline[1].d,"from-domain") == 0)
    show_report(CMD_MCP_SHOW_FROM_DOMAIN,CMD_MCP_SHOW_FROM_DOMAIN_RESP);
}

/***************************************************************/

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
  	"exit | quit\t\t\tquit program\n"
  	"iplist accept a.b.c.d/M\t\taccept IP addresses\n"
  	"iplist reject a.b.c.d/M\t\treject IP addresses\n"
  	"iplist graylist a.b.c.d/M\tgreylist IP addresses\n"
  	"iplist file <file> [relaydelay]\tbulk load IP addresses\n"
	"to accept <addr>\t\taccept recpient address\n"
	"to reject <addr>\t\treject recipient address\n"
	"to graylist <addr>\t\tgreylist recipient address\n"
	"to-domain accept <domain>\taccept recipient domain\n"
	"to-domain reject <domain>\treject recipient domain\n"
	"to-domain graylist <domain>\tgreylist recipient domain\n"
	"from accept <addr>\t\taccept sender address\n"
	"from reject <addr>\t\treject sender address\n"
	"from graylist <addr>\t\tgreylist sender address\n"
	"from-domain accept <domain>\taccept sender domain\n"
	"from-domain reject <domain>\treject sender domain\n"
	"from-domain graylist <domain>\tgreylist sender domain\n"
  	"show config\t\t\tshow configuration settings\n"
  	"show stats\t\t\tshow current statistics\n"
  	"show tuples [all]\t\tshow all tuples\n"
  	"show tuples whitelist\t\tshow tuples on whitelist\n"
  	"show iplist\t\t\tshow accepted/rejected IP addresses\n"
	"show to\t\t\t\tshow recipient addresses\n"
	"show to-domains\t\t\tshow recipient domains\n"
	"show from\t\t\tshow sender addresses\n"
	"show from-domain\t\tshow sender domains\n"
  	"help | ?\t\t\tthis text\n"
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

static void iplist(String *cmdline,size_t cmds)
{
  struct glmcp_request_iplist  ipr;
  byte                         data[1500];
  size_t                       octet;
  char                        *p;
  int                          rc;
  
  ddt(cmdline != NULL);
  ddt(cmds    >  0);

  if (cmds < 3)		/* valid command has at least 3 paramters */
    return;

  memset(ipr.data,0,16);
  
  ipr.version = htons(VERSION);
  ipr.MTA     = htons(MTA_MCP);
  ipr.type    = htons(CMD_MCP_IPLIST);
  ipr.ipsize  = htons(4);
  ipr.mask    = htons(32);	/* default mask */
  
  if (strcmp(cmdline[1].d,"file") == 0)
  {
    if (cmds == 3)
      iplist_file(cmdline[2].d);
    else if ((cmds == 4) && (strcmp(cmdline[3].d,"relaydelay") == 0))
      iplist_file_relaydelay(cmdline[2].d);
    return;
  }
  
  ipr.cmd = ci_map_int(up_string(cmdline[1].d),c_ipcmds,C_IPCMDS);

  if (ipr.cmd == (unet16)-1)
  {
    LineS(StdoutStream,"bad command\n");
    return;
  }
  
  ipr.cmd = htons(ipr.cmd);
  
  for (octet = 0 , p = cmdline[2].d ; octet < 4 ; octet++)
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

static void iplist_file(char *fname)
{
  Stream                       in;
  struct glmcp_request_iplist  ipr;
  byte                         data[1500];
  int                          linecnt;

  ddt(fname != NULL);
  
  in = FileStreamRead(fname);
  if (in == NULL)
  {
    LineSFormat(StdoutStream,"$","could not open %a",fname);
    return;
  }
  
  linecnt = 0;
  
  while(!StreamEOF(in))
  {
    char   *line;
    char   *p;
    char   *r;
    int     rc;
    size_t  octet;
    
    linecnt++;
    line = LineSRead(in);
    if (empty_string(line)) continue;
    p = trim_space(line);
    if (*p == '#') continue;
    
    memset(ipr.data,0,sizeof(ipr.data));
    ipr.version = htons(VERSION);
    ipr.MTA     = htons(MTA_MCP);
    ipr.type    = htons(CMD_MCP_IPLIST);
    ipr.ipsize  = htons(4);
    ipr.mask    = htons(32);	/* default mask */
    
    for (octet = 0 ; octet < 4 ; octet++)
    {
      ipr.data[octet] = strtoul(p,&p,10);
      if (*p != '.') break;
      p++;
    }
    
    if (*p == '/')
      ipr.mask = htons(strtoul(++p,&p,10));
    
    for ( ; *p && isspace(*p) ; p++)
      ;

    for ( r = p ; *r && isalpha(*r) ; r++)
      ;

    *r++ = '\0';
    
    up_string(p);
    ipr.cmd = ci_map_int(p,c_ipcmds,C_IPCMDS);

    MemFree(line);
    
    if (ipr.cmd == (unet16)-1)
    {
      LineSFormat(StdoutStream,"$ i","%a(%b): syntax error",fname,linecnt);
      return;
    }
  
    ipr.cmd = htons(ipr.cmd);

    rc = send_request(&ipr,sizeof(ipr),data,sizeof(data),CMD_MCP_IPLIST_RESP);
    if (rc != ERR_OKAY)
      LineS(StdoutStream,"bad response\n");
  }

  StreamFree(in);
}

/*****************************************************************/

static void iplist_file_relaydelay(char *fname)
{
  struct glmcp_request_iplist  ipr;
  Stream                       in;
  byte                         data[1500];
  size_t                       octet;
  char                        *p;
  int                          rc;
  int                          linecnt;
  
  in = FileStreamRead(fname);
  if (in == NULL)
  {
    LineSFormat(StdoutStream,"$","could not open %a",fname);
    return;
  }
  
  linecnt = 0;
  
  while(!StreamEOF(in))
  {
    char *line;
    char *p;
    
    linecnt++;
    line = LineSRead(in);
    if (empty_string(line)) continue;
    p = trim_space(line);
    if (*p == '#') continue;
    
    memset(ipr.data,0,sizeof(ipr.data));
    ipr.version = htons(VERSION);
    ipr.MTA     = htons(MTA_MCP);
    ipr.type    = htons(CMD_MCP_IPLIST);
    ipr.ipsize  = htons(4);
    
    for (octet = 0 ; octet < 4 ; octet++)
    {
      ipr.data[octet] = strtoul(p,&p,10);
      if (*p != '.') break;
      p++;
    }
    
    ipr.mask = htons((octet + 1) * 8);
    ipr.cmd  = htons(IPCMD_ACCEPT);
    
    rc = send_request(&ipr,sizeof(ipr),data,sizeof(data),CMD_MCP_IPLIST_RESP);
    if (rc != ERR_OKAY)
      LineS(StdoutStream,"bad response\n");
  }
  StreamFree(in);
}

/*************************************************************************/

static void tofrom(String *cmdline,size_t cmds,int cmd,int resp,int domain)
{
  byte                         packet[1500];
  byte                         data  [1500];
  struct glmcp_request_tofrom *ptfr;
  size_t                       addrsize;
  int                          rc;
  
  ddt(cmdline != NULL);
  ddt(cmds    >  0);
  
  ptfr     = (struct glmcp_request_tofrom *)packet;
  addrsize = min(strlen(cmdline[2].d),200);  
  up_string(cmdline[1].d);

  if (domain)
  {
    char *at = strchr(cmdline[2].d,'@');
    if (at)
    {
      LineSFormat(StdoutStream,"$","this '%a' is not a domain\n",cmdline[2].d);
      return;
    }
  }
  
  ptfr->cmd = ci_map_int(up_string(cmdline[1].d),c_ipcmds,C_IPCMDS);
  if (ptfr->cmd == (unet16)-1)
    return;
   
  ptfr->version = htons(VERSION);
  ptfr->MTA     = htons(MTA_MCP);
  ptfr->type    = htons(cmd);
  ptfr->cmd     = htons(ptfr->cmd);
  memcpy(ptfr->data,cmdline[2].d,addrsize);
  ptfr->data[addrsize] = '\0';
  
  rc = send_request(
  		ptfr,
  		sizeof(struct glmcp_request_tofrom) + addrsize,
  		data,
  		sizeof(data),
  		resp
  	);
  if (rc != ERR_OKAY)
    LineS(StdoutStream,"bad response\n");
}

/**********************************************************************/

