
#ifndef GLOBALS_H
#define GLOBALS_H

#include <syslog.h>

/***********************************************************/

extern const int         c_log_facility;
extern const int         c_log_level;
extern const char *const c_log_id;
extern const int         cf_debug;

/***********************************************************/

int	(GlobalsInit)	(int,char *[]);
int	(GlobalsDeinit)	(void);
void	report_syslog	(int,char *,char *, ... );

#endif
