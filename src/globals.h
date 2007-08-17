/************************************************************************
*
* Copyright 2007 by Sean Conner.  All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*************************************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <signal.h>
#include <cgilib/stream.h>
#include "graylist.h"

extern const char *const   c_whitefile;
extern const char *const   c_grayfile;
extern const char *const   c_timeformat;
extern const char *const   c_host;
extern const int           c_pfport;
extern const int           c_smport;
extern const size_t        c_poolmax;
extern const unsigned int  c_timeout_cleanup;
extern const double	   c_timeout_accept;
extern const double        c_timeout_gray;
extern const double        c_timeout_white;
extern const int           c_facility;
extern const int           c_level;
extern const char *const   c_sysid;
extern const int           cf_debug;
extern const int           cf_foreground;
extern void              (*cv_report)(int,char *,char *, ... );

extern size_t              g_poolnum;
extern Tuple              *g_pool;

/****************************************************************/

int	(GlobalInit)	(int,char *[]);
int	(GlobalDeinit)	(void);

/***************************************************************/

#endif

