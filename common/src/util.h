
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

/***************************************************************/

int	    create_socket		(const char *,int);
void	    close_on_exec		(int);
void	    close_stream_on_exec	(Stream);
int	    ci_map_int			(const char *,const struct chars_int *,size_t);
const char *ci_map_chars		(int,const struct chars_int *,size_t);
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

/***************************************************************/

#endif

