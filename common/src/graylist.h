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

#ifndef GRAYLIST_COMMON_H
#define GRAYLIST_COMMON_H

#include <time.h>

#define VERSION		0x0102

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
  OPT_SECRET,
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
  /*--------------------------------------
  ; commands from MTAs
  ;--------------------------------------*/
  
  CMD_NONE,
  CMD_NONE_RESP,
  CMD_GRAYLIST,
  CMD_GRAYLIST_RESP,
  
  /*------------------------------------
  ; commands from the MCP
  ;-------------------------------------*/
  
  CMD_MCP_SHOW_STATS,
  CMD_MCP_SHOW_STATS_RESP,
  CMD_MCP_SHOW_CONFIG,
  CMD_MCP_SHOW_CONFIG_RESP,
  CMD_MCP_SHOW_IPLIST,
  CMD_MCP_SHOW_IPLIST_RESP,
  CMD_MCP_SHOW_TUPLE_ALL,
  CMD_MCP_SHOW_TUPLE_ALL_RESP,
  CMD_MCP_SHOW_WHITELIST,
  CMD_MCP_SHOW_WHITELIST_RESP,

  CMD_MCP_SHOW_TO,
  CMD_MCP_SHOW_TO_RESP,
  CMD_MCP_SHOW_TO_DOMAIN,
  CMD_MCP_SHOW_TO_DOMAIN_RESP,

  CMD_MCP_SHOW_FROM,
  CMD_MCP_SHOW_FROM_RESP,
  CMD_MCP_SHOW_FROM_DOMAIN,
  CMD_MCP_SHOW_FROM_DOMAIN_RESP,
  
  CMD_MCP_IPLIST,
  CMD_MCP_IPLIST_RESP,
  
  CMD_MCP_TO,
  CMD_MCP_TO_RESP,
  CMD_MCP_TO_DOMAIN,
  CMD_MCP_TO_DOMAIN_RESP,

  CMD_MCP_FROM,
  CMD_MCP_FROM_RESP,
  CMD_MCP_FROM_DOMAIN,
  CMD_MCP_FROM_DOMAIN_RESP,
  CMD_MAX
};

enum
{
  GLERR_OKAY,
  GLERR_VERSION_NOT_SUPPORTED,
  GLERR_MTA_NOT_SUPPORTED,
  GLERR_TYPE_NOT_SUPPORTED,
  GLERR_BAD_DATA,
  GLERR_CANT_GENERATE_REPORT,
  GLERR_IPADDR_NOT_SUPPORTED,
  GLERR_MAX
};

enum
{
  GRAYLIST_NAY,
  GRAYLIST_YEA,
  GRAYLIST_LATER
};

enum
{
  IPCMD_NONE,
  IPCMD_ACCEPT,
  IPCMD_REJECT,
  IPCMD_GRAYLIST,
  IPCMD_REMOVE
};

struct graylist_response
{
  unet32 crc;
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 response;
};

struct graylist_request
{
  unet32 crc;
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
  unet32 crc;
  unet16 version;
  unet16 MTA;
  unet16 type;
};

struct glmcp_response
{
  unet32 crc;
  unet16 version;
  unet16 MTA;
  unet16 type;
};

struct glmcp_response_show_stats
{
  unet32 crc;
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
  unet32 requests;
  unet32 requests_cu;
  unet32 requests_cu_max;
  unet32 requests_cu_ave;
  unet32 from;
  unet32 fromd;
  unet32 to;
  unet32 tod;
};

struct glmcp_response_show_config
{
  unet32 crc;
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 pad;
  unet32 max_tuples;
  unet32 timeout_cleanup;
  unet32 timeout_embargo;
  unet32 timeout_gray;
  unet32 timeout_white;
};

struct glmcp_request_iplist
{
  unet32 crc;
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 cmd;
  unet16 ipsize;
  unet16 mask;	/* the "/X" part of A.B.C.D/X */
  byte   data[16];
};

struct glmcp_request_tofrom
{
  unet32 crc;
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 cmd;
  unet16 size;
  byte   data[1];
};

/***************************************************************/

#endif
