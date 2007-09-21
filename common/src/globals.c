
#include <syslog.h>

#include "graylist.h"
#include "util.h"

/******************************************************************/

const struct chars_int c_facilities[20] =
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

const struct chars_int c_levels[7] = 
{
  { "ALERT"     , LOG_ALERT     } ,     
  { "CRIT"      , LOG_CRIT      } ,     
  { "DEBUG"     , LOG_DEBUG     } ,
  { "ERR"       , LOG_ERR       } ,
  { "INFO"      , LOG_INFO      } ,
  { "NOTICE"    , LOG_NOTICE    } ,
  { "WARNING"   , LOG_WARNING   }
};

const struct chars_int c_ipcmds[4] =
{
  { "NONE (this is a bug)"	, IPCMD_NONE		} ,
  { "ACCEPT"			, IPCMD_ACCEPT		} ,
  { "REJECT"			, IPCMD_REJECT		} ,
  { "GRAYLIST"			, IPCMD_GRAYLIST	} 
};

