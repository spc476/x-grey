
#ifndef GLOBALS_H
#define GLOBALS_H

#include "tuple.h"
#include "iplist.h"
#include "emaildomain.h"

	/*----------------------------------------------*/

extern const char *const    c_pidfile;	
extern const char *const    c_whitefile;
extern const char *const    c_grayfile;
extern const char *const    c_dumpfile;
extern const char *const    c_tofile;
extern const char *const    c_todfile;
extern const char *const    c_fromfile;
extern const char *const    c_fromdfile;
extern const char *const    c_iplistfile;
extern const char *const    c_timeformat;
extern const char *const    c_host;
extern const int            c_port;
extern const size_t         c_poolmax;
extern const unsigned int   c_time_cleanup;
extern const double         c_time_savestate;
extern const double         c_timeout_embargo;
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
	
extern size_t              g_poolnum;
extern struct tuple       *g_pool;
extern Tuple              *g_tuplespace;
extern int               **g_argv;

extern size_t	           g_graylisted;
extern size_t	           g_whitelisted;
extern size_t	           g_whitelist_expired;
extern size_t	           g_graylist_expired;

extern struct ipnode      *g_tree;
extern size_t              g_ipcnt;

extern size_t              g_smaxfrom;
extern size_t              g_sfrom;
extern struct emaildomain *g_from;
extern size_t              g_smaxfromd;
extern size_t              g_sfromd;
extern struct emaildomain *g_fromd;
extern size_t              g_smaxto;
extern size_t              g_sto;
extern struct emaildomain *g_to;
extern size_t              g_smaxtod;
extern size_t              g_stod;
extern struct emaildomain *g_tod;

extern time_t              g_time_savestate;

/*****************************************************************/

int	(GlobalsInit)	(int,char *[]);
int	(GlobalsDeinit)	(void);

#endif

