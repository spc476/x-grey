
#ifndef GRAYLIST_H
#define GRAYLIST_H

#include <time.h>
#include "../../common/src/graylist.h"

#define SECSMIN         (60.0)
#define SECSHOUR        (60.0 * 60.0)
#define SECSDAY         (60.0 * 60.0 * 24.0)
#define SECSYEAR        (60.0 * 60.0 * 24.0 * 365.2422)

/**********************************************************/

int	 tuple_cmp		(const void *,const void *);
int	 tuple_qsort_cmp	(const void *,const void *);
Tuple	 tuple_search		(Tuple,size_t *);
Tuple	 tuple_allocate		(void);
void	 tuple_add		(Tuple,size_t);
void	 tuple_expire		(time_t);

char	*iptoa			(IP);
char	*ipptoa			(IP,Port);
char	*timetoa		(time_t);
char	*report_time		(time_t,time_t);
char	*report_delta		(double);

#endif
