
#ifndef GRAYLIST_H
#define GRAYLIST_H

#include <time.h>
#include "../../common/src/graylist.h"

/**********************************************************/

int	 tuple_cmp		(const void *,const void *);
int	 tuple_qsort_cmp	(const void *,const void *);
Tuple	 tuple_search		(Tuple,size_t *);
Tuple	 tuple_allocate		(void);
void	 tuple_add		(Tuple,size_t);
void	 tuple_expire		(time_t);

#endif

