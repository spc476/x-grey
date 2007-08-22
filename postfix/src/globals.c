
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <syslog.h>
#include <getopt.h>

#include <cgilib/memory.h>
#include <cgilib/util.h>
#include <cgilib/ddt.h>

#include "../../common/src/graylist.h"
#include "../../common/src/globals.h"
#include "../../common/src/util.h"

enum
{
  OPT_NONE,
  OPT_LOG_FACILITY,
  OPT_LOG_LEVEL,
  OPT_LOG_ID,
  OPT_DEBUG,
  OPT_HELP,
  OPT_MAX
};

/*****************************************************************/

static void		 parse_cmdline	(int,char *[]);
static void		 dump_defaults	(void);

/****************************************************************/

char  *c_host        = DEF_HOST;
int    c_port        = DEF_PORT;
int   c_log_facility = LOG_LOCAL5;
int   c_log_level    = LOG_DEBUG;
char *c_log_id	     = "pfgl";
int   cf_debug       = 0;
void  (*cv_report)(int,char *,char *,...) = report_syslog;

  /*----------------------------------------------------*/

static const struct option mc_options[] =
{
  { "log-facility"	, required_argument	, NULL	, OPT_LOG_FACILITY	} ,
  { "log-level"		, required_argument	, NULL	, OPT_LOG_LEVEL		} ,
  { "log-id"		, required_argument	, NULL	, OPT_LOG_ID		} ,
  { "debug"		, no_argument		, NULL	, OPT_DEBUG		} ,
  { "help"		, no_argument		, NULL	, OPT_HELP		} ,
  { NULL		, 0			, NULL	, 0			} 
};

/*********************************************************************/

int (GlobalsInit)(int argc,char *argv[])
{
  ddt(argc >  0);
  ddt(argv != NULL);
  
  parse_cmdline(argc,argv);
  openlog(c_log_id,0,c_log_facility);
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
      case OPT_LOG_FACILITY:
           {
             char *tmp = up_string(dup_string(optarg));
             c_log_facility = ci_map_int(tmp,c_facilities,C_FACILITIES);
             MemFree(tmp);
           }
           break;
      case OPT_LOG_LEVEL:
           {
             char *tmp = up_string(dup_string(optarg));
             c_log_level = ci_map_int(tmp,c_levels,C_LEVELS);
             MemFree(tmp);
           }
           break;
      case OPT_LOG_ID:
           c_log_id = dup_string(optarg);
           break;
      case OPT_DEBUG:
           cf_debug = 1;
           break;
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
  fprintf(
    stderr,
    "\t--log-facility <facility> (%s)\n"
    "\t--log-level <level>	 (%s)\n"
    "\t--log-id <id>		 (%s)\n"
    "\t--debug			 (%s)\n"
    "\t--help\n",
    ci_map_chars(c_log_facility,c_facilities,C_FACILITIES),
    ci_map_chars(c_log_level,   c_levels,    C_LEVELS),
    c_log_id,
    (cf_debug) ? "true" : "false"
  );  
}

/********************************************************************/

