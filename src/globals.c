/************************************************************************
*
* Copyright 2007 by Sean Conner.  All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>

#include <cgilib/memory.h>
#include <cgilib/errors.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>
#include <cgilib/types.h>
#include <cgilib/ddt.h>

#include "graylist.h"

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

/************************************************************/

static void		 daemon_init		(void);
static void		 report_syslog		(int,char *,char *, ... );
static void		 report_stderr		(int,char *,char *, ... );
static void		 parse_cmdline		(int,char *[]);
static size_t		 read_size		(char *);
static double   	 read_dtime		(char *);
static void		 dump_defaults		(void);
static void		 read_dump		(void);
static int		 ci_map_int		(const char *,const struct chars_int *,size_t);
static const char	*ci_map_chars		(int,const struct chars_int *,size_t);
static void		 my_exit		(void);

/***************************************************************/

char          *c_whitefile       = "/tmp/whitelist.txt";
char          *c_grayfile        = "/tmp/grayfile.txt";	
char          *c_timeformat      = "%c";
char          *c_host            = "localhost";
int            c_pfport          = 9990;
int            c_smport          = 9991;
size_t         c_poolmax         = 65536uL;
unsigned int   c_timeout_cleanup = 60;
double	       c_timeout_accept  = 3600.0;
double         c_timeout_gray    = 3600.0 *  4.0;
double	       c_timeout_white   = 3600.0 * 24.0 * 36.0;
int	       c_facility        = LOG_LOCAL0;
int	       c_level           = LOG_INFO;
char	      *c_sysid           = "graylist";
time_t         c_starttime       = 0;
int            cf_debug          = 0;
int            cf_foreground     = 0;
void         (*cv_report)(int,char *,char *, ... ) = report_syslog;

	/*---------------------------------------------------*/
	
size_t        g_unique;
size_t        g_uniquepassed;
size_t        g_poolnum;
struct tuple *g_tuplespace;	/* actual space */
Tuple        *g_pool;		/* used for sorting records */

	/*---------------------------------------------------*/

static const struct chars_int m_facilities[] =
{
  { "AUTH"      , LOG_AUTHPRIV  } ,
  { "AUTHPRIV"  , LOG_AUTHPRIV  } ,
  { "CRON"      , LOG_CRON      } ,
  { "DAEMON"    , LOG_DAEMON    } ,
  { "FTP"       , LOG_FTP       } ,
  { "KERN"      , LOG_KERN      } ,
  { "LOCAL0"    , LOG_LOCAL0    } ,
  { "LOCAL1"    , LOG_LOCAL1    } ,
  { "LOCAL2"    , LOG_LOCAL2    } ,
  { "LOCAL3"    , LOG_LOCAL3    } ,
  { "LOCAL4"    , LOG_LOCAL4    } ,
  { "LOCAL5"    , LOG_LOCAL5    } ,
  { "LOCAL6"    , LOG_LOCAL6    } ,
  { "LOCAL7"    , LOG_LOCAL7    } ,
  { "LPR"       , LOG_LPR       } ,
  { "MAIL"      , LOG_MAIL      } ,
  { "NEWS"      , LOG_NEWS      } ,
  { "SYSLOG"    , LOG_SYSLOG    } ,
  { "USER"      , LOG_USER      } ,
  { "UUCP"      , LOG_UUCP      } 
};

static const struct chars_int m_levels[] =
{
  { "ALERT"     , LOG_ALERT     } ,
  { "CRIT"      , LOG_CRIT      } ,
  { "DEBUG"     , LOG_DEBUG     } ,
  { "ERR"       , LOG_ERR       } ,
  { "INFO"      , LOG_INFO      } ,
  { "NOTICE"    , LOG_NOTICE    } ,
  { "WARNING"   , LOG_WARNING   }
};

static const size_t mc_facilities_cnt = sizeof(m_facilities) / sizeof(struct chars_int);
static const size_t mc_levels_cnd     = sizeof(m_levels)     / sizeof(struct chars_int);
	
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

/************************************************************/

int (GlobalInit)(int argc,char *argv[])
{
  c_starttime = time(NULL);
  
  parse_cmdline(argc,argv);

  openlog(c_sysid,0,c_facility);  
  
  g_tuplespace = MemAlloc(c_poolmax * sizeof(struct tuple));

  set_signal(SIGINT,  sighandler_sigs);
  set_signal(SIGHUP,  sighandler_sigs);
  set_signal(SIGTRAP, sighandler_sigs);
  set_signal(SIGQUIT, sighandler_sigs);
  set_signal(SIGTERM, sighandler_sigs);
  set_signal(SIGUSR1, sighandler_sigs);
  set_signal(SIGUSR2, sighandler_sigs);
  set_signal(SIGCHLD, sighandler_chld);
  set_signal(SIGWINCH,sighandler_sigs);
  
  atexit(my_exit);
  
  if (cf_debug)
    dump_defaults();

  if (!cf_foreground)
    daemon_init();

  return(ERR_OKAY);
}

/***********************************************************/

int (GlobalDeinit)(void)
{
  MemFree(g_pool);
  closelog();
  return(ERR_OKAY);
}

/************************************************************/

static void daemon_init(void)
{
  pid_t pid;
  
  pid = fork();
  if (pid == (pid_t)-1)
  {
    (*cv_report)(LOG_EMERG,"$","daemon_init: fork() returned %a",strerror(errno));
    exit(EXIT_FAILURE);
  }
  else if (pid != 0)	/* parent goes bye bye */
    exit(EXIT_SUCCESS);
  
  /*chdir("/tmp");*/

  setsid();

  StreamFree(StdinStream);	/* these streams are no longer needed */
  StreamFree(StdoutStream);
  close(STDOUT_FILENO);
  close(STDIN_FILENO);
  
  if (cv_report == report_syslog)
  {
    StreamFree(StderrStream);
    close(STDERR_FILENO);
  }
}

/*****************************************************************/

static void report_syslog(int level,char *format,char *msg, ... )
{
  Stream  out;
  va_list arg;
  
  va_start(arg,msg);
  out = StringStreamWrite();
  LineSFormatv(out,format,msg,arg);
  msg = StringFromStream(out);
  syslog(level,"%s",msg);
  MemFree(msg);
  StreamFree(out);
  va_end(arg);
}

/***********************************************************************/

static void report_stderr(int level,char *format,char *msg, ... )
{
  va_list arg;
  
  va_start(arg,msg);
  if (level <= c_level)
  {
    LineSFormatv(StderrStream,format,msg,arg);
    StreamWrite(StderrStream,'\n');
  }
  va_end(arg);
}

/***********************************************************************/

static void parse_cmdline(int argc,char *argv[])
{
  int option = 0;
  
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
           c_graylist = dup_string(optarg);
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
             char *tmp = up_string(dup_string(optarg));
             
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
             chat *tmp  = up_string(dup_string(optarg));
             c_facility = ci_map_int(tmp,m_facilities,mc_facilities_cnt);
             MemFree(tmp);
           }
           break;
      case OPT_LEVEL:
           {
             char *tmp = up_string(dup_string(optarg));
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

/*********************************************************************/

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

/*********************************************************************/

static void dump_defaults(void)
{
  char *togray;
  char *towhite;
  
  togray  = report_delta(c_timeout_gray);
  towhite = report_delta(c_timeout_white);

  LineSFormat(
  	StderrStream,
  	"$ $ $ i i L $ $ $ $ $ $ $ i i i",
  	"\t--whitelist <file>			 (%a)\n"
  	"\t--graylist  <file>			 (%b)\n"
  	"\t--host <hostname>			 (%c)\n"
  	"\t--postfix-port <num>			 (%d)\n"
  	"\t--sendmail-port <num>*		 (%e)\n"
  	"\t--max-tuples <num>			 (%f)\n"
  	"\t--timeout-gray <timespec>		 (%g)\n"
  	"\t--timeout-white <timespec>		 (%h)\n"
  	"\t--time-format <strftime>		 (%i)\n"
  	"\t--report-format <'syslog' | 'stderr'> (%j)\n"
  	"\t--sys-facility <facility>		 (%k)\n"
  	"\t--sys-level <level>			 (%l)\n"
  	"\t--sys-sysid <string>			 (%m)\n"
  	"\t--debug				 (%n)\n"
  	"\t--foreground				 (%o)\n"
  	"\t--stderr				 (%p)\n"
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

/*************************************************************************/

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
	/*--------------------------------------------
	; This routine only exists as a handy place to
	; put a breakpoint to catch those unexpected
	; calls to exit().
	;----------------------------------------------*/
}

/******************************************************************/

