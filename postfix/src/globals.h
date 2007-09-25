
#ifndef GLOBALS_H
#define GLOBALS_H

#include <syslog.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/***********************************************************/

extern const char *const          c_host;
extern const int                  c_port;
extern const char *const          c_rhost;
extern const int                  c_rport;
extern const struct sockaddr_in   c_raddr;
extern const socklen_t            c_raddrsize;
extern const int                  c_timeout;
extern const int                  c_log_facility;
extern const int                  c_log_level;
extern const char *const          c_log_id;
extern const int                  cf_debug;
extern const char *const          c_secret;
extern const size_t               c_secretsize;
extern void                     (*cv_report)(int,char *,char *,...);

/***********************************************************/

int	(GlobalsInit)	(int,char *[]);
int	(GlobalsDeinit)	(void);
void	report_syslog	(int,char *,char *, ... );

#endif
