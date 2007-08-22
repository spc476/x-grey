
#ifndef E_GLOBALS_COMMON_H
#define E_GLOBALS_COMMON_H

extern const char *const c_host;
extern const int         c_port;
extern const int         c_log_facility;
extern const int         c_log_level;
extern const char *const c_log_id;
extern const int         cf_debug;
extern void            (*cv_report)(int,char *,char *, ... );

#endif

