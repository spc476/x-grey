
#ifndef GRAYLIST_COMMON_H
#define GRAYLIST_COMMON_H

#include <time.h>

#define VERSION		0x0101

#define DEF_HOST	"localhost"
#define DEF_PORT	9990
#define DEF_RHOST	"localhost"
#define DEF_RPORT	9990
#define DEF_LHOST	"localhost"
#define DEF_LPORT	20000

/********************************************************************/

typedef unsigned char  byte;
typedef unsigned long  IP;
typedef unsigned long  flags;
typedef unsigned short Port;
typedef unsigned short unet16;
typedef unsigned long  unet32;

enum
{
  OPT_NONE,
  OPT_HELP,
  OPT_DEBUG,
  OPT_LOG_FACILITY,
  OPT_LOG_LEVEL,
  OPT_LOG_ID,
  OPT_HOST,	/* local host */
  OPT_PORT,	/* local port */
  OPT_RHOST,	/* remote host */
  OPT_RPORT,	/* remote port */
  OPT_USER
};

enum
{
  MTA_MCP,
  MTA_SENDMAIL,
  MTA_QMAIL,
  MTA_POSTFIX,
  MTA_USER = 16
};

enum
{
  CMD_NONE,
  CMD_NONE_RESP,
  CMD_GRAYLIST,
  CMD_GRAYLIST_RESP,
  CMD_MCP_SHOW_STATS,
  CMD_MCP_SHOW_STATS_RESP,
  CMD_MCP_SHOW_CONFIG,
  CMD_MCP_SHOW_CONFIG_RESP,
  CMD_MAX
};

enum
{
  GLERR_OKAY,
  GLERR_VERSION_NOT_SUPPORTED,
  GLERR_MTA_NOT_SUPPORTED,
  GLERR_TYPE_NOT_SUPPORTED,
  GLERR_BAD_DATA,
  GLERR_MAX
};

enum
{
  GRAYLIST_NAY,
  GRAYLIST_YEA,
  GRAYLIST_LATER
};

struct graylist_response
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 response;
};

struct graylist_request
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 ipsize;
  unet16 fromsize;
  unet16 tosize;
  byte   data[1];
};

struct glmcp_request
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 pad;
};

struct glmcp_response
{
  unet16 version;
  unet16 MTA;
  unet16 type;
};

struct glmcp_response_show_stats
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 pad;
  unet32 starttime;
  unet32 nowtime;
  unet32 tuples;
  unet32 ips;
  unet32 graylisted;
  unet32 whitelisted;
  unet32 graylist_expired;
  unet32 whitelist_expired;
};

struct glmcp_response_show_config
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 pad;
  unet32 max_tuples;
  unet32 max_ips;
  unet32 timeout_cleanup;
  unet32 timeout_embargo;
  unet32 timeout_gray;
  unet32 timeout_white;
};

#if 0

#define F_WHITELIST	(1uL << 0)
#define F_GRAYLIST	(1uL << 1)
#define F_TRUNCFROM	(1uL << 2)
#define F_TRUNCTO	(1uL << 3)
#define F_IPv6		(1uL << 4)

typedef struct tuple
{
  time_t       ctime;
  time_t       atime;
  size_t       fromsize;
  size_t       tosize;
  unsigned int pad;
  flags        f;
  byte         ip  [16];
  char         from[108];
  char         to  [108];
} *Tuple;

#endif

/***************************************************************/

#endif
