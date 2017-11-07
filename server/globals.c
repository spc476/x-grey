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

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

#include <libgen.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "../common/greylist.h"
#include "../common/globals.h"
#include "../common/util.h"
#include "../conf.h"

#include "tuple.h"
#include "signals.h"
#include "iplist.h"
#include "emaildomain.h"

enum
{
  OPT_FOREGROUND = OPT_USER,
  OPT_NOMONITOR,
  OPT_MAX
};

/********************************************************************/

static void      dump_defaults  (void);
static void      my_exit        (void);

/*********************************************************************/

extern char **environ;

char          *c_pidfile         = SERVER_PIDFILE;
char          *c_host            = SERVER_BINDHOST;
int            c_port            = SERVER_PORT;
int            c_log_facility    = SERVER_LOG_FACILITY;
int            c_log_level       = SERVER_LOG_LEVEL;
char          *c_log_id          = SERVER_LOG_ID;
char          *c_secret          = SECRET;
size_t         c_secretsize      = SECRETSIZE;
bool           cf_debug          = false;

char          *c_conffile        = SERVER_STATEDIR "/config.txt";
char          *c_whitefile       = SERVER_STATEDIR "/whitelist.txt";
char          *c_greyfile        = SERVER_STATEDIR "/greyfile.txt";
char          *c_dumpfile        = SERVER_STATEDIR "/dump.txt";
char          *c_iplistfile      = SERVER_STATEDIR "/iplist.txt";
char          *c_tofile          = SERVER_STATEDIR "/to.txt";
char          *c_todfile         = SERVER_STATEDIR "/to-domain.txt";
char          *c_fromfile        = SERVER_STATEDIR "/from.txt";
char          *c_fromdfile       = SERVER_STATEDIR "/from-domain.txt";
char          *c_timeformat      = "%c";
size_t         c_poolmax         = SERVER_MAX_TUPLES;
unsigned int   c_time_cleanup    = SERVER_CLEANUP;
double         c_time_savestate  = SERVER_SAVESTATE;
double         c_timeout_embargo = SERVER_TIMEOUT_EMBARGO;
double         c_timeout_grey    = SERVER_TIMEOUT_GREYLIST;
double         c_timeout_white   = SERVER_TIMEOUT_WHITELIST;
time_t         c_starttime       = 0;
bool           cf_foreground     = false;
bool           cf_oldcounts      = false;
bool           cf_nomonitor      = false;

        /*---------------------------------------------------*/
        
char                 g_argv0[FILENAME_MAX];
char               **g_argv;

size_t               g_poolnum;
struct tuple        *g_pool;            /* actual space */
Tuple               *g_tuplespace;      /* used for sorting records */

size_t               g_requests;
size_t               g_req_cu;
size_t               g_req_cucurrent;
size_t               g_req_cumax;
size_t               g_cleanup_count;
size_t               g_greylisted;
size_t               g_whitelisted;
size_t               g_whitelist_expired;
size_t               g_greylist_expired;
size_t               g_tuples_read;
size_t               g_tuples_read_cu;
size_t               g_tuples_read_cucurrent;
size_t               g_tuples_read_cumax;
size_t               g_tuples_write;
size_t               g_tuples_write_cu;
size_t               g_tuples_write_cucurrent;
size_t               g_tuples_write_cumax;
size_t               g_tuples_low;
size_t               g_tuples_high;

struct ipnode       *g_tree;
size_t               g_ipcnt = 1;
size_t               g_ip_cmdcnt[3];

size_t               g_smaxfrom;
size_t               g_sfrom;
struct emaildomain  *g_from;
size_t               g_from_cmdcnt[3];

size_t               g_smaxfromd;
size_t               g_sfromd;
struct emaildomain  *g_fromd;
size_t               g_fromd_cmdcnt[3];

size_t               g_smaxto;
size_t               g_sto;
struct emaildomain  *g_to;
size_t               g_to_cmdcnt[3];

size_t               g_smaxtod;
size_t               g_stod;
struct emaildomain  *g_tod;
size_t               g_tod_cmdcnt[3];

time_t               g_time_savestate;

int                  g_defto         = IFT_GREYLIST;
int                  g_deftodomain   = IFT_GREYLIST;
int                  g_deffrom       = IFT_GREYLIST;
int                  g_deffromdomain = IFT_GREYLIST;
size_t               g_toc;
size_t               g_todomainc;
size_t               g_fromc;
size_t               g_fromdomainc;

/*******************************************************************/

static struct option const mc_options[] =
{
  { "host"              , required_argument     , NULL  , OPT_HOST              } ,
  { "port"              , required_argument     , NULL  , OPT_PORT              } ,
  { "debug"             , no_argument           , NULL  , OPT_DEBUG             } ,
  { "foreground"        , no_argument           , NULL  , OPT_FOREGROUND        } ,
  { "nomonitor"         , no_argument           , NULL  , OPT_NOMONITOR         } ,
  { "version"           , no_argument           , NULL  , OPT_VERSION           } ,
  { "help"              , no_argument           , NULL  , OPT_HELP              } ,
  { NULL                , 0                     , NULL  , 0                     }
};

/********************************************************************/

int (GlobalsInit)(void)
{
  c_starttime = g_time_savestate = time(NULL);
  openlog(c_log_id,0,c_log_facility);
  
  g_pool       = calloc(c_poolmax,sizeof(struct tuple));
  g_tuplespace = calloc(c_poolmax,sizeof(Tuple));
  
  madvise(g_pool,      c_poolmax * sizeof(struct tuple),MADV_RANDOM);
  madvise(g_tuplespace,c_poolmax * sizeof(Tuple),       MADV_RANDOM);
  
  for (size_t i = 0 ; i < c_poolmax ; i++)
    g_tuplespace[i] = &g_pool[i];
    
  g_tree         = malloc(sizeof(struct ipnode));
  g_tree->parent = NULL;
  g_tree->zero   = NULL;
  g_tree->one    = NULL;
  g_tree->count  = 0;
  g_tree->match  = IFT_GREYLIST;
  
  if (cf_debug)
    dump_defaults();
    
  iplist_read(c_iplistfile);
  whitelist_load();
  to_read();
  tod_read();
  from_read();
  fromd_read();
  
  atexit(my_exit);
  
  /*-------------------------------------------------------------
  ; yes, I don't bother checking the return code for thes calls.
  ; Why?  Because the underlying system call, sigaction(), only
  ; returns type 1 errors (see http://boston.conman.org/2009/12/01.2
  ; for more information) which indicate bad code, not a real
  ; error condition.
  ;---------------------------------------------------------------*/
  
  set_signal(SIGCHLD, sighandler_sigs);
  set_signal(SIGINT,  sighandler_sigs);
  set_signal(SIGQUIT, sighandler_sigs);
  set_signal(SIGTERM, sighandler_sigs);
  set_signal(SIGUSR1, sighandler_sigs);
  set_signal(SIGUSR2, sighandler_sigs);
  set_signal(SIGALRM, sighandler_sigs);
  set_signal(SIGHUP,  sighandler_sigs);
  
  if (!cf_nomonitor)
    write_pidfile(c_pidfile);
  alarm(c_time_cleanup);        /* start the countdown */
  return(ERR_OKAY);
}

/*********************************************************************/

int (GlobalsDeinit)(void)
{
  free(g_pool);
  free(g_tuplespace);   /* XXX - free g_tree */
  closelog();
  return(ERR_OKAY);
}

/*********************************************************************/

static void dump_defaults(void)
{
  fprintf(
        stderr,
        "\t--host <hostname>\t\t(%s)\n"
        "\t--port <num>\t\t\t(%d)\n"
        "\t--debug\t\t\t\t(%s)\n"
        "\t--foreground\t\t\t(%s)\n"
        "\t--nomonitor\t\t\t(%s)\n"
        "\t--version\t\t\t(" PROG_VERSION ")\n"
        "\t--help\n"
        "\n",
        c_host,
        c_port,
        (cf_debug)                   ? "true" : "false",
        (cf_foreground)              ? "true" : "false",
        (cf_nomonitor)               ? "true" : "false"
   );
}

/********************************************************************/

void parse_cmdline(int argc,char *argv[])
{
  int   option = 0;
  
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
      case OPT_DEBUG:
           cf_debug     = true;
           c_log_level  = LOG_DEBUG;
           break;
      case OPT_FOREGROUND:
           cf_foreground = true;
           break;
      case OPT_NOMONITOR:
           cf_nomonitor = true;
           break;
      case OPT_VERSION:
           fputs("Version: " PROG_VERSION "\n",stderr);
           exit(EXIT_FAILURE);
      case OPT_HELP:
      default:
           fprintf(stderr,"usage: %s [options]\n",argv[0]);
           dump_defaults();
           exit(EXIT_FAILURE);
    }
  }
}

/*******************************************************************/

void daemon_init(void)
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
    exit(EXIT_FAILURE);
  }
  else if (pid != 0)
    _exit(EXIT_SUCCESS);
    
  chdir("/");
  umask(022);
  
  fh = open("/dev/null",O_RDWR);
  if (fh == -1)
  {
    syslog(LOG_EMERG,"daemon_init(): open(/dev/null) = %s",strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  assert(fh > 2);
  dup2(fh,STDIN_FILENO);
  dup2(fh,STDOUT_FILENO);
  dup2(fh,STDERR_FILENO);       /* we always close this when going into daemon mode */
  
  close(fh);
}

/**********************************************************************/

static void my_exit(void)
{
  /*----------------------------------------------
  ; this routine only exists as a handy place to
  ; put a breakpoint to catch those unexpected
  ; calls to exit().
  ;------------------------------------------------*/
  
  unlink(c_pidfile);
  closelog();
  
  if (cf_foreground)
  {
    fflush(stdout);
    fflush(stderr);
  }
}

/*************************************************************************/

