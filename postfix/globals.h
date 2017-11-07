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

#include <syslog.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/***********************************************************/

extern char               const *const c_host;
extern int                const        c_port;
extern char               const *const c_rhost;
extern int                const        c_rport;
extern struct sockaddr_in const        c_raddr;
extern socklen_t          const        c_raddrsize;
extern int                const        c_timeout;
extern int                const        c_log_facility;
extern int                const        c_log_level;
extern char               const *const c_log_id;
extern bool               const        cf_debug;
extern char               const *const c_secret;
extern size_t             const        c_secretsize;

/***********************************************************/

int	(GlobalsInit)	(int,char *[]);
int	(GlobalsDeinit)	(void);
void	report_syslog	(int,char const *, ... );

#endif
