
#ifndef DK_UTIL_H
#define DK_UTIL_H

#include <cgilib/stream.h>

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

/***************************************************************/

#endif

