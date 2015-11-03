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

#define _BSD_SOURCE

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include <syslog.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cgilib6/util.h>

#include "../common/greylist.h"
#include "../common/globals.h"
#include "../common/util.h"
#include "../conf.h"

enum
{
  OPT_TIMEOUT = OPT_USER
};

/*****************************************************************/

static void		 parse_cmdline	(int,char *[]);
static void		 dump_defaults	(void);

/****************************************************************/

char                *c_host         = POSTFIX_HOST;
int                  c_port         = 0;
char                *c_timeformat   = "%c";
char                *c_rhost        = SERVER_HOST;
int                  c_rport        = SERVER_PORT;
struct sockaddr_in   c_raddr;
socklen_t            c_raddrsize    = sizeof(struct sockaddr_in);
int                  c_timeout      = POSTFIX_TIMEOUT;
int                  c_log_facility = POSTFIX_LOG_FACILITY;
int                  c_log_level    = POSTFIX_LOG_LEVEL;
char                *c_log_id	    = POSTFIX_LOG_ID;
bool                 cf_debug       = false;
char                *c_secret       = SECRET;
size_t               c_secretsize   = SECRETSIZE;
void               (*cv_report)(int,const char *, ...) = report_syslog;

  /*----------------------------------------------------*/

static const struct option mc_options[] =
{
  { "host"		, required_argument	, NULL	, OPT_HOST		} ,
  { "port"		, required_argument	, NULL	, OPT_PORT		} ,
  { "server"		, required_argument	, NULL	, OPT_RHOST		} ,
  { "server-port"	, required_argument	, NULL	, OPT_RPORT		} ,
  { "timeout"		, required_argument	, NULL  , OPT_TIMEOUT		} ,
  { "log-facility"	, required_argument	, NULL	, OPT_LOG_FACILITY	} ,
  { "log-level"		, required_argument	, NULL	, OPT_LOG_LEVEL		} ,
  { "log-id"		, required_argument	, NULL	, OPT_LOG_ID		} ,
  { "secret"		, required_argument	, NULL	, OPT_SECRET		} ,
  { "debug"		, no_argument		, NULL	, OPT_DEBUG		} ,
  { "version"		, no_argument		, NULL	, OPT_VERSION		} ,
  { "help"		, no_argument		, NULL	, OPT_HELP		} ,
  { NULL		, 0			, NULL	, 0			} 
};

/*********************************************************************/

int (GlobalsInit)(int argc,char *argv[])
{
  struct hostent *remote;

  assert(argc >  0);
  assert(argv != NULL);
  
  parse_cmdline(argc,argv);
  remote = gethostbyname(c_rhost);
  if (remote == NULL)
  {
    (*cv_report)(LOG_ERR,"gethostbyname(%s) = %s",c_rhost,strerror(errno));
    return EXIT_FAILURE;
  }
  
  memcpy(&c_raddr.sin_addr.s_addr,remote->h_addr,remote->h_length);
  c_raddr.sin_family = AF_INET;
  c_raddr.sin_port   = htons(c_rport);

  openlog(c_log_id,0,c_log_facility);
  return EXIT_SUCCESS;
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
    "\t--timeout <time>          (%s)\n"
    "\t--log-facility <facility> (%s)\n"
    "\t--log-level <level>       (%s)\n"
    "\t--log-id <id>             (%s)\n"
    "\t--debug                   (%s)\n"
    "\t--version                 (" PROG_VERSION ")\n"
    "\t--help\n",
    tout,
    ci_map_chars(c_log_facility,c_facilities,C_FACILITIES),
    ci_map_chars(c_log_level,   c_levels,    C_LEVELS),
    c_log_id,
    (cf_debug) ? "true" : "false"
  );
  
  free(tout);
}

/********************************************************************/

