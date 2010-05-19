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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <cgilib6/util.h>

#include "../../common/src/greylist.h"
#include "../../common/src/globals.h"
#include "../../common/src/util.h"
#include "../../conf.h"

#include "tuple.h"
#include "signals.h"
#include "iplist.h"
#include "emaildomain.h"

enum
{
  OPT_LIST_WHITE = OPT_USER,
  OPT_LIST_GREY,
  OPT_MAX_TUPLES,
  OPT_TIME_CLEANUP,
  OPT_TIME_SAVESTATE,
  OPT_TIMEOUT_EMBARGO,
  OPT_TIMEOUT_GREY,
  OPT_TIMEOUT_WHITE,
  OPT_FILE_IPLIST,
  OPT_REPORT_FORMAT,
  OPT_TIME_FORMAT,  
  OPT_FOREGROUND,
  OPT_STDERR,
  OPT_OLDCOUNTS,
  OPT_MAX
};

/********************************************************************/

static void	 dump_defaults	(void);
static void	 my_exit	(void);

/*********************************************************************/

extern char **environ;

char          *c_pidfile         = SERVER_PIDFILE;
char          *c_host            = SERVER_BINDHOST;
int            c_port            = SERVER_PORT;
int            c_log_facility    = SERVER_LOG_FACILITY;
int            c_log_level       = SERVER_LOG_LEVEL;
char	      *c_log_id          = SERVER_LOG_ID;
char	      *c_secret		 = SECRET;
size_t         c_secretsize	 = SECRETSIZE;
int            cf_debug          = 0;
void         (*cv_report)(int,const char *, ...) = report_syslog;

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
double	       c_timeout_embargo = SERVER_TIMEOUT_EMBARGO;
double         c_timeout_grey    = SERVER_TIMEOUT_GREYLIST;
double	       c_timeout_white   = SERVER_TIMEOUT_WHITELIST;
time_t         c_starttime       = 0;
int            cf_foreground     = 0;
int            cf_oldcounts      = 0;

	/*---------------------------------------------------*/
	
char                 g_argv0[FILENAME_MAX];
char               **g_argv;

size_t               g_poolnum;
struct tuple        *g_pool;		/* actual space */
Tuple               *g_tuplespace;	/* used for sorting records */

size_t               g_requests;
size_t               g_req_cu;
size_t               g_req_cucurrent;
size_t               g_req_cumax;
size_t               g_cleanup_count;
size_t               g_greylisted;
size_t               g_whitelisted;
size_t               g_whitelist_expired;
size_t               g_greylist_expired;
size_t		     g_tuples_read;
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

size_t		     g_smaxfrom;
size_t	             g_sfrom;
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

int		     g_defto         = IFT_GREYLIST;
int                  g_deftodomain   = IFT_GREYLIST;
int		     g_deffrom       = IFT_GREYLIST;
int                  g_deffromdomain = IFT_GREYLIST;
size_t               g_toc;
size_t               g_todomainc;
size_t               g_fromc;
size_t               g_fromdomainc;

/*******************************************************************/

volatile int m_debug = 1;

static const struct option mc_options[] =
{
  { "whitelist"      	, required_argument	, NULL	, OPT_LIST_WHITE	} ,	/* gone */
  { "graylist"		, required_argument	, NULL	, OPT_LIST_GREY 	} ,	/* gone */
  { "greylist"		, required_argument	, NULL	, OPT_LIST_GREY		} ,	/* gone */
  { "host"		, required_argument	, NULL	, OPT_HOST		} ,
  { "port"		, required_argument	, NULL	, OPT_PORT		} ,
  { "max-tuples"	, required_argument	, NULL	, OPT_MAX_TUPLES	} ,	/* gone */
  { "time-cleanup" 	, required_argument	, NULL	, OPT_TIME_CLEANUP	} ,	/* gone */
  { "time-checkpoint"	, required_argument	, NULL	, OPT_TIME_SAVESTATE	} ,	/* gone */
  { "timeout-embargo"	, required_argument	, NULL	, OPT_TIMEOUT_EMBARGO	} ,	/* gone */
  { "timeout-grey"	, required_argument	, NULL	, OPT_TIMEOUT_GREY	} ,	/* gone */
  { "timeout-grey"	, required_argument	, NULL	, OPT_TIMEOUT_GREY	} ,	/* gone */
  { "timeout-white"	, required_argument	, NULL	, OPT_TIMEOUT_WHITE	} ,	/* gone */
  { "iplist"		, required_argument	, NULL	, OPT_FILE_IPLIST	} ,	/* gone */
  { "time-format"     	, required_argument 	, NULL	, OPT_TIME_FORMAT    	} ,	/* gone */
  { "report-format"     , required_argument 	, NULL	, OPT_REPORT_FORMAT	} ,	/* gone --if foreground use stderr */
  { "log-facility"   	, required_argument 	, NULL	, OPT_LOG_FACILITY	} ,	/* gone */
  { "log-level"      	, required_argument 	, NULL	, OPT_LOG_LEVEL		} ,	/* gone */
  { "log-id"      	, required_argument 	, NULL	, OPT_LOG_ID        	} ,	/* gone */
  { "secret"		, required_argument	, NULL	, OPT_SECRET		} ,	/* gone */
  { "debug"          	, no_argument       	, NULL	, OPT_DEBUG		} ,
  { "foreground"     	, no_argument       	, NULL	, OPT_FOREGROUND   	} ,
  { "old-counts"	, no_argument		, NULL	, OPT_OLDCOUNTS		} ,	/* gone */
  { "stderr"	     	, no_argument       	, NULL	, OPT_STDERR       	} ,	/* gone */
  { "version"		, no_argument		, NULL	, OPT_VERSION		} ,
  { "help"	     	, no_argument       	, NULL	, OPT_HELP         	} ,
  { NULL	     	, 0		 	, NULL	, 0 			}
};

/********************************************************************/

int (GlobalsInit)(void)
{
  c_starttime = g_time_savestate = time(NULL);  
  openlog(c_log_id,0,c_log_facility);

  g_pool       = malloc(c_poolmax * sizeof(struct tuple));
  g_tuplespace = malloc(c_poolmax * sizeof(Tuple));

  memset(g_pool, 0,c_poolmax * sizeof(struct tuple));

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

  set_signal(SIGSEGV ,sighandler_critical);
  set_signal(SIGBUS  ,sighandler_critical);
  set_signal(SIGFPE  ,sighandler_critical);
  set_signal(SIGILL  ,sighandler_critical);
  set_signal(SIGXCPU ,sighandler_critical);
  set_signal(SIGXFSZ ,sighandler_critical);

  write_pidfile(c_pidfile);
  alarm(c_time_cleanup);	/* start the countdown */
  return(ERR_OKAY);
}

/*********************************************************************/

int (GlobalsDeinit)(void)
{
  free(g_pool);
  free(g_tuplespace);	/* XXX - free g_tree */
  closelog();
  return(ERR_OKAY);
}

/*********************************************************************/

static void dump_defaults(void)
{
  char *togrey;
  char *towhite;
  char *toclean;

  togrey  = report_delta(c_timeout_grey);
  towhite = report_delta(c_timeout_white);
  toclean = report_delta(c_time_cleanup);
  
  fprintf(
        stderr,
        "\t--whitelist <file>\t\t(%s)\n"
        "\t--greylist  <file>\t\t(%s)\n"
        "\t--host <hostname>\t\t(%s)\n"
        "\t--port <num>\t\t\t(%d)\n"
        "\t--max-tuples <num>\t\t(%lu)\n"
        "\t--time-cleanup <num>\t\t(%s)\n"
        "\t--timeout-grey <timespec>\t(%s)\n"
        "\t--timeout-white <timespec>\t(%s)\n"
        "\t--iplist <file>\t\t\t(%s)\n"
        "\t--time-format <strftime>\t(%s)\n"
        "\t--report format syslog | stderr\t(%s)\n"
        "\t--log-facility <facility>\t(%s)\n"
        "\t--log-level <level>\t\t(%s)\n"
        "\t--log-sysid <string>\t\t(%s)\n"
        "\t--debug\t\t\t\t(%s)\n"
        "\t--foreground\t\t\t(%s)\n"
        "\t--old-counts\t\t\t(%s)\n"
        "\t--stderr\t\t\t(%s)\n"
        "\t--version\t\t\t(" PROG_VERSION ")\n"
        "\t--help\n"
        "\n",
        c_whitefile,
        c_greyfile,
        c_host,
        c_port,
        (unsigned long)c_poolmax,
        toclean,
        togrey,
        towhite,
        c_iplistfile,
        c_timeformat,
        (cv_report == report_stderr) ? "stderr" : "syslog",
        ci_map_chars(c_log_facility,c_facilities,C_FACILITIES),
        ci_map_chars(c_log_level,   c_levels,    C_LEVELS),
        c_log_id,
        (cf_debug)                   ? "true" : "false" ,
        (cf_foreground)              ? "true" : "false",
        (cf_oldcounts)               ? "true" : "false",
        (cv_report == report_stderr) ? "true" : "false"        
  );

  free(toclean);
  free(towhite);
  free(togrey);
}

/********************************************************************/ 

void parse_cmdline(int argc,char *argv[])
{
  char *tmp;
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
      case OPT_LIST_WHITE:
           c_whitefile = optarg;
           break;
      case OPT_LIST_GREY:
           c_greyfile = optarg;
           break;
      case OPT_HOST:
           c_host = optarg;
           break;
      case OPT_PORT:
           c_port = strtoul(optarg,NULL,10);
           break;
      case OPT_MAX_TUPLES:
           c_poolmax = strtoul(optarg,NULL,10);
           break;
      case OPT_TIME_CLEANUP:
           c_time_cleanup = (int)read_dtime(optarg,c_time_cleanup);
           break;
      case OPT_TIME_SAVESTATE:
           c_time_savestate = read_dtime(optarg,c_time_savestate);
           break;
      case OPT_TIMEOUT_EMBARGO:
           c_timeout_embargo = read_dtime(optarg,c_timeout_embargo);
           break;
      case OPT_TIMEOUT_GREY:
           c_timeout_grey = read_dtime(optarg,c_timeout_grey);
           break;
      case OPT_TIMEOUT_WHITE:
           c_timeout_white = read_dtime(optarg,c_timeout_white);
           break;
      case OPT_FILE_IPLIST:
           c_iplistfile = optarg;
           break;
      case OPT_TIME_FORMAT:
           c_timeformat = optarg;
           break;
      case OPT_REPORT_FORMAT:
           if (strcmp(optarg,"syslog") == 0)
             cv_report = report_syslog;
           else if (strcmp(optarg,"stdout") == 0)
             cv_report = report_stderr;
           else
           {
             fprintf(stderr,"format %s not supported\n",optarg);
             exit(EXIT_FAILURE);
           }
           break;
      case OPT_SECRET:
           c_secret     = optarg;
           c_secretsize = strlen(c_secret);
           break;
      case OPT_DEBUG:
           cf_debug     = 1;
           c_log_level  = LOG_DEBUG;
           fprintf(stderr,"using '--log-level debug'\n");
           break;
      case OPT_LOG_FACILITY:
           {
             tmp            = up_string(strdup(optarg));
             c_log_facility = ci_map_int(tmp,c_facilities,C_FACILITIES);
             free(tmp);
           }
           break;
      case OPT_LOG_LEVEL:
           {
             tmp         = up_string(strdup(optarg));
             c_log_level = ci_map_int(tmp,c_levels,C_LEVELS);
             free(tmp);
           }
           break;
      case OPT_LOG_ID:
           c_log_id = optarg;
           break;
      case OPT_FOREGROUND:
           cf_foreground = 1;
           break;
      case OPT_OLDCOUNTS:
           cf_oldcounts = 1;
           break;
      case OPT_STDERR:
           cv_report = report_stderr;
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
    (*cv_report)(LOG_EMERG,"daemon_init(): fork() = %s",strerror(errno));
    exit(EXIT_FAILURE);
  }
  else if (pid != 0)	/* parent goes bye bye */
    exit(EXIT_SUCCESS);

  setsid();
  set_signal(SIGHUP,SIG_IGN);
  
  pid = fork();
  if (pid == (pid_t)-1)
  {
    (*cv_report)(LOG_EMERG,"daemon_init(): fork(2) = %s",strerror(errno));
    exit(EXIT_FAILURE);
  }
  else if (pid != 0)
    _exit(EXIT_SUCCESS);
  
  chdir("/");
  umask(022);
  
  fh = open("/dev/null",O_RDWR);
  if (fh == -1)
  {
    (*cv_report)(LOG_EMERG,"daemon_init(): open(/dev/null) = %s",strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  assert(fh > 2);
  dup2(fh,STDIN_FILENO);
  dup2(fh,STDOUT_FILENO);
  dup2(fh,STDERR_FILENO);	/* we always close this when going into daemon mode */
  
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

