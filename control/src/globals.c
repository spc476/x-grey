
#include <stdlib.h>

#include <syslog.h>
#include <getopt.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cgilib/stream.h>
#include <cgilib/memory.h>
#include <cgilib/util.h>
#include <cgilib/types.h>
#include <cgilib/errors.h>
#include <cgilib/ddt.h>

#include "../../common/src/graylist.h"
#include "../../common/src/util.h"
#include "../../common/src/globals.h"

/*********************************************************/

static void	parse_cmdline	(int,char *[]);
static void	dump_defaults	(void);
static void	my_exit		(void);

/**********************************************************/

const char          *c_host         = DEF_HOST;
int                  c_port         = 0;
const char          *c_rhost        = DEF_RHOST;
int                  c_rport        = DEF_RPORT;
int                  c_timeout      = 5;
struct sockaddr_in   c_raddr;
socklen_t            c_raddrsize = sizeof(struct sockaddr_in);
int                  c_log_facility = LOG_LOCAL5;
int                  c_log_level    = LOG_INFO;
const char          *c_log_id       = "gld-mcp";
int                  cf_debug       = FALSE;
const char          *c_timeformat   = "%c";
const char          *c_pager        = "/bin/more";
void               (*cv_report)(int,char *,char *, ... ) = report_stderr;

/**********************************************************/

static const struct option mc_options[] =
{
  { "host"		, required_argument	, NULL	, OPT_HOST 		} ,
  { "port"		, required_argument	, NULL	, OPT_PORT 		} ,
  { "remote-host"	, required_argument	, NULL	, OPT_RHOST		} ,
  { "remote-port"	, required_argument	, NULL	, OPT_RPORT		} ,
  { "log-facility"	, required_argument	, NULL	, OPT_LOG_FACILITY 	} ,
  { "log-level"		, required_argument	, NULL	, OPT_LOG_LEVEL		} ,
  { "log-id"		, required_argument	, NULL	, OPT_LOG_ID		} ,
  { "debug"		, no_argument		, NULL	, OPT_DEBUG		} ,
  { "help"		, no_argument		, NULL	, OPT_HELP		} ,
  { NULL		, 0			, NULL	, 0			}
};

/********************************************************/

int (GlobalsInit)(int argc,char *argv[])
{
  struct hostent *remote;
  char           *pager;
  
  ddt(argc >  0);
  ddt(argv != NULL);

  pager = getenv("PAGER");
  if (pager)
    c_pager = dup_string(pager);

  parse_cmdline(argc,argv);

  remote = gethostbyname(c_rhost);
  if (remote == NULL)
  {
    (*cv_report)(LOG_ERR,"$ $","gethostbyname(%a) = %b",c_rhost,strerror(errno));
    return(EXIT_FAILURE);
  }
  
  memcpy(&c_raddr.sin_addr.s_addr,remote->h_addr,remote->h_length);
  c_raddr.sin_family = AF_INET;
  c_raddr.sin_port   = htons(c_rport);

  atexit(my_exit);
  return(ERR_OKAY);  
}

/******************************************************/

int (GlobalsDeinit)(void)
{
  return(ERR_OKAY);
}

/****************************************************/

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
      case OPT_HOST:
           c_host = dup_string(optarg);
           break;
      case OPT_PORT:
           c_port = strtoul(optarg,NULL,10);
           break;
      case OPT_RHOST:
           c_rhost = dup_string(optarg);
           break;
      case OPT_RPORT:
           c_rport = strtoul(optarg,NULL,10);
           break;
      case OPT_LOG_FACILITY:
           tmp = up_string(dup_string(optarg));
           c_log_facility = ci_map_int(tmp,c_facilities,C_FACILITIES);
           MemFree(tmp);
           break;
      case OPT_LOG_LEVEL:
           tmp = up_string(dup_string(optarg));
           c_log_level = ci_map_int(tmp,c_levels,C_LEVELS);
           MemFree(tmp);
           break;
      case OPT_LOG_ID:
           c_log_id = dup_string(optarg);
           break;
      case OPT_DEBUG:
           cf_debug    = TRUE;
           c_log_level = LOG_DEBUG;
           LineSFormat(StderrStream,"","using '--log-facility debug'\n");
           break;
      case OPT_HELP:
      default:
           LineSFormat(StderrStream,"$","usage: %a [options] command [data]\n",argv[0]);
           dump_defaults();
           exit(EXIT_FAILURE);
    }
  }
}

/********************************************************/

static void dump_defaults(void)
{
  LineSFormat(
  	StderrStream,
  	"$ i $ $ $ $ $ i",
  	"\t--host <hostname>\t\t(%a)\n"
  	"\t--port <num>\t\t\t(%b)\n"
  	"\t--remote-host <hostname>\t(%g)\n"
  	"\t--remote-port <num>\t\t(%h)\n"
  	"\t--log-facility <facility>\t(%c)\n"
  	"\t--log-level <level>\t\t(%d)\n"
  	"\t--log-id <string>\t\t(%e)\n"
  	"\t--debug\t\t\t\t(%f)\n"
  	"\t--help\n",
  	c_host,
  	c_port,
  	ci_map_chars(c_log_facility,c_facilities,C_FACILITIES),
  	ci_map_chars(c_log_level,   c_levels,    C_LEVELS),
  	c_log_id,
  	(cf_debug) ? "true" : "false",
  	c_rhost,
  	c_rport
  );
}

/*****************************************************/

static void my_exit(void)
{
  StreamFlush(StderrStream);
  StreamFlush(StdoutStream);
}

/*******************************************************/
