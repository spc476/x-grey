
#ifndef DK_UTIL_H
#define DK_UTIL_H

int	create_socket		(const char *,int);
void	close_on_exec		(int);
void	close_stream_on_exec	(Stream);

#endif

