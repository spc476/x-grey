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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>

#include "tuple.h"
#include "iplist.h"
#include "emaildomain.h"

	/*----------------------------------------------*/

extern const char *const    c_pidfile;	
extern const char *const    c_whitefile;
extern const char *const    c_greyfile;
extern const char *const    c_dumpfile;
extern const char *const    c_tofile;
extern const char *const    c_todfile;
extern const char *const    c_fromfile;
extern const char *const    c_fromdfile;
extern const char *const    c_iplistfile;
extern const char *const    c_timeformat;
extern const char *const    c_host;
extern const int            c_port;
extern const size_t         c_poolmax;
extern const unsigned int   c_time_cleanup;
extern const double         c_time_savestate;
extern const double         c_timeout_embargo;
extern const double         c_timeout_grey;
extern const double         c_timeout_white;
extern const int            c_facility;
extern const int            c_level;
extern const char *const    c_sysid;
extern const time_t         c_starttime;
extern const char *const    c_secret;
extern const size_t         c_secretsize;
extern const bool           cf_debug;
extern const bool           cf_foreground;
extern const bool           cf_oldcounts;
extern const bool           cf_nomonitor;

	/*---------------------------------------------*/
	
extern size_t              g_poolnum;
extern struct tuple       *g_pool;
extern Tuple              *g_tuplespace;

extern char                g_argv0[FILENAME_MAX];
extern char              **g_argv;

extern size_t		   g_requests;
extern size_t              g_req_cu;
extern size_t              g_req_cucurrent;
extern size_t		   g_req_cumax;
extern size_t		   g_cleanup_count;
extern size_t	           g_greylisted;
extern size_t	           g_whitelisted;
extern size_t	           g_whitelist_expired;
extern size_t	           g_greylist_expired;
extern size_t		   g_tuples_read;
extern size_t              g_tuples_read_cu;
extern size_t              g_tuples_read_cucurrent;
extern size_t              g_tuples_read_cumax;
extern size_t		   g_tuples_write;
extern size_t              g_tuples_write_cu;
extern size_t		   g_tuples_write_cucurrent;
extern size_t              g_tuples_write_cumax;
extern size_t              g_tuples_low;
extern size_t              g_tuples_high;

extern struct ipnode      *g_tree;
extern size_t              g_ipcnt;
extern size_t              g_ip_cmdcnt[3];

extern size_t              g_smaxfrom;
extern size_t              g_sfrom;
extern struct emaildomain *g_from;
extern size_t              g_from_cmdcnt[3];

extern size_t              g_smaxfromd;
extern size_t              g_sfromd;
extern struct emaildomain *g_fromd;
extern size_t              g_fromd_cmdcnt[3];

extern size_t              g_smaxto;
extern size_t              g_sto;
extern struct emaildomain *g_to;
extern size_t              g_to_cmdcnt[3];

extern size_t              g_smaxtod;
extern size_t              g_stod;
extern struct emaildomain *g_tod;
extern size_t              g_tod_cmdcnt[3];

extern time_t              g_time_savestate;

extern int                 g_defto;
extern int                 g_deftodomain;
extern int                 g_deffrom;
extern int                 g_deffromdomain;
extern size_t              g_toc;
extern size_t              g_todomainc;
extern size_t              g_fromc;
extern size_t              g_fromdomainc;

/*****************************************************************/

int	(GlobalsInit)	(void);
int	(GlobalsDeinit)	(void);

void	parse_cmdline	(int,char *[]);
void	daemon_init	(void);

#endif

