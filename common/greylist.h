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

#ifndef GREYLIST_COMMON_H
#define GREYLIST_COMMON_H

#include <stdint.h>
#include <time.h>
#include <limits.h>

        /*----------------------------------------------------------------
        ; PROG_VERSION (in version.h) defines the actual program version.
        ; VERSION is the protocol version.  The names are this way for
        ; hysterical reasons.
        ;----------------------------------------------------------------*/
        
#define VERSION         0x0101

/********************************************************************/

typedef uint16_t Port;
typedef uint32_t IP;
typedef uint32_t flags;

#define ERR_OKAY        0
#define ERR_ERR         1

enum
{
  OPT_NONE,
  OPT_HELP,
  OPT_VERSION,
  OPT_DEBUG,
  OPT_LOG_FACILITY,
  OPT_LOG_LEVEL,
  OPT_LOG_ID,
  OPT_HOST,     /* local host */
  OPT_PORT,     /* local port */
  OPT_RHOST,    /* remote host */
  OPT_RPORT,    /* remote port */
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
  IFT_EXPIRE,
  IFT_NONE
};

struct greylist_response
{
  uint32_t crc;
  uint16_t version;
  uint16_t MTA;
  uint16_t type;
  uint16_t response;
  uint16_t why;
};

struct greylist_request
{
  uint32_t crc;
  uint16_t version;
  uint16_t MTA;
  uint16_t type;
  uint16_t ipsize;
  uint16_t fromsize;
  uint16_t tosize;
  uint8_t   data[1];
};

struct glmcp_request
{
  uint32_t crc;
  uint16_t version;
  uint16_t MTA;
  uint16_t type;
};

struct glmcp_response
{
  uint32_t crc;
  uint16_t version;
  uint16_t MTA;
  uint16_t type;
};

struct glmcp_response_show_stats
{
  uint32_t crc;
  uint16_t version;
  uint16_t MTA;
  uint16_t type;
  uint16_t pad;
  uint32_t starttime;
  uint32_t nowtime;
  uint32_t tuples;
  uint32_t ips;
  uint32_t ip_greylist;
  uint32_t ip_accept;
  uint32_t ip_reject;
  uint32_t greylisted;
  uint32_t whitelisted;
  uint32_t greylist_expired;
  uint32_t whitelist_expired;
  uint32_t requests;
  uint32_t requests_cu;
  uint32_t requests_cu_max;
  uint32_t requests_cu_ave;
  uint32_t from;
  uint32_t from_greylist;
  uint32_t from_accept;
  uint32_t from_reject;
  uint32_t fromd;
  uint32_t fromd_greylist;
  uint32_t fromd_accept;
  uint32_t fromd_reject;
  uint32_t to;
  uint32_t to_greylist;
  uint32_t to_accept;
  uint32_t to_reject;
  uint32_t tod;
  uint32_t tod_greylist;
  uint32_t tod_accept;
  uint32_t tod_reject;
  
  /*-------------------------------------------------
  ; new data for protocol v1.1
  ;-------------------------------------------------*/
  
  uint32_t tuples_read;
  uint32_t tuples_read_cu;
  uint32_t tuples_read_cu_max;
  uint32_t tuples_read_cu_ave;
  uint32_t tuples_write;
  uint32_t tuples_write_cu;
  uint32_t tuples_write_cu_max;
  uint32_t tuples_write_cu_ave;
  uint32_t tuples_low;
  uint32_t tuples_high;
};

struct glmcp_response_show_config
{
  uint32_t crc;
  uint16_t version;
  uint16_t MTA;
  uint16_t type;
  uint16_t pad;
  uint32_t max_tuples;
  uint32_t timeout_cleanup;
  uint32_t timeout_embargo;
  uint32_t timeout_grey;
  uint32_t timeout_white;
};

struct glmcp_request_iplist
{
  uint32_t crc;
  uint16_t version;
  uint16_t MTA;
  uint16_t type;
  uint16_t cmd;
  uint16_t ipsize;
  uint16_t mask;        /* the "/X" part of A.B.C.D/X */
  uint8_t  data[16];
};

struct glmcp_request_tofrom
{
  uint32_t crc;
  uint16_t version;
  uint16_t MTA;
  uint16_t type;
  uint16_t cmd;
  uint16_t size;
  uint8_t  data[1];
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
  uint8_t                           data[1500];
};

/***************************************************************/

#endif
