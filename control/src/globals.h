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

#ifndef CONTROL_GLOBALS_H
#define CONTROL_GLOBALS_H

/**************************************************************/

extern const char *const	  c_license;
extern const char *const	  c_commands;
extern const char *const          c_host;
extern const int                  c_port;
extern const char *const          c_rhost;
extern const int                  c_rport;
extern const int                  c_timeout;
extern const struct sockaddr_in   c_raddr;
extern const socklen_t            c_raddrsize;
extern const int                  c_log_facility;
extern const int                  c_log_levels;
extern const char *const          c_log_id;
extern const int                  cf_debug;
extern const char *const          c_timeformat;
extern const char *const          c_pager;
extern const char *const          c_secret;
extern const size_t               c_secretsize;
extern void                     (*cv_report)(int,const char *, ... );
extern void                     (*cv_pager) (int);

/*************************************************************/

int	(GlobalsInit)		(int,char *[]);
int	(GlobalsDeinit)		(void);

void	 pager_batch		(int);
void	 pager_interactive	(int);

/************************************************************/

#endif
