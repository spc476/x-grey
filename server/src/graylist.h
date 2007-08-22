
#ifndef GRAYLIST_H
#define GRAYLIST_H

#include <time.h>

#define SECSMIN         (60.0)
#define SECSHOUR        (60.0 * 60.0)
#define SECSDAY         (60.0 * 60.0 * 24.0)
#define SECSYEAR        (60.0 * 60.0 * 24.0 * 365.2422)

/**********************************************************/

char	*iptoa		(IP);
char	*ipptoa		(IP,Port);
char	*timetoa	(time_t);
char	*report_time	(time_t,time_t);
char	*report_delta	(double);

#endif
