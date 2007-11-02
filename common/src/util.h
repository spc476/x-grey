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

#ifndef DK_UTIL_H
#define DK_UTIL_H

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <cgilib/stream.h>

#include "graylist.h"

#define SECSMIN		(60.0)
#define SECSHOUR	(60.0 * 60.0)
#define SECSDAY		(60.0 * 60.0 * 24.0)
#define SECSYEAR	(60.0 * 60.0 * 24.0 * 365.2422)

struct chars_int
{
  const char *name;
  const int   value;
};

typedef struct mystring
{
  size_t  s;
  char   *d;
} String;

/***************************************************************/

int	    create_socket		(const char *,int,int);
int	    ci_map_int			(const char *,const struct chars_int [],size_t);
const char *ci_map_chars		(int,const struct chars_int [],size_t);
void	    report_syslog		(int,char *,char *, ... );
void	    report_stderr		(int,char *,char *, ... );
void	    log_address			(struct sockaddr *);
char	   *ipv4			(const byte *);
void	    set_signal			(int,void (*)(int));
double	    read_dtime			(char *);

char       *iptoa			(IP);
char       *ipptoa			(IP,Port);
char       *timetoa			(time_t);
char       *report_time			(time_t,time_t);
char       *report_delta		(double);

String	   *split			(size_t *,char *);

void	    write_pidfile		(const char *);
int	    parse_ip			(byte *,int *,char *);

/***************************************************************/

#endif

