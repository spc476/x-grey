
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
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <cgilib/memory.h>
#include <cgilib/errors.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>
#include <cgilib/types.h>
#include <cgilib/ddt.h>

#include "graylist.h"
#include "signals.h"
#include "util.h"

enum
{
  OPT_NONE,
  OPT_LIST_WHITE,
  OPT_LIST_GRAY,
  OPT_HOST,
  OPT_PORT_POSTFIX,
  OPT_PORT_SENDMAIL,
  OPT_MAX_TUPLES,
  OPT_TIMEOUT_CLEANUP,
  OPT_TIMEOUT_ACCEPT,
  OPT_TIMEOUT_GRAY,
  OPT_TIMEOUT_WHITE,
  OPT_REPORT_FORMAT,
  OPT_TIME_FORMAT,  
  OPT_DEBUG,
  OPT_FACILITY,
  OPT_LEVEL,
  OPT_SYSID,
  OPT_FOREGROUND,
  OPT_STDERR,
  OPT_HELP,
  OPT_MAX
};

struct chars_int
{
  const char *name;
  const int   value;
};

/********************************************************************/

static void		 dump_defaults	(void);
static void		 parse_cmdline	(int,char *[]);
static void		 daemon_init	(void);
static void		 report_syslog	(int,char *,char *, ... );
static void		 report_stderr	(int,char *,char *, ... );
static double		 read_dtime	(char *);
static int		 ci_map_int	(const char *,const struct chars_int *,size_t);
static const char	*ci_map_chars	(int,const struct chars_int *,size_t);
static void		 my_exit	(void);

/*********************************************************************/

extern char **environ;

char          *c_whitefile       = "/tmp/whitelist.txt";
char          *c_grayfile        = "/tmp/grayfile.txt";	
char          *c_timeformat      = "%c";
char          *c_host            = "localhost";
int            c_port            = 9990;
size_t         c_poolmax         = 65536uL;
unsigned int   c_timeout_cleanup = 60;
double	       c_timeout_accept  = 3600.0;
double         c_timeout_gray    = 3600.0 *  4.0;
double	       c_timeout_white   = 3600.0 * 24.0 * 36.0;
int	       c_facility        = LOG_LOCAL6;
int	       c_level           = LOG_INFO;
char	      *c_sysid           = "graylist";
time_t         c_starttime       = 0;
int            cf_debug          = 0;
int            cf_foreground     = 0;
void         (*cv_report)(int,char *,char *, ... ) = report_syslog;

	/*---------------------------------------------------*/
	
size_t          g_unique;
size_t          g_uniquepassed;
size_t          g_poolnum;
struct tuple   *g_tuplespace;	/* actual space */
Tuple          *g_pool;		/* used for sorting records */
char          **g_argv;

/*******************************************************************/

volatile int m_debug = 1;

static const struct chars_int m_facilities[] =
{
  { "AUTH"	, LOG_AUTHPRIV	} ,
  { "AUTHPRIV"	, LOG_AUTHPRIV	} ,
  { "CRON"	, LOG_CRON	} ,
  { "DAEMON"	, LOG_DAEMON	} ,
  { "FTP"	, LOG_FTP	} ,
  { "KERN"	, LOG_KERN	} ,
  { "LOCAL0"	, LOG_LOCAL0	} ,
  { "LOCAL1"	, LOG_LOCAL1	} ,
  { "LOCAL2"	, LOG_LOCAL2	} ,
  { "LOCAL3"	, LOG_LOCAL3	} ,
  { "LOCAL4"	, LOG_LOCAL4	} ,
  { "LOCAL5"	, LOG_LOCAL5	} ,
  { "LOCAL6"	, LOG_LOCAL6	} ,
  { "LOCAL7"	, LOG_LOCAL7	} ,
  { "LPR"	, LOG_LPR	} ,
  { "MAIL"	, LOG_MAIL	} ,
  { "NEWS"	, LOG_NEWS	} ,
  { "SYSLOG"	, LOG_SYSLOG	} ,
  { "USER"	, LOG_USER	} ,
  { "UUCP"	, LOG_UUCP	} 
};

static const struct chars_int m_levels[] = 
{
  { "ALERT"	, LOG_ALERT	} ,
  { "CRIT"	, LOG_CRIT	} ,
  { "DEBUG"	, LOG_DEBUG	} ,
  { "ERR"	, LOG_ERR	} ,
  { "INFO"	, LOG_INFO	} ,
  { "NOTICE"	, LOG_NOTICE	} ,
  { "WARNING"	, LOG_WARNING	}
};

static const size_t mc_facilities_cnt = sizeof(m_facilities) / sizeof(struct chars_int);
static const size_t mc_levels_cnt     = sizeof(m_levels)     / sizeof(struct chars_int);

static const struct option mc_options[] =
{
  { "whitelist"      	, required_argument	, NULL	, OPT_LIST_WHITE	},
  { "graylist"		, required_argument	, NULL	, OPT_LIST_GRAY 	} ,
  { "greylist"		, required_argument	, NULL	, OPT_LIST_GRAY		} ,
  { "host"		, required_argument	, NULL	, OPT_HOST		} ,
  { "postfix-port"	, required_argument	, NULL	, OPT_PORT_POSTFIX	} ,
  { "sendmail-port"	, required_argument	, NULL	, OPT_PORT_SENDMAIL	} ,
  { "max-tuples"	, required_argument	, NULL	, OPT_MAX_TUPLES	} ,
  { "timeout-cleanup" 	, required_argument	, NULL	, OPT_TIMEOUT_CLEANUP	} ,
  { "timeout-accept"	, required_argument	, NULL	, OPT_TIMEOUT_ACCEPT	} ,
  { "timeout-gray"	, required_argument	, NULL	, OPT_TIMEOUT_GRAY	} ,
  { "timeout-grey"	, required_argument	, NULL	, OPT_TIMEOUT_GRAY	} ,
  { "timeout-white"	, required_argument	, NULL	, OPT_TIMEOUT_WHITE	} ,
  { "time-format"     	, required_argument 	, NULL	, OPT_TIME_FORMAT    	} ,
  { "report-format"     , required_argument 	, NULL	, OPT_REPORT_FORMAT	} ,
  { "sys-facility"   	, required_argument 	, NULL	, OPT_FACILITY		} ,
  { "sys-level"      	, required_argument 	, NULL	, OPT_LEVEL		} ,
  { "sys-sysid"      	, required_argument 	, NULL	, OPT_SYSID        	} ,
  { "debug"          	, no_argument       	, NULL	, OPT_DEBUG		} ,
  { "foreground"     	, no_argument       	, NULL	, OPT_FOREGROUND   	} ,
  { "stderr"	     	, no_argument       	, NULL	, OPT_STDERR       	} ,
  { "help"	     	, no_argument       	, NULL	, OPT_HELP         	} ,
  { NULL	     	, 0		 	, NULL	, 0 			}
};

/********************************************************************/

int (GlobalsInit)(int argc,char *argv[])
{
  int rc;
  int i;
  
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
  c_starttime = time(NULL);
  
  parse_cmdline(argc,argv);
  openlog(c_sysid,0,c_facility);

  atexit(my_exit);

  g_tuplespace = MemAlloc(c_poolmax * sizeof(struct tuple));
  g_pool       = MemAlloc(c_poolmax * sizeof(Tuple));

  memset(g_tuplespace,0,c_poolmax * sizeof(struct tuple));
  memset(g_pool      ,0,c_poolmax * sizeof(Tuple));

  for (i = 0 ; i < c_poolmax ; i++)
    g_pool[i] = &g_tuplespace[i];

  if (cf_debug)
    dump_defaults();
  
  if (!cf_foreground)
    daemon_init();

  set_signal(SIGINT,  sighandler_sigs);
  set_signal(SIGQUIT, sighandler_sigs);
  set_signal(SIGTERM, sighandler_sigs);
  set_signal(SIGCHLD, sighandler_chld);
  
  set_extsignal(SIGSEGV ,sighandler_critical);
  set_extsignal(SIGBUS  ,sighandler_critical);
  set_extsignal(SIGFPE  ,sighandler_critical);
  set_extsignal(SIGILL  ,sighandler_critical);
  set_extsignal(SIGXCPU ,sighandler_critical);
  set_extsignal(SIGXFSZ ,sighandler_critical);

  close_on_exec(g_queue);
  
  return(ERR_OKAY);
}

/*********************************************************************/

int (GlobalsDeinit)(void)
{
  MemFree(g_pool);
  MemFree(g_tuplespace);
  close(g_queue);
  close(g_sigpipew);
  close(g_sigpiper);
  closelog();
  return(ERR_OKAY);
}

/*********************************************************************/

static void dump_defaults(void)
{
  char *togray;
  char *towhite;

  togray  = report_delta(c_timeout_gray);
  towhite = report_delta(c_timeout_white);

  LineSFormat(
  	StderrStream,
  	"$ $ $ i i L $ $ $ $ $ $ $ $ $ $",
  	"\t--whitelist <file>\t\t(%a)\n"
  	"\t--graylist  <file>\t\t(%b)\n"
  	"\t--host <hostname>\t\t(%c)\n"
  	"\t--postfix-port <num>\t\t(%d)\n"
  	"\t--sendmail-port <num>*\t\t(%e)\n"
  	"\t--max-tuples <num>\t\t(%f)\n"
  	"\t--timeout-gray <timespec>\t(%g)\n"
  	"\t--timeout-white <timespec>\t(%h)\n"
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
  	c_pfport,
  	c_smport,
  	(unsigned long)c_poolmax,
  	togray,
  	towhite,
  	c_timeformat,
  	(cv_report == report_stderr) ? "stderr" : "syslog",
  	ci_map_chars(c_facility,m_facilities,mc_facilities_cnt),
  	ci_map_chars(c_level,   m_levels,    mc_levels_cnt),
  	c_sysid,
  	(cf_debug) ? "true" : "false" ,
  	(cf_foreground) ? "true" : "false",
  	(cv_report == report_stderr) ? "true" : "false"
  );

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
      case OPT_PORT_POSTFIX:
           c_pfport = strtoul(optarg,NULL,10);
           break;
      case OPT_PORT_SENDMAIL:
           c_smport = strtoul(optarg,NULL,10);
           break;
      case OPT_MAX_TUPLES:
           c_poolmax = strtoul(optarg,NULL,10);
           break;
      case OPT_TIMEOUT_CLEANUP:
           c_timeout_cleanup = strtoul(optarg,NULL,10);
           break;
      case OPT_TIMEOUT_ACCEPT:
           c_timeout_accept = read_dtime(optarg);
           break;
      case OPT_TIMEOUT_GRAY:
           c_timeout_gray = read_dtime(optarg);
           break;
      case OPT_TIMEOUT_WHITE:
           c_timeout_white = read_dtime(optarg);
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
           c_level  = LOG_DEBUG;
           LineSFormat(StderrStream,"","using '--sys-facility debug'\n");
           break;
      case OPT_FACILITY:
           {
             tmp = up_string(dup_string(optarg));
             c_facility = ci_map_int(tmp,m_facilities,mc_facilities_cnt);
             MemFree(tmp);
           }
           break;
      case OPT_LEVEL:
           {
             tmp = up_string(dup_string(optarg));
             c_level   = ci_map_int(tmp,m_levels,mc_levels_cnt);
             MemFree(tmp);
           }
           break;
      case OPT_SYSID:
           c_sysid = dup_string(optarg);
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

static double read_dtime(char *arg)
{
  double time = 0.0;
  double val;
  char   *p;
  
  p = arg;
  do
  {
    val = strtod(p,&p);
    switch(toupper(*p))
    {
      case 'Y':
           val *= SECSYEAR;
           p++;
           break;
      case 'D':
           val *= SECSDAY;
           p++;
           break;
      case 'H':
           val *= SECSHOUR;
           p++;
           break;
      case 'M':
           val *= SECSMIN;
           p++;
           break;
      case 'S':
           p++;
           break;
      case '\0':
           break;
      default:
           LineSFormat(
           	StderrStream,
           	"$",
           	"Bad time specifier '%a'\n",
           	arg
           );
           exit(EXIT_FAILURE);
    }
    time += val;
  } while (*p);
  
  return(time);
}

/*******************************************************************/

static void report_syslog(int level,char *format,char *msg, ... )
{
  Stream   out;
  va_list  arg;
  char    *txt;
  
  ddt(level  >= 0);
  ddt(format != NULL);
  ddt(msg    != NULL);
  
  va_start(arg,msg);
  out = StringStreamWrite();
  LineSFormatv(out,format,msg,arg);
  txt = StringFromStream(out);
  syslog(level,"%s",txt);
  MemFree(txt);
  StreamFree(out);
  va_end(arg);
}

/********************************************************************/

static void report_stderr(int level,char *format,char *msg, ... )
{
  va_list arg;
  
  ddt(level  >= 0);
  ddt(format != NULL);
  ddt(msg    != NULL);
  
  va_start(arg,msg);
  if (level <= c_level)
  {
    LineSFormatv(StderrStream,format,msg,arg);
    StreamWrite(StderrStream,'\n');
  }
  va_end(arg);
}

/*******************************************************************/

static int ci_map_int(const char *name,const struct chars_int *list,size_t size)
{
  int i;
  
  ddt(name != NULL);
  ddt(list != NULL);
  ddt(size >  0);
  
  for (i = 0 ; i < size ; i++)
  {
    if (strcmp(name,list[i].name) == 0)
      return(list[i].value);
  }
  return(-1);
}

/************************************************************************/

static const char *ci_map_chars(int value,const struct chars_int *list,size_t size)
{
  int i;
  
  ddt(list != NULL);
  ddt(size >  0);
  
  for (i = 0 ; i < size ; i++)
  {
    if (value == list[i].value)
      return(list[i].name);
  }
  return("");
}

/**************************************************************************/

static void my_exit(void)
{
  /*----------------------------------------------
  ; this routine only exists as a handy place to
  ; put a breakpoint to catch those unexpected
  ; calls to exit().
  ;------------------------------------------------*/

  closelog();
}

/*************************************************************************/

