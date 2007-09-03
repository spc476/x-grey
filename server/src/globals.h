
#ifndef GLOBALS_H
#define GLOBALS_H

#include "graylist.h"

	/*----------------------------------------------*/
	
extern const char *const    c_whitefile;
extern const char *const    c_grayfile;
extern const char *const    c_timeformat;
extern const char *const    c_host;
extern const int            c_port;
extern const size_t         c_poolmax;
extern const unsigned int   c_timeout_cleanup;
extern const double         c_timeout_accept;
extern const double         c_timeout_gray;
extern const double         c_timeout_white;
extern const int            c_facility;
extern const int            c_level;
extern const char *const    c_sysid;
extern const time_t         c_starttime;
extern const int            cf_debug;
extern const int            cf_foreground;
extern void               (*cv_report)(int,char *,char *, ... );

	/*---------------------------------------------*/
	
extern size_t               g_unique;
extern size_t               g_uniquepassed;
extern size_t               g_pollnum;
extern const struct tuple  *g_tuplespace;
extern const Tuple         *g_pool;
extern const int          **g_argv;

extern size_t               g_graycount_waiting;
extern size_t               g_graycount_expired;
extern size_t               g_whitecount_waiting;
extern size_t               g_whitecount_expired;

/*****************************************************************/

int	(GlobalsInit)	(int,char *[]);
int	(GlobalsDeinit)	(void);

#endif

