
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <syslog.h>
#include <getopt.h>

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

struct chars_int
{
  const char *name;
  const int   value;
};

/*****************************************************************/

static void		 parse_cmdline	(int,char *[]);
static void		 dump_defaults	(void);
static int		 ci_map_int	(const char *,const struct chars_int *,size_t);
static const char	*ci_map_chars	(int,const struct chars_int *,size_t);
static char		*up_string	(char *);

/****************************************************************/

int   c_log_facility = LOG_LOCAL5;
int   c_log_level    = LOG_DEBUG;
char *c_log_id	     = "pfgl";
int   cf_debug       = 0;

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

/*********************************************************************/

int (GlobalsInit)(int argc,char *argv[])
{
  assert(argc >  0);
  assert(argv != NULL);
  
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
      case OPT_LOG_FACILITY:
           {
             char *tmp = up_string(strdup(optarg));
             c_log_facility = ci_map_int(tmp,m_facilities,mc_facilities_cnt);
             free(tmp);
           }
           break;
      case OPT_LOG_LEVEL:
           {
             char *tmp = up_string(strdup(optarg));
             c_log_level = ci_map_int(tmp,m_levels,mc_levels_cnt);
             free(tmp);
           }
           break;
      case OPT_LOG_ID:
           c_log_id = strdup(optarg);
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
    ci_map_chars(c_log_facility,m_facilities,mc_facilities_cnt),
    ci_map_chars(c_log_level,   m_levels,    mc_levels_cnt),
    c_log_id,
    (cf_debug) ? "true" : "false"
  );  
}

/********************************************************************/

static int ci_map_int(const char *name,const struct chars_int *list,size_t size)
{
  int i;
  
  assert(name != NULL);
  assert(list != NULL);
  assert(size >  0);
  
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
  
  assert(list != NULL);
  assert(size >  0);
  
  for (i = 0 ; i < size ; i++)
  {
    if (value == list[i].value)
      return(list[i].name);
  }
  return("");
}

/*************************************************************************/

static char *up_string(char *s)
{
  char *r = s;
  
  for ( ; *s ; s++)
    *s = toupper(*s);
  
  return(r);
}

/*********************************************************************/
