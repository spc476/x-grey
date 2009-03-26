/***************************************************************************
*
* Copyright 2007 by Sean Conner.
*
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

#include <syslog.h>

#include "greylist.h"
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

const struct chars_int c_ift[5] =
{
  { "GREYLIST"			, IFT_GREYLIST	} ,
  { "GREYLIST"			, IFT_GREYLIST	} ,
  { "ACCEPT"			, IFT_ACCEPT	} ,
  { "REJECT"			, IFT_REJECT	} ,
  { "REMOVE"			, IFT_REMOVE	} ,
};  

const struct chars_int c_reason[8] =
{
  { "NONE"			, REASON_NONE		} ,
  { "IP"			, REASON_IP		} ,
  { "FROM"			, REASON_FROM		} ,
  { "FROM-DOMAIN"		, REASON_FROM_DOMAIN	} ,
  { "TO"			, REASON_TO		} ,
  { "TO-DOMAIN"			, REASON_TO_DOMAIN	} ,
  { "GREYLIST"			, REASON_GREYLIST	} ,
  { "WHITELIST"			, REASON_WHITELIST	}
};
