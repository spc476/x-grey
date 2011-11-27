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


#ifndef __GNUC__
#  define __attribute(x)
#endif

#define _POSIX_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <netinet/in.h>

#include "../../common/src/greylist.h"
#include "../../common/src/globals.h"
#include "../../common/src/util.h"
#include "../../common/src/crc32.h"

#include "globals.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

/**************************************************************/

	/*------------------------------------------------------------------
	; this is defined external because if I define _GNU_SOURCE to get it
	; defined, I then break calls to sendto(), recvfrom() and
	; connection(), since they're defined using some wierd GNU alien
	; technology that I have no ken of.
	;------------------------------------------------------------------*/
	
extern ssize_t getline	(char **,size_t *,FILE *);

static void	cmdline			(void);
static void	process_cmdline		(String *,size_t);
static void	show			(String *,size_t);
static void	iplist			(String *,size_t);
static void	tofrom			(String *,size_t,int,int,int);
static void	tuple			(String *,size_t);

static void	show_stats		(void);
static void	show_config		(void);
static void	show_report		(int,int);
static void	show_warranty		(void);
static void	show_redistribute	(void);

static void	iplist_file		(char *);
static void	iplist_file_relaydelay	(char *);
static void	iplist_file_bogonspace	(char *);

static int	send_request		(void *,size_t,void *,size_t,int);
static void	handler_sigalrm		(int);
static void	help			(void);
static void	fcopy			(FILE *,FILE *);

/*************************************************************/

extern char **environ;

static volatile sig_atomic_t   mf_sigalrm;
static int                     m_sock;

/************************************************************/

int main(int argc,char *argv[])
{
  int cmd;
  
  cmd = GlobalsInit(argc,argv);
  
  if (cmd < 0)
  {
    fputs("can't init program\n",stderr);
    fflush(stderr);
    return(EXIT_FAILURE);
  }

  set_signal(SIGALRM,handler_sigalrm);
  m_sock = create_socket(c_host,c_port,SOCK_DGRAM);

  if (m_sock == -1)
    exit(EXIT_FAILURE);
    
  if (cmd < argc)
  {
    size_t  cmds  = argc - cmd;
    String *what  = malloc(cmds * sizeof(String));
    size_t  i;
    
    for (i = 0 ; cmd < argc ; i++,cmd++)
    {
      what[i].d = argv[cmd];
      what[i].s = strlen(argv[cmd]);
    }
    
    cv_pager = pager_batch;
    process_cmdline(what,cmds);  
    free(what);
  }
  else
  {
    printf(
	"\n"
    	"%s " PROG_VERSION " Copyright (C) " COPYRIGHT_YEAR " Sean Conner\n"
	"\n"
    	"This program comes with ABSOLUTELY NO WARRANTY; for details type 'show w'.\n"
    	"This is free software, and you are welcome to redistribute it\n"
    	"under certain conditions; type 'show c' for details.\n"
    	"\n",
    	argv[0]
      );    	
    cmdline();
  }
  
  return(EXIT_SUCCESS);
}

/***************************************************************/

static void cmdline(void)
{
  String *cmdline;
  char    prompt[BUFSIZ];
  char   *cmd;
  size_t  cmds;

  sprintf(prompt,"%s>",c_log_id);
  using_history();
  
  while(true)
  {
    cmd = readline(prompt);
    
    if (cmd == NULL) break;
    if (empty_string(cmd)) continue;
    
    trim_space(cmd);
    
    if (strcmp(cmd,"exit") == 0)
      break;
      
    if (strcmp(cmd,"quit") == 0)
      break;

    add_history(cmd);

    cmdline = split(&cmds,cmd);    
    process_cmdline(cmdline,cmds);
    free(cmdline);
    
    fflush(stdout);
    free(cmd);
  }
  
  free(cmd);
  fputc('\n',stdout);
}

/****************************************************************/

static void process_cmdline(String *cmdline,size_t cmds)
{
  assert(cmdline != NULL);
  assert(cmds    >  0);

  if (cmds == 0) return;
  
  if (strcmp(cmdline[0].d,"show") == 0)
    show(cmdline,cmds);
  else if (strcmp(cmdline[0].d,"iplist") == 0)
    iplist(cmdline,cmds);
  else if (strcmp(cmdline[0].d,"to") == 0)
    tofrom(cmdline,cmds,CMD_MCP_TO,CMD_MCP_TO_RESP,0);
  else if (strcmp(cmdline[0].d,"to-domain") == 0)
    tofrom(cmdline,cmds,CMD_MCP_TO_DOMAIN,CMD_MCP_TO_DOMAIN_RESP,1);
  else if (strcmp(cmdline[0].d,"from") == 0)
    tofrom(cmdline,cmds,CMD_MCP_FROM,CMD_MCP_FROM_RESP,0);
  else if (strcmp(cmdline[0].d,"from-domain") == 0)
    tofrom(cmdline,cmds,CMD_MCP_FROM_DOMAIN,CMD_MCP_FROM_DOMAIN_RESP,1);
  else if (strcmp(cmdline[0].d,"tuple") == 0)
    tuple(cmdline,cmds);
  else if (strcmp(cmdline[0].d,"help") == 0)
    help();
  else if (strcmp(cmdline[0].d,"?") == 0)
    help();
}

/**************************************************************/

static void show(String *cmdline,size_t cmds)
{
  assert(cmdline != NULL);
  assert(cmds    >  0);
  
  if (cmds < 2)
    return;

  if (strcmp(cmdline[1].d,"w") == 0)
    show_warranty();
  else if (strcmp(cmdline[1].d,"c") == 0)
    show_redistribute();
  else if (strcmp(cmdline[1].d,"stats") == 0)
    show_stats();
  else if (strcmp(cmdline[1].d,"config") == 0)
    show_config();
  else if (strcmp(cmdline[1].d,"iplist") == 0)
    show_report(CMD_MCP_SHOW_IPLIST,CMD_MCP_SHOW_IPLIST_RESP);
  else if (strcmp(cmdline[1].d,"tuples") == 0)
  {
    if (cmds == 2)
      show_report(CMD_MCP_SHOW_TUPLE,CMD_MCP_SHOW_TUPLE_RESP);
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
  struct glmcp_response *gr   = result;
  struct glmcp_request  *greq = req;
  socklen_t              sipsize;
  ssize_t                rrc;
  CRC32                  crc;
  
  assert(req    != NULL);
  assert(result != NULL);
  assert(size   >  0);
  
  crc       = crc32(INIT_CRC32,&greq->version,reqsize - sizeof(CRC32));
  crc       = crc32(crc,c_secret,c_secretsize);
  greq->crc = htonl(crc);

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
      (*cv_report)(LOG_ERR,"recvfrom() = %s",strerror(errno));
    return ERR_ERR;
  }
  
  crc = crc32(INIT_CRC32,&gr->version,rrc - sizeof(CRC32));
  crc = crc32(crc,c_secret,c_secretsize);
  
  if (crc != ntohl(gr->crc))
  {
    (*cv_report)(LOG_ERR,"bad packet");
    return ERR_ERR;
  }

  if (ntohs(gr->version) > VERSION)
  {
    (*cv_report)(LOG_ERR,"received response from wrong version");
    return ERR_ERR;
  }
  
  if (ntohs(gr->MTA) != MTA_MCP)
  {
    (*cv_report)(LOG_ERR,"we got a reponse for something else");
    return ERR_ERR;
  }
  
  if (ntohs(gr->type) != expected_response)
  {
    (*cv_report)(LOG_ERR,"received wrong response");
    return ERR_ERR;
  }
  
  return ERR_OKAY;
}

/*********************************************************************/

static void handler_sigalrm(int sig __attribute((unused)))
{
  mf_sigalrm = 1;
}

/********************************************************************/

static void show_stats(void)
{
  struct glmcp_request              request;
  struct glmcp_response_show_stats *gss;
  union greylist_all_packets        data;
  int                               rc;
  char                             *t;

  gss             = &data.mcp_show_stats;;
  request.crc     = htons(0);
  request.version = htons(VERSION);
  request.MTA     = htons(MTA_MCP);
  request.type    = htons(CMD_MCP_SHOW_STATS);

  rc = send_request(&request,sizeof(request),&data,sizeof(data),CMD_MCP_SHOW_STATS_RESP);
  
  if (rc != ERR_OKAY)
  {
    fputs("bad response\n",stdout);
    return;
  }

  t = report_time(ntohl(gss->starttime),ntohl(gss->nowtime));

  printf(
  	"%s\n"
  	"Requests:               %10lu\n"
  	"Requests-Cu:            %10lu\n"
  	"Requests-Cu-Max:        %10lu\n"
  	"Requests-Cu-Ave:        %10lu\n"
	"\n",
  	t,
  	(unsigned long)ntohl(gss->requests),
  	(unsigned long)ntohl(gss->requests_cu),
  	(unsigned long)ntohl(gss->requests_cu_max),
  	(unsigned long)ntohl(gss->requests_cu_ave)
  );
  
  printf(
  	"Tuples-Low:             %10lu\n"
  	"Tuples-High:            %10lu\n"
  	"Tuples-Read:            %10lu\n"
  	"Tuples-Read-Cu:         %10lu\n"
  	"Tuples-Read-Cu-Max:     %10lu\n"
  	"Tuples-Read-Cu-Ave:     %10lu\n"
  	"Tuples-Write:           %10lu\n"
  	"Tuples-Write-Cu:        %10lu\n"
  	"Tuples-Write-Cu-Max:    %10lu\n"
  	"Tuples-Write-Cu-Ave:    %10lu\n"
  	"\n",
  	(unsigned long)ntohl(gss->tuples_low),
  	(unsigned long)ntohl(gss->tuples_high),
  	(unsigned long)ntohl(gss->tuples_read),
  	(unsigned long)htonl(gss->tuples_read_cu),
  	(unsigned long)htonl(gss->tuples_read_cu_max),
  	(unsigned long)htonl(gss->tuples_read_cu_ave),
  	(unsigned long)htonl(gss->tuples_write),
  	(unsigned long)htonl(gss->tuples_write_cu),
  	(unsigned long)htonl(gss->tuples_write_cu_max),
  	(unsigned long)htonl(gss->tuples_write_cu_ave)
  );
  
  printf(
  	"IP-Entries:             %10lu\n"
  	"IP-Greylisted:          %10lu\n"
  	"IP-Accepted:            %10lu\n"
  	"IP-Rejected:            %10lu\n"
	"\n",
  	(unsigned long)ntohl(gss->ips),
  	(unsigned long)ntohl(gss->ip_greylist),
  	(unsigned long)ntohl(gss->ip_accept),
  	(unsigned long)ntohl(gss->ip_reject)
  );
  
  printf(
  	"From-Entries:           %10lu\n"
  	"From-Greylisted:        %10lu\n"
  	"From-Accepted:          %10lu\n"
  	"From-Rejected:          %10lu\n"
	"\n",
  	(unsigned long)ntohl(gss->from),
  	(unsigned long)ntohl(gss->from_greylist),
  	(unsigned long)ntohl(gss->from_accept),
  	(unsigned long)ntohl(gss->from_reject)
  );

  printf(  
  	"From-Domain-Entries:    %10lu\n"
  	"From-Domain-Greylisted: %10lu\n"
  	"From-Domain-Accepted:   %10lu\n"
  	"From-Domain-Rejected:   %10lu\n"
	"\n",
  	(unsigned long)ntohl(gss->fromd),
  	(unsigned long)ntohl(gss->fromd_greylist),
  	(unsigned long)ntohl(gss->fromd_accept),
  	(unsigned long)ntohl(gss->fromd_reject)
  );
  
  printf(
  	"To-Entries:             %10lu\n"
  	"To-Greylisted:          %10lu\n"
  	"To-Accepted:            %10lu\n"
  	"To-Rejected:            %10lu\n"
	"\n",
  	(unsigned long)ntohl(gss->to),
  	(unsigned long)ntohl(gss->to_greylist),
  	(unsigned long)ntohl(gss->to_accept),
  	(unsigned long)ntohl(gss->to_reject)
  );
  
  printf(
  	"To-Domain-Entries:      %10lu\n"
  	"To-Domain-Greylisted:   %10lu\n"
  	"To-Domain-Accepted:     %10lu\n"
  	"To-Domain-Rejected:     %10lu\n"
	"\n",
  	(unsigned long)ntohl(gss->tod),
  	(unsigned long)ntohl(gss->tod_greylist),
  	(unsigned long)ntohl(gss->tod_accept),
  	(unsigned long)ntohl(gss->tod_reject)
  );
  
  printf(
  	"Tuples:                 %10lu\n"
  	"Greylisted:             %10lu\n"
  	"Greylisted-Expired:     %10lu\n"
  	"Whitelisted:            %10lu\n"
  	"Whitelisted-Expired:    %10lu\n",
  	(unsigned long)ntohl(gss->tuples),
  	(unsigned long)ntohl(gss->greylisted),
  	(unsigned long)ntohl(gss->greylist_expired),
  	(unsigned long)ntohl(gss->whitelisted),
  	(unsigned long)ntohl(gss->whitelist_expired)
  );
  	
  free(t);
}

/******************************************************************/

static void show_config(void)
{
  struct glmcp_request               request;
  struct glmcp_response_show_config *gsc;
  union greylist_all_packets         data;
  int                                rc;
  char                              *cleanup;
  char                              *embargo;
  char                              *greylist;
  char                              *whitelist;
  
  gsc             = &data.mcp_show_config;
  request.crc     = htons(0);
  request.version = htons(VERSION);
  request.MTA     = htons(MTA_MCP);
  request.type    = htons(CMD_MCP_SHOW_CONFIG);
  
  rc = send_request(&request,sizeof(request),&data,sizeof(data),CMD_MCP_SHOW_CONFIG_RESP);
  
  if (rc != ERR_OKAY)
  {
    fputs("bad response\n",stdout);
    return;
  }
  
  cleanup   = report_delta(ntohl(gsc->timeout_cleanup));
  embargo   = report_delta(ntohl(gsc->timeout_embargo));
  greylist  = report_delta(ntohl(gsc->timeout_grey));
  whitelist = report_delta(ntohl(gsc->timeout_white));
  
  printf(
  	"Max-Tuples:        %10lu\n"
  	"Timeout-Cleanup:   %s\n"
  	"Timeout-Embargo:   %s\n"
  	"Timeout-Greylist:  %s\n"
  	"Timeout-Whitelist: %s\n",
  	(unsigned long)ntohl(gsc->max_tuples),
  	cleanup,
  	embargo,
  	greylist,
  	whitelist
    );

  free(whitelist);
  free(greylist);
  free(embargo);
  free(cleanup);
}

/*************************************************************/

static void show_report(int req,int resp)
{
  struct glmcp_request        request;
  struct glmcp_response      *gr;
  int                         conn;
  union greylist_all_packets  data;
  int                         rc;

  gr              = &data.mcp_res;  
  request.crc     = htons(0);
  request.version = htons(VERSION);
  request.MTA     = htons(MTA_MCP);
  request.type    = htons(req);
  
  rc = send_request(&request,sizeof(request),&data,sizeof(data),resp);
  
  if (rc != ERR_OKAY)
  {
    fputs("bad reponse\n",stdout);
    return;
  }
  
  conn = create_socket(c_host,c_port,SOCK_STREAM);
  if (conn == -1)
  {
    fputs("cannot connect to gld\n",stdout);
    return;
  }
  
  rc = connect(conn,(struct sockaddr *)&c_raddr,c_raddrsize);
  if (rc == -1)
  {
    fputs("cannot connect to gld\n",stdout);
    return;
  }

  (*cv_pager)(conn);
  close(conn); 
}

/************************************************************/

static void help(void)
{
  int fh;
  
  fh = open(c_commands,O_RDONLY);
  if (fh == -1) return;
  (*cv_pager)(fh);
  close(fh);
}

/**************************************************************/

void pager_batch(int fh)
{
  FILE *in;
  
  assert(fh > -1);

  in = fdopen(fh,"r");
  if (in)
  {
    fcopy(stdout,in);
    fclose(in);
  }
}

/*************************************************************/

void pager_interactive(int fh)
{
  pid_t  child;
  pid_t  pid;
  char  *argv[2];
  int    status;
  int    rc;
  
  assert(fh >= 0);
  
  pid = fork();
  if (pid == -1)	/* error */
    (*cv_report)(LOG_ERR,"fork() = %s",strerror(errno));
  else if (pid > 0)	/* parent */
  {
    child = waitpid(pid,&status,0);
    if (child == (pid_t)-1)
      (*cv_report)(LOG_ERR,"waitpid() = %s",strerror(errno));
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
  union greylist_all_packets   data;
  size_t                       octet;
  char                        *p;
  int                          rc;
  
  assert(cmdline != NULL);
  assert(cmds    >  0);

  if (cmds < 3)		/* valid command has at least 3 paramters */
    return;

  memset(ipr.data,0,16);

  ipr.crc     = htons(0);  
  ipr.version = htons(VERSION);
  ipr.MTA     = htons(MTA_MCP);
  ipr.type    = htons(CMD_MCP_IPLIST);
  ipr.ipsize  = htons(4);
  ipr.mask    = htons(32);	/* default mask */
  
  if (strcmp(cmdline[1].d,"file") == 0)
  {
    if (cmds == 3)
      iplist_file(cmdline[2].d);
    else if (cmds == 4)
    {
      if (strcmp(cmdline[3].d,"relaydelay") == 0)
        iplist_file_relaydelay(cmdline[2].d);
      else if (strcmp(cmdline[3].d,"bogonspace") == 0)
        iplist_file_bogonspace(cmdline[2].d);
    }
    return;
  }
  
  ipr.cmd = ci_map_int(up_string(cmdline[1].d),c_ift,C_IFT);

  if (ipr.cmd == (unet16)-1)
  {
    fputs("bad command\n",stdout);
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
    fputs("bad response\n",stdout);
}

/******************************************************************/

static void iplist_file(char *fname)
{
  FILE                        *in;
  struct glmcp_request_iplist  ipr;
  union greylist_all_packets   data;
  int                          linecnt;
  char                        *line;
  size_t                       linesize;

  assert(fname != NULL);
  
  in = fopen(fname,"r");
  if (in == NULL)
  {
    printf("%s: %s\n",fname,strerror(errno));
    return;
  }
  
  line     = NULL;
  linesize = 0;
  linecnt  = 0;
  
  while(getline(&line,&linesize,in) > 0)
  {
    char   *p;
    char   *r;
    int     rc;
    size_t  octet;
    
    linecnt++;
    if (empty_string(line)) continue;
    p = trim_space(line);
    if (*p == '#') continue;
    
    memset(ipr.data,0,sizeof(ipr.data));
    
    ipr.crc     = htons(0);
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
      ipr.mask = htons(strtoul(p + 1,&p,10));
    
    for ( ; *p && isspace(*p) ; p++)
      ;

    for ( r = p ; *r && isalpha(*r) ; r++)
      ;

    *r++ = '\0';
    
    up_string(p);
    ipr.cmd = ci_map_int(p,c_ift,C_IFT);

    if (ipr.cmd == (unet16)-1)
    {
      printf("%s(%d): syntax error",fname,linecnt);
      return;
    }
  
    ipr.cmd = htons(ipr.cmd);

    rc = send_request(&ipr,sizeof(ipr),&data,sizeof(data),CMD_MCP_IPLIST_RESP);
    if (rc != ERR_OKAY)
      fputs("bad response\n",stdout);
  }

  free(line);
  fclose(in);
}

/*****************************************************************/

static void iplist_file_relaydelay(char *fname)
{
  FILE                        *in;
  struct glmcp_request_iplist  ipr;
  union greylist_all_packets   data;
  size_t                       octet;
  int                          rc;
  int                          linecnt;
  char                        *line;
  size_t                       linesize;
  
  in = fopen(fname,"r");
  if (in == NULL)
  {
    printf("%s: %s\n",fname,strerror(errno));
    return;
  }
  
  line     = NULL;
  linesize = 0;
  linecnt  = 0;
  
  while(getline(&line,&linesize,in) > 0)
  {
    char *p;
    
    linecnt++;
    if (empty_string(line)) continue;
    p = trim_space(line);
    if (*p == '#') continue;
    
    memset(ipr.data,0,sizeof(ipr.data));
    
    ipr.crc     = htons(0);
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
    ipr.cmd  = htons(IFT_ACCEPT);
    
    rc = send_request(&ipr,sizeof(ipr),&data,sizeof(data),CMD_MCP_IPLIST_RESP);
    if (rc != ERR_OKAY)
      fputs("bad response\n",stdout);
  }
  free(line);
  fclose(in);
}

/*************************************************************************/

static void iplist_file_bogonspace(char *fname)
{
  FILE                        *in;
  struct glmcp_request_iplist  ipr;
  union greylist_all_packets   data;
  size_t                       octet;
  int                          rc;
  int                          linecnt;
  char                        *line;
  size_t                       linesize;
  
  in = fopen(fname,"r");
  if (in == NULL)
  {
    printf("%s: %s\n",fname,strerror(errno));
    return;
  }
  
  line     = NULL;
  linesize = 0;
  linecnt  = 0;
  
  while(getline(&line,&linesize,in) > 0)
  {
    char *p;
    
    linecnt++;
    if (empty_string(line)) continue;
    p = trim_space(line);
    if (*p == '#') continue;
    
    memset(ipr.data,0,sizeof(ipr.data));
    
    ipr.crc     = htons(0);
    ipr.version = htons(VERSION);
    ipr.MTA     = htons(MTA_MCP);
    ipr.type    = htons(CMD_MCP_IPLIST);
    ipr.ipsize  = htons(4);
    ipr.mask    = htons(32);
    ipr.cmd     = htons(IFT_REJECT);
    
    for (octet = 0 ; octet < 4 ; octet++)
    {
      ipr.data[octet] = strtoul(p,&p,10);
      if (*p != '.') break;
      p++;
    }
    
    if (*p == '/')
      ipr.mask = htons(strtoul(++p,NULL,10));
    
    rc = send_request(&ipr,sizeof(ipr),&data,sizeof(data),CMD_MCP_IPLIST_RESP);
    if (rc != ERR_OKAY)
      fputs("bad response\n",stdout);
  }
  free(line);
  fclose(in);
}

/*************************************************************************/

static void tofrom(String *cmdline,size_t cmds,int cmd,int resp,int domain)
{
  union greylist_all_packets   packet;
  union greylist_all_packets   data;
  struct glmcp_request_tofrom *ptfr;
  size_t                       addrsize;
  int                          rc;
  
  assert(cmdline != NULL);
  assert(cmds    >  0);
  
  if (cmds != 3)
    return;

  ptfr     = &packet.mcp_tofrom;
  addrsize = min(strlen(cmdline[2].d),200);  
  up_string(cmdline[1].d);

  if (domain)
  {
    char *at = strchr(cmdline[2].d,'@');
    if (at)
    {
      printf("this '%s' is not a domain\n",cmdline[2].d);
      return;
    }
  }
  
  ptfr->cmd = ci_map_int(up_string(cmdline[1].d),c_ift,C_IFT);
  if (ptfr->cmd == (unet16)-1)
    return;
   
  ptfr->crc     = htons(0);
  ptfr->version = htons(VERSION);
  ptfr->MTA     = htons(MTA_MCP);
  ptfr->type    = htons(cmd);
  ptfr->cmd     = htons(ptfr->cmd);
  memcpy(ptfr->data,cmdline[2].d,addrsize);
  ptfr->data[addrsize] = '\0';
  
  rc = send_request(
  		ptfr,
  		sizeof(struct glmcp_request_tofrom) + addrsize,
  		&data,
  		sizeof(data),
  		resp
  	);

  if (rc != ERR_OKAY)
    fputs("bad response\n",stdout);
}

/**********************************************************************/

static void show_warranty(void)
{
  printf(
  	"\n"
  	"THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY\n"
  	"APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT\n"
  	"HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY\n"
  	"OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,\n"
  	"THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n"
  	"PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM\n"
  	"IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF\n"
  	"ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n"
  	"\n"
  );
}

/**************************************************************************/

static void show_redistribute(void)
{
  int fh;
  
  fh = open(c_license,O_RDONLY);
  if (fh == -1) return;
  (*cv_pager)(fh);
  close(fh);
}

/***********************************************************************/

static const struct chars_int m_tuple_cmd[4] =
{
  { "GRAYLIST"	, CMD_GREYLIST		} ,
  { "GREYLIST"	, CMD_GREYLIST		} ,
  { "WHITELIST" , CMD_WHITELIST		} ,
  { "REMOVE"	, CMD_TUPLE_REMOVE	}
};

static const struct chars_int m_reason[8] =
{
  { "NONE"		, REASON_NONE		} ,
  { "IP"		, REASON_IP		} ,
  { "To"		, REASON_TO		} ,
  { "To-Domain"		, REASON_TO_DOMAIN	} ,
  { "From"		, REASON_FROM		} ,
  { "From-Domain"	, REASON_FROM_DOMAIN	} ,
  { "Greylist"		, REASON_GREYLIST	} ,
  { "Whitelist"		, REASON_WHITELIST	} 
};
  
static void tuple(String *cmdline,size_t cmds)
{
  union greylist_all_packets  outpacket;
  union greylist_all_packets  inpacket;
  struct greylist_request    *glq;
  struct greylist_response   *glr;
  int                         cmd;
  int                         mask;
  size_t                      packetsize;
  size_t                      sfrom;
  size_t                      sto;
  byte                       *p;
  int                         rc;
  
  assert(cmdline != NULL);
  assert(cmds    >  0);

  if (cmds != 5)
    return;

  cmd = ci_map_int((const char *)up_string(cmdline[1].d),m_tuple_cmd,4);  
  if (cmd == -1)
    return;
      
  sfrom = min(strlen(cmdline[3].d),200);
  sto   = min(strlen(cmdline[4].d),200);
  glq   = &outpacket.req;
  p     = glq->data;
  
  glq->crc      = htons(0);
  glq->version  = htons(VERSION);
  glq->MTA      = htons(MTA_MCP);
  glq->type     = htons(cmd);
  glq->ipsize   = htons(4);
  glq->fromsize = htons(sfrom);
  glq->tosize   = htons(sto);
  
  rc = parse_ip(p,&mask,cmdline[2].d); p += 4;
  if (rc == ERR_ERR)
    return;

  if (mask != 32)
    return;

  if (strcmp(cmdline[3].d,"-") != 0)
  {
    memcpy(p,cmdline[3].d,sfrom); 
    p += sfrom;
  }
  else
    glq->fromsize = htons(0);
    
  if (strcmp(cmdline[4].d,"-") != 0)
  {
    memcpy(p,cmdline[4].d,sto);
    p += sto;
  }
  else
    glq->tosize = htons(0);

  packetsize = (size_t)(p - outpacket.data);
  
  rc = send_request(&outpacket,packetsize,&inpacket,sizeof(inpacket),cmd + 1);    
  if (rc != ERR_OKAY)
    fputs("bad response\n",stdout);
    
  glr           = (struct greylist_response *)&inpacket;
  glr->response = ntohs(glr->response);
  glr->why      = ntohs(glr->why);
  
  if (!((glr->why == REASON_GREYLIST) || (glr->why == REASON_WHITELIST)))
  {
    printf(
        "tuple not added because of %s %s rule\n",
    	ci_map_chars(glr->why,m_reason,8),
    	ci_map_chars(glr->response,c_ift,C_IFT)
    );
  } 
}

/***********************************************************************/

static void fcopy(FILE *fpout,FILE *fpin)
{
  char   buffer[BUFSIZ];
  size_t size;
  
  assert(fpout != NULL);
  assert(fpin  != NULL);
  
  while((size = fread(buffer,1,BUFSIZ,fpin)) > 0)
    fwrite(buffer,1,size,fpout);
}

/*************************************************************************/
