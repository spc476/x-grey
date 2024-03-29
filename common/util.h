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

#ifndef DK_UTIL_H
#define DK_UTIL_H

#include <stdbool.h>
#include <assert.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "greylist.h"

#define SECSMIN         (60.0)
#define SECSHOUR        (60.0 * 60.0)
#define SECSDAY         (60.0 * 60.0 * 24.0)
#define SECSYEAR        (60.0 * 60.0 * 24.0 * 365.242199)

#define min(a,b)        ((a) < (b)) ? (a) : (b)
#define max(a,b)        ((a) > (b)) ? (a) : (b)

struct chars_int
{
  char const *name;
  int  const  value;
};

typedef struct mystring
{
  size_t  s;
  char   *d;
} String;

/***************************************************************/

extern int         create_socket (char const *,int,int);
extern int         ci_map_int    (char const *,struct chars_int const [],size_t);
extern char const *ci_map_chars  (int,struct chars_int const [],size_t);
extern void        report_syslog (int,char const *, ... );
extern void        report_stderr (int,char const *, ... );
extern void        log_address   (struct sockaddr *);
extern char       *ipv4          (uint8_t const *);
extern int         set_signal    (int,void (*)(int));
extern double      read_dtime    (char *,double);
extern size_t      read_size     (char *);

extern char       *iptoa         (IP);
extern char       *ipptoa        (IP,Port);
extern char       *timetoa       (time_t);
extern char       *report_time   (time_t,time_t);
extern char       *report_delta  (double);

extern String     *split         (size_t *,char *);

extern void        write_pidfile (char const *);
extern int         parse_ip      (uint8_t *,int *,char *);

/***************************************************************/

#endif
