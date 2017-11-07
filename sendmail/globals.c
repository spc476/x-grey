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

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include <syslog.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cgilib6/util.h>

#include "../common/greylist.h"
#include "../common/globals.h"
#include "../common/util.h"
#include "../conf.h"

enum
{
  OPT_TIMEOUT = OPT_USER,
  OPT_CHANNEL,
  OPT_FOREGROUND,
  OPT_MAXSTACK
};

/*****************************************************************/

static void              parse_cmdline  (int,char *[]);
static void              dump_defaults  (void);
static void              daemon_init    (void);
static void              my_exit        (void);

/****************************************************************/

char                *c_pidfile       = SENDMAIL_PIDFILE;
char                *c_host          = SENDMAIL_HOST;
int                  c_port          = 0;
char                *c_timeformat    = "%c";
char                *c_rhost         = SERVER_HOST;
int                  c_rport         = SERVER_PORT;
int                  c_timeout       = SENDMAIL_TIMEOUT;
struct sockaddr_in   c_raddr;
socklen_t            c_raddrsize     = sizeof(struct sockaddr_in);
int                  c_log_facility  = SENDMAIL_LOG_FACILITY;
int                  c_log_level     = SENDMAIL_LOG_LEVEL;
char                *c_log_id        = SENDMAIL_LOG_ID;
char                *c_secret        = SECRET;
size_t               c_secretsize    = SECRETSIZE;
char                *c_filterchannel = SENDMAIL_FILTERCHANNEL;
bool                 cf_foreground   = false;
bool                 cf_debug        = false;
size_t               c_maxstack      = (64uL * 1024uL);

int                  gl_sock;

  /*----------------------------------------------------*/
  
static struct option const mc_options[] =
{
  { "host"              , required_argument     , NULL  , OPT_HOST              } ,
  { "port"              , required_argument     , NULL  , OPT_PORT              } ,
  { "server"            , required_argument     , NULL  , OPT_RHOST             } ,
  { "server-port"       , required_argument     , NULL  , OPT_RPORT             } ,
  { "timeout"           , required_argument     , NULL  , OPT_TIMEOUT           } ,
  { "foreground"        , no_argument           , NULL  , OPT_FOREGROUND        } ,
  { "max-stack"         , required_argument     , NULL  , OPT_MAXSTACK          } ,
  { "log-facility"      , required_argument     , NULL  , OPT_LOG_FACILITY      } ,
  { "log-level"         , required_argument     , NULL  , OPT_LOG_LEVEL         } ,
  { "log-id"            , required_argument     , NULL  , OPT_LOG_ID            } ,
  { "channel"           , required_argument     , NULL  , OPT_CHANNEL           } ,
  { "secret"            , required_argument     , NULL  , OPT_SECRET            } ,
  { "debug"             , no_argument           , NULL  , OPT_DEBUG             } ,
  { "version"           , no_argument           , NULL  , OPT_VERSION           } ,
  { "help"              , no_argument           , NULL  , OPT_HELP              } ,
  { NULL                , 0                     , NULL  , 0                     }
};

/*********************************************************************/

int (GlobalsInit)(int argc,char *argv[])
{
  struct hostent *remote;
  struct rlimit   limit;
  int             rc;
  
  assert(argc >  0);
  assert(argv != NULL);
  
  parse_cmdline(argc,argv);
  
  /*----------------------------------------------------------
  ; on some systems, the default stack is set as high as 10M,
  ; and libmilter creates a lot of threads, which can lead to
  ; out-of-memory problems (and I thought I was using a lot
  ; of memory in the greylist daemon!).  This will set the
  ; default stack size to something a bit more reasonable.
  ;----------------------------------------------------------*/
  
  rc = getrlimit(RLIMIT_STACK,&limit);
  
  if (rc != 0)
    syslog(LOG_WARNING,"getrlimit(RLIMIT_STACK) = %s, can't modify stack size",strerror(errno));
  else
  {
    syslog(
        LOG_DEBUG,
        "stack current: %lu max: %lu",
        (unsigned long)limit.rlim_cur,
        (unsigned long)limit.rlim_max
    );
    
    if (limit.rlim_max > c_maxstack)
    {
      limit.rlim_cur = c_maxstack;
      limit.rlim_max = c_maxstack;
      
      rc = setrlimit(RLIMIT_STACK,&limit);
      
      if (rc != 0)
        syslog(LOG_WARNING,"setrlimit(RLIMIT_STACK) = %s, can't modify stack size",strerror(errno));
      else
      {
        extern char **environ;
        
        /*--------------------------------------------
        ; the pthreads library apparently gets control
        ; before main() is called, and thus the limits
        ; we set aren't enforced.  So we restart the
        ; program with a restricted stacksize.  Cool
        ; all the crap we have to do in order to run
        ; correctly, huh?
        ;
        ; We dont' check to see if exec() works,
        ; because what's the point?
        ;-------------------------------------------*/
        
        syslog(LOG_DEBUG,"running with a smaller stack");
        execve(argv[0],argv,environ);
      }
    }
  }
  
  remote = gethostbyname(c_rhost);
  if (remote == NULL)
  {
    syslog(LOG_ERR,"gethostbyname(%s) = %s",c_rhost,strerror(errno));
    return(EXIT_FAILURE);
  }
  
  memcpy(&c_raddr.sin_addr.s_addr,remote->h_addr,remote->h_length);
  c_raddr.sin_family = AF_INET;
  c_raddr.sin_port   = htons(c_rport);
  
  openlog(c_log_id,0,c_log_facility);
  
  if (!cf_foreground)
    daemon_init();
    
  unlink(&c_filterchannel[5]);  /* making sure it doesn't exist */
  atexit(my_exit);
  write_pidfile(c_pidfile);
  return(EXIT_SUCCESS);
}

/******************************************************************/

int (GlobalsDeinit)(void)
{
  return(EXIT_SUCCESS);
}

/*****************************************************************/

static void parse_cmdline(int argc,char *argv[])
{
  int option = 0;
  
  assert(argc >  0);
  assert(argv != NULL);
  
  while(1)
  {
    switch(getopt_long_only(argc,argv,"",mc_options,&option))
    {
      case EOF:
           return;
      case OPT_NONE:
           break;
      case OPT_HOST:
           c_host = optarg;
           break;
      case OPT_PORT:
           c_port = strtoul(optarg,NULL,10);
           break;
      case OPT_RHOST:
           c_rhost = optarg;
           break;
      case OPT_RPORT:
           c_rport = strtoul(optarg,NULL,10);
           break;
      case OPT_TIMEOUT:
           c_timeout = read_dtime(optarg,c_timeout);
           break;
      case OPT_CHANNEL:
           c_filterchannel = optarg;
           break;
      case OPT_MAXSTACK:
           c_maxstack = strtoul(optarg,NULL,10);        /* fix later */
           break;
      case OPT_LOG_FACILITY:
           {
             char *tmp = up_string(strdup(optarg));
             c_log_facility = ci_map_int(tmp,c_facilities,C_FACILITIES);
             free(tmp);
           }
           break;
      case OPT_LOG_LEVEL:
           {
             char *tmp = up_string(strdup(optarg));
             c_log_level = ci_map_int(tmp,c_levels,C_LEVELS);
             free(tmp);
           }
           break;
      case OPT_LOG_ID:
           c_log_id = optarg;
           break;
      case OPT_FOREGROUND:
           cf_foreground = true;
           break;
      case OPT_SECRET:
           c_secret     = optarg;
           c_secretsize = strlen(c_secret);
           break;
      case OPT_DEBUG:
           cf_debug = true;
           break;
      case OPT_VERSION:
           fputs("Version: " PROG_VERSION "\n",stdout);
           exit(EXIT_FAILURE);
      case OPT_HELP:
      default:
           fprintf(stderr,"usage: %s [options]\n",argv[0]);
           dump_defaults();
           exit(EXIT_FAILURE);
    }
  }
}

/********************************************************************/

static void dump_defaults(void)
{
  char *tout;
  
  tout = report_delta(c_timeout);
  
  fprintf(
    stderr,
    "\t--host <host>             (%s)\n"
    "\t--port <port>             (%d)\n"
    "\t--server <host>           (%s)\n"
    "\t--server-port <port>      (%d)\n"
    "\t--foreground              (%s)\n"
    "\t--max-stack <size>        (%lu)\n"
    "\t--secret <string>         (%s)\n"
    "\t--timeout <time>          (%s)\n"
    "\t--log-facility <facility> (%s)\n"
    "\t--log-level <level>       (%s)\n"
    "\t--log-id <id>             (%s)\n"
    "\t--channel <channel>       (%s)\n"
    "\t--debug                   (%s)\n"
    "\t--version                 (" PROG_VERSION ")\n"
    "\t--help\n",
    c_host,
    c_port,
    c_rhost,
    c_rport,
    (cf_foreground) ? "true" : "false",
    (unsigned long)c_maxstack,
    c_secret,
    tout,
    ci_map_chars(c_log_facility,c_facilities,C_FACILITIES),
    ci_map_chars(c_log_level,   c_levels,    C_LEVELS),
    c_log_id,
    c_filterchannel,
    (cf_debug) ? "true" : "false"
  );
  
  free(tout);
}

/********************************************************************/

static void daemon_init(void)
{
  pid_t pid;
  int   fh;
  
  pid = fork();
  if (pid == (pid_t)-1)
  {
    syslog(LOG_EMERG,"daemon_init(): fork() = %s",strerror(errno));
    exit(EXIT_FAILURE);
  }
  else if (pid != 0)    /* parent goes bye bye */
    exit(EXIT_SUCCESS);
    
  setsid();
  set_signal(SIGHUP,SIG_IGN);
  
  pid = fork();
  if (pid == (pid_t)-1)
  {
    syslog(LOG_EMERG,"daemon_init(): fork(2) = %s",strerror(errno));
    _exit(EXIT_FAILURE);
  }
  else if (pid != 0)
    _exit(EXIT_SUCCESS);
    
  chdir("/");
  umask(022);
  
  fh = open("/dev/null",O_RDWR);
  if (fh == -1)
  {
    syslog(LOG_EMERG,"daemon_init(): open(/dev/null) = %s",strerror(errno));
    _exit(EXIT_FAILURE);
  }
  
  assert(fh > 2);
  dup2(fh,STDIN_FILENO);
  dup2(fh,STDOUT_FILENO);
  dup2(fh,STDERR_FILENO);
  
  close(fh);
}

/********************************************************************/

static void my_exit(void)
{
  unlink(&c_filterchannel[5]);
  closelog();
  fflush(stderr);
  fflush(stdout);
}

/********************************************************************/

