
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
#include <arpa/inet.h>

#include <cgilib/memory.h>
#include <cgilib/errors.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>
#include <cgilib/types.h>
#include <cgilib/ddt.h>

#include "../../common/src/graylist.h"
#include "../../common/src/globals.h"
#include "../../common/src/util.h"

#include "graylist.h"
#include "signals.h"
#include "util.h"
#include "iplist.h"
#include "iptable.h"
#include "emaildomain.h"

enum
{
  OPT_LIST_WHITE = OPT_USER,
  OPT_LIST_GRAY,
  OPT_MAX_TUPLES,
  OPT_TIME_CLEANUP,
  OPT_TIME_SAVESTATE,
  OPT_TIMEOUT_EMBARGO,
  OPT_TIMEOUT_GRAY,
  OPT_TIMEOUT_WHITE,
  OPT_FILE_IPLIST,
  OPT_REPORT_FORMAT,
  OPT_TIME_FORMAT,  
  OPT_FOREGROUND,
  OPT_STDERR,
  OPT_MAX
};

/********************************************************************/

static void		 dump_defaults	(void);
static void		 parse_cmdline	(int,char *[]);
static void		 daemon_init	(void);
static void		 my_exit	(void);

/*********************************************************************/

extern char **environ;

char          *c_pidfile         = "/tmp/graylist.pid";
char          *c_host            = DEF_HOST;
int            c_port            = DEF_PORT;
int            c_log_facility    = LOG_LOCAL6;
int            c_log_level       = LOG_INFO;
char	      *c_log_id          = "graylist";
int            cf_debug          = 0;
void         (*cv_report)(int,char *,char *, ...) = report_syslog;

char          *c_whitefile       = "/tmp/whitelist.txt";
char          *c_grayfile        = "/tmp/grayfile.txt";	
char          *c_dumpfile        = "/tmp/dump.txt";
char          *c_iplistfile      = "/tmp/iplist.txt";
char          *c_tofile          = "/tmp/to.txt";
char          *c_todfile         = "/tmp/to-domain.txt";
char          *c_fromfile        = "/tmp/from.txt";
char          *c_fromdfile       = "/tmp/from-domain.txt";
char          *c_timeformat      = "%c";
size_t         c_poolmax         = 65536uL;
unsigned int   c_time_cleanup    =    60   * 5;
double         c_time_savestate  =  3600.0;
double	       c_timeout_embargo =    60.0 * 25.0; /* down from an hour */
double         c_timeout_gray    =  3600.0 * 6.0;  /* 4.0;*/
double	       c_timeout_white   =  3600.0 * 24.0 * 36.0;
time_t         c_starttime       =     0;
int            cf_foreground     =     0;

	/*---------------------------------------------------*/

char               **g_argv;

size_t               g_poolnum;
struct tuple        *g_pool;		/* actual space */
Tuple               *g_tuplespace;	/* used for sorting records */

size_t               g_graylisted;
size_t               g_whitelisted;
size_t               g_whitelist_expired;
size_t               g_graylist_expired;

struct ipnode       *g_tree;
size_t               g_ipcnt = 1;

size_t		     g_smaxfrom;
size_t	             g_sfrom;
struct emaildomain  *g_from;
size_t               g_smaxfromd;
size_t               g_sfromd;
struct emaildomain  *g_fromd;
size_t               g_smaxto;
size_t               g_sto;
struct emaildomain  *g_to;
size_t               g_smaxtod;
size_t               g_stod;
struct emaildomain  *g_tod;

time_t               g_time_savestate;

/*******************************************************************/

volatile int m_debug = 1;

static const struct option mc_options[] =
{
  { "whitelist"      	, required_argument	, NULL	, OPT_LIST_WHITE	},
  { "graylist"		, required_argument	, NULL	, OPT_LIST_GRAY 	} ,
  { "greylist"		, required_argument	, NULL	, OPT_LIST_GRAY		} ,
  { "host"		, required_argument	, NULL	, OPT_HOST		} ,
  { "port"		, required_argument	, NULL	, OPT_PORT		} ,
  { "max-tuples"	, required_argument	, NULL	, OPT_MAX_TUPLES	} ,
  { "time-cleanup" 	, required_argument	, NULL	, OPT_TIME_CLEANUP	} ,
  { "time-checkpoint"	, required_argument	, NULL	, OPT_TIME_SAVESTATE	} ,
  { "timeout-embargo"	, required_argument	, NULL	, OPT_TIMEOUT_EMBARGO	} ,
  { "timeout-gray"	, required_argument	, NULL	, OPT_TIMEOUT_GRAY	} ,
  { "timeout-grey"	, required_argument	, NULL	, OPT_TIMEOUT_GRAY	} ,
  { "timeout-white"	, required_argument	, NULL	, OPT_TIMEOUT_WHITE	} ,
  { "iplist"		, required_argument	, NULL	, OPT_FILE_IPLIST	} ,
  { "time-format"     	, required_argument 	, NULL	, OPT_TIME_FORMAT    	} ,
  { "report-format"     , required_argument 	, NULL	, OPT_REPORT_FORMAT	} ,
  { "log-facility"   	, required_argument 	, NULL	, OPT_LOG_FACILITY	} ,
  { "log-level"      	, required_argument 	, NULL	, OPT_LOG_LEVEL		} ,
  { "log-id"      	, required_argument 	, NULL	, OPT_LOG_ID        	} ,
  { "debug"          	, no_argument       	, NULL	, OPT_DEBUG		} ,
  { "foreground"     	, no_argument       	, NULL	, OPT_FOREGROUND   	} ,
  { "stderr"	     	, no_argument       	, NULL	, OPT_STDERR       	} ,
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
  g_tree->match  = IPCMD_GRAYLIST;
  
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

  set_signal(SIGCHLD, sighandler_chld);
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

  {
    Stream pfile;
    
    pfile = FileStreamWrite(c_pidfile,FILE_CREATE | FILE_TRUNCATE);
    if (pfile == NULL)
      (*cv_report)(LOG_ERR,"$","unable to write pid file %a",c_pidfile);
    else
    {
      LineSFormat(pfile,"L","%a\n",(unsigned long)getpid());
      StreamFlush(pfile);
      StreamFree(pfile);
    }
  }
  
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
  char *togray;
  char *towhite;
  char *toclean;

  togray  = report_delta(c_timeout_gray);
  towhite = report_delta(c_timeout_white);
  toclean = report_delta(c_time_cleanup);
  
  LineSFormat(
  	StderrStream,
  	"$ $ $ i i L $ $ $ $ $ $ $ $ $ $ $ $",
  	"\t--whitelist <file>\t\t(%a)\n"
  	"\t--graylist  <file>\t\t(%b)\n"
  	"\t--host <hostname>\t\t(%c)\n"
  	"\t--port <num>\t\t\t(%d)\n"
  	"\t--max-tuples <num>\t\t(%f)\n"
  	"\t--time-cleanup <num>\t\t(%q)\n"
  	"\t--timeout-gray <timespec>\t(%g)\n"
  	"\t--timeout-white <timespec>\t(%h)\n"
  	"\t--iplist <file>\t\t\t(%r)\n"
  	"\t--time-format <strftime>\t(%i)\n"
  	"\t--report-format syslog | stderr\t(%j)\n"
  	"\t--sys-facility <facility>\t(%k)\n"
  	"\t--sys-level <level>\t\t(%l)\n"
  	"\t--sys-sysid <string>\t\t(%m)\n"
  	"\t--debug\t\t\t\t(%n)\n"
  	"\t--foreground\t\t\t(%o)\n"
  	"\t--stderr\t\t\t(%p)\n"
  	"\t--help\n"
  	"\t\t* not implemented\n",
  	c_whitefile,
  	c_grayfile,
  	c_host,
  	c_port,
  	0,
  	(unsigned long)c_poolmax,
  	togray,
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
  	c_iplistfile
  );

  MemFree(toclean);
  MemFree(towhite);
  MemFree(togray);
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
      case OPT_LIST_GRAY:
           c_grayfile = dup_string(optarg);
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
           c_time_cleanup = (int)read_dtime(optarg);
           break;
      case OPT_TIME_SAVESTATE:
           c_time_savestate = read_dtime(optarg);
           break;
      case OPT_TIMEOUT_EMBARGO:
           c_timeout_embargo = read_dtime(optarg);
           break;
      case OPT_TIMEOUT_GRAY:
           c_timeout_gray = read_dtime(optarg);
           break;
      case OPT_TIMEOUT_WHITE:
           c_timeout_white = read_dtime(optarg);
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
      case OPT_STDERR:
           cv_report = report_stderr;
           break;
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

#if 0
static size_t read_size(char *arg)
{
  size_t  value;
  char   *p;
  
  ddt(arg != NULL);
  
  value = strtoul(arg,&p,10);
  if (value == 0)
  {
    LineSFormat(StderrStream,"$","bad size specified: %a\n",arg);
    exit(EXIT_FAILURE);
  }
  
  switch(*p)
  {
    case 'k': value *= 1024uL; break;
    case 'm': value *= (1024uL * 1024uL); break;
    case 'g': value *= (1024uL * 1024uL * 1024uL); break;
    case 'b':
    default:
         break;
  }
  return(value);
}
#endif

/********************************************************************/

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

