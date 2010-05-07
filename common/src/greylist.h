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

#ifndef GREYLIST_COMMON_H
#define GREYLIST_COMMON_H

#include <time.h>
#include <limits.h>

#include "../../version.h"

	/*----------------------------------------------------------------
	; PROG_VERSION (in version.h) defines the actual program version. 
	; VERSION is the protocol version.  The names are this way for
	; hysterical reasons.
	;----------------------------------------------------------------*/

#define VERSION		0x0100

/********************************************************************/

#if (UCHAR_MAX == 255U)
  typedef unsigned char byte;
#else
#  error No integral type is 8 bits on this platform
#endif

#if (UINT_MAX == 65535U)
  typedef unsigned int unet16;
  typedef unsigned int Port;
#elif (USHRT_MAX == 65535U)
  typedef unsigned short unet16;
  typedef unsigned short Port;
#elif (UCHAR_MAX == 65535U)
  typedef unsigned char unet16;
  typedef unsigned char Port;
#else
# error No integral type is 16 bits on this platform.
#endif

#if (UINT_MAX == 4294967295UL)
  typedef unsigned int unet32;
  typedef unsigned int IP;
  typedef unsigned int flags;
#elif (ULONG_MAX == 4294967295UL)
  typedef unsigned long unet32;
  typedef unsigned long IP;
  typedef unsigned long flags;
#elif (USHORT_MAX == 4294967295UL)
  typedef unsigned short unet32;
  typedef unsigned short IP;
  typedef unsigned short flags;
#elif (UCHAR_MAX == 4294967295UL)
  typedef unsigned char unet32;
  typedef unsigned char IP;
  typedef unsigned char flags;
#else
# error No integral type is 32 bits on this platform.
#endif

enum
{
  OPT_NONE,
  OPT_HELP,
  OPT_VERSION,
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
  CMD_GREYLIST,
  CMD_GREYLIST_RESP,
  CMD_WHITELIST,
  CMD_WHITELIST_RESP,
  CMD_TUPLE_REMOVE,
  CMD_TUPLE_REMOVE_RESP,
  
  /*------------------------------------
  ; commands from the MCP
  ;-------------------------------------*/
  
  CMD_MCP_SHOW_STATS,
  CMD_MCP_SHOW_STATS_RESP,
  CMD_MCP_SHOW_CONFIG,
  CMD_MCP_SHOW_CONFIG_RESP,
  CMD_MCP_SHOW_IPLIST,
  CMD_MCP_SHOW_IPLIST_RESP,
  CMD_MCP_SHOW_TUPLE,
  CMD_MCP_SHOW_TUPLE_RESP,
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
  REASON_NONE,
  REASON_IP,
  REASON_FROM,
  REASON_FROM_DOMAIN,
  REASON_TO,
  REASON_TO_DOMAIN,
  REASON_GREYLIST,
  REASON_WHITELIST
};

enum
{
  IFT_GREYLIST,
  IFT_ACCEPT,
  IFT_REJECT,
  IFT_REMOVE,
  IFT_NONE
};

struct greylist_response
{
  unet32 crc;
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 response;
  unet16 why;
};

struct greylist_request
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
  unet32 ip_greylist;
  unet32 ip_accept;
  unet32 ip_reject;
  unet32 greylisted;
  unet32 whitelisted;
  unet32 greylist_expired;
  unet32 whitelist_expired;
  unet32 requests;
  unet32 requests_cu;
  unet32 requests_cu_max;
  unet32 requests_cu_ave;
  unet32 from;
  unet32 from_greylist;
  unet32 from_accept;
  unet32 from_reject;
  unet32 fromd;
  unet32 fromd_greylist;
  unet32 fromd_accept;
  unet32 fromd_reject;
  unet32 to;
  unet32 to_greylist;
  unet32 to_accept;
  unet32 to_reject;
  unet32 tod;
  unet32 tod_greylist;
  unet32 tod_accept;
  unet32 tod_reject;
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
  unet32 timeout_grey;
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

union greylist_all_packets
{
  struct greylist_request           req;
  struct greylist_response          res;
  struct glmcp_request              mcp_req;
  struct glmcp_response             mcp_res;
  struct glmcp_response_show_stats  mcp_show_stats;
  struct glmcp_response_show_config mcp_show_config;
  struct glmcp_request_iplist       mcp_iplist;
  struct glmcp_request_tofrom       mcp_tofrom;
  byte                              data[1500];
};

/***************************************************************/

#endif
