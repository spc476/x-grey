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

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
/*#include <arpa/inet.h>*/

#include <cgilib/memory.h>
#include <cgilib/errors.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>
#include <cgilib/types.h>
#include <cgilib/ddt.h>

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

static void		 dump_defaults	(void);
static void		 parse_cmdline	(int,char *[]);
static void		 daemon_init	(void);
static void		 my_exit	(void);

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
void         (*cv_report)(int,char *,char *, ...) = report_syslog;

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
  { "whitelist"      	, required_argument	, NULL	, OPT_LIST_WHITE	},
  { "graylist"		, required_argument	, NULL	, OPT_LIST_GREY 	} ,
  { "greylist"		, required_argument	, NULL	, OPT_LIST_GREY		} ,
  { "host"		, required_argument	, NULL	, OPT_HOST		} ,
  { "port"		, required_argument	, NULL	, OPT_PORT		} ,
  { "max-tuples"	, required_argument	, NULL	, OPT_MAX_TUPLES	} ,
  { "time-cleanup" 	, required_argument	, NULL	, OPT_TIME_CLEANUP	} ,
  { "time-checkpoint"	, required_argument	, NULL	, OPT_TIME_SAVESTATE	} ,
  { "timeout-embargo"	, required_argument	, NULL	, OPT_TIMEOUT_EMBARGO	} ,
  { "timeout-grey"	, required_argument	, NULL	, OPT_TIMEOUT_GREY	} ,
  { "timeout-grey"	, required_argument	, NULL	, OPT_TIMEOUT_GREY	} ,
  { "timeout-white"	, required_argument	, NULL	, OPT_TIMEOUT_WHITE	} ,
  { "iplist"		, required_argument	, NULL	, OPT_FILE_IPLIST	} ,
  { "time-format"     	, required_argument 	, NULL	, OPT_TIME_FORMAT    	} ,
  { "report-format"     , required_argument 	, NULL	, OPT_REPORT_FORMAT	} ,
  { "log-facility"   	, required_argument 	, NULL	, OPT_LOG_FACILITY	} ,
  { "log-level"      	, required_argument 	, NULL	, OPT_LOG_LEVEL		} ,
  { "log-id"      	, required_argument 	, NULL	, OPT_LOG_ID        	} ,
  { "secret"		, required_argument	, NULL	, OPT_SECRET		} ,
  { "debug"          	, no_argument       	, NULL	, OPT_DEBUG		} ,
  { "foreground"     	, no_argument       	, NULL	, OPT_FOREGROUND   	} ,
  { "old-counts"	, no_argument		, NULL	, OPT_OLDCOUNTS		} ,
  { "stderr"	     	, no_argument       	, NULL	, OPT_STDERR       	} ,
  { "version"		, no_argument		, NULL	, OPT_VERSION		} ,
  { "help"	     	, no_argument       	, NULL	, OPT_HELP         	} ,
  { NULL	     	, 0		 	, NULL	, 0 			}
};

/********************************************************************/

int (GlobalsInit)(int argc,char *argv[])
{
  ddt(argc >  0);
  ddt(argv != NULL);

  /*--------------------------------------------------------------
  ; we save the arguments for possible later use.  What possible
  ; later use?  When one of the worker threads unexpectantly dies,
  ; we will inform the other remaining threads to finish up, then
  ; they're killed and this program will then reexec itself to start
  ; up.  This is what they do in Erlang to remain up and running.
  ;--------------------------------------------------------------*/
  
  g_argv      = argv;
  c_starttime = g_time_savestate = time(NULL);
  
  parse_cmdline(argc,argv);
  openlog(c_log_id,0,c_log_facility);

  g_pool       = MemAlloc(c_poolmax * sizeof(struct tuple));
  g_tuplespace = MemAlloc(c_poolmax * sizeof(Tuple));

  memset(g_pool,      0,c_poolmax * sizeof(struct tuple));
  memset(g_tuplespace,0,c_poolmax * sizeof(Tuple));

  g_tree         = MemAlloc(sizeof(struct ipnode));
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
  
  if (!cf_foreground)
    daemon_init();

  atexit(my_exit);	/* used to be above daemon_init(), race condition */

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
  MemFree(g_pool);
  MemFree(g_tuplespace);
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
  
  LineSFormat(
  	StderrStream,
  	"$ $ $ i i L $ $ $ $ $ $ $ $ $ $ $ $ $",
  	"\t--whitelist <file>\t\t(%a)\n"
  	"\t--greylist  <file>\t\t(%b)\n"
  	"\t--host <hostname>\t\t(%c)\n"
  	"\t--port <num>\t\t\t(%d)\n"
  	"\t--max-tuples <num>\t\t(%f)\n"
  	"\t--time-cleanup <num>\t\t(%q)\n"
  	"\t--timeout-grey <timespec>\t(%g)\n"
  	"\t--timeout-white <timespec>\t(%h)\n"
  	"\t--iplist <file>\t\t\t(%r)\n"
  	"\t--time-format <strftime>\t(%i)\n"
  	"\t--report-format syslog | stderr\t(%j)\n"
  	"\t--log-facility <facility>\t(%k)\n"
  	"\t--log-level <level>\t\t(%l)\n"
  	"\t--log-sysid <string>\t\t(%m)\n"
  	"\t--debug\t\t\t\t(%n)\n"
  	"\t--foreground\t\t\t(%o)\n"
  	"\t--old-counts\t\t\t(%s)\n"
  	"\t--stderr\t\t\t(%p)\n"
  	"\t--version\t\t\t(" PROG_VERSION ")\n"
  	"\t--help\n"
  	"\t\t* not implemented\n",
  	c_whitefile,
  	c_greyfile,
  	c_host,
  	c_port,
  	0,
  	(unsigned long)c_poolmax,
  	togrey,
  	towhite,
  	c_timeformat,
  	(cv_report == report_stderr) ? "stderr" : "syslog",
	ci_map_chars(c_log_facility,c_facilities,C_FACILITIES),
  	ci_map_chars(c_log_level,   c_levels,    C_LEVELS),
  	c_log_id,
  	(cf_debug) ? "true" : "false" ,
  	(cf_foreground) ? "true" : "false",
  	(cv_report == report_stderr) ? "true" : "false",
  	toclean,
  	c_iplistfile,
  	(cf_oldcounts) ? "true" : "false"
  );

  MemFree(toclean);
  MemFree(towhite);
  MemFree(togrey);
}

/********************************************************************/ 

static void parse_cmdline(int argc,char *argv[])
{
  char *tmp;
  int   option = 0;
  
  ddt(argc >  0);
  ddt(argv != NULL);
  
  while(1)
  {
    switch(getopt_long_only(argc,argv,"",mc_options,&option))
    {
      case EOF:
           return;
      case OPT_NONE:
           break;
      case OPT_LIST_WHITE:
           c_whitefile = dup_string(optarg);
           break;
      case OPT_LIST_GREY:
           c_greyfile = dup_string(optarg);
           break;
      case OPT_HOST:
           c_host = dup_string(optarg);
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
           c_iplistfile = dup_string(optarg);
           break;
      case OPT_TIME_FORMAT:
           c_timeformat = dup_string(optarg);
           break;
      case OPT_REPORT_FORMAT:
           {
             tmp = up_string(dup_string(optarg));
             
             if (strcmp(tmp,"SYSLOG") == 0)
               cv_report = report_syslog;
             else if (strcmp(tmp,"STDOUT") == 0)
               cv_report = report_stderr;
             else
             {
               LineSFormat(StderrStream,"$","format %a not supported\n",optarg);
               exit(EXIT_FAILURE);
             }
             MemFree(tmp);
           }
           break;
      case OPT_SECRET:
           c_secret     = dup_string(optarg);
           c_secretsize = strlen(c_secret);
           break;
      case OPT_DEBUG:
           cf_debug = 1;
           c_log_level  = LOG_DEBUG;
           LineSFormat(StderrStream,"","using '--sys-facility debug'\n");
           break;
      case OPT_LOG_FACILITY:
           {
             tmp = up_string(dup_string(optarg));
             c_log_facility = ci_map_int(tmp,c_facilities,C_FACILITIES);
             MemFree(tmp);
           }
           break;
      case OPT_LOG_LEVEL:
           {
             tmp = up_string(dup_string(optarg));
             c_log_level   = ci_map_int(tmp,c_levels,C_LEVELS);
             MemFree(tmp);
           }
           break;
      case OPT_LOG_ID:
           c_log_id = dup_string(optarg);
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
           LineS(StdoutStream,"Version: " PROG_VERSION "\n");
	   exit(EXIT_FAILURE);
      case OPT_HELP:
      default:
           LineSFormat(StderrStream,"$","usage: %a [options]\n",argv[0]);
           dump_defaults();
           exit(EXIT_FAILURE);
    }
  }
} 

/*******************************************************************/

static void daemon_init(void)
{
  pid_t pid;

  pid = fork();
  if (pid == (pid_t)-1)
  {
    (*cv_report)(LOG_EMERG,"$","daemon_init(): fork() returned %a",strerror(errno));
    exit(EXIT_FAILURE);
  }
  else if (pid != 0)	/* parent goes bye bye */
    exit(EXIT_SUCCESS);
  
  /*----------------------------------------------
  ; there used to be a chdir("/tmp") call here, but
  ; then the re-exec may fail if the program was
  ; started using a relative path.  It has been
  ; disabled for now.
  ;----------------------------------------------*/

  /*chdir("/tmp");*/

  setsid();

  StreamFree(StdinStream);	/* these no longer needed */
  StreamFree(StdoutStream);
  close(STDOUT_FILENO);
  close(STDIN_FILENO);

  if (cv_report == report_syslog)
  {
    StreamFree(StderrStream); /* except maybe this one */
    close(STDERR_FILENO);
  }
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
    StreamFlush(StdoutStream);
    StreamFlush(StderrStream);
  }
}

/*************************************************************************/

