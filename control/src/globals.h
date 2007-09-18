
#ifndef CONTROL_GLOBALS_H
#define CONTROL_GLOBALS_H

/**************************************************************/

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
extern void                     (*cv_report)(int,char *,char *, ... );

/*************************************************************/

int	(GlobalsInit)	(int,char *[]);
int	(GlobalsDeinit)	(void);

/************************************************************/

#endif
