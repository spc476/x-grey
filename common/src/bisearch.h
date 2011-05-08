
#ifndef BISEARCH_H
#define BISEARCH_H

typedef struct bisearch__t
{
  void   *datum;
  size_t  idx;
} bisearch__t;

bisearch__t	bisearch	(
				  const void *const restrict,
				  const void *const restrict,
				  const size_t,
				  const size_t,
				  int (*)(const void *,const void *)
				);

#endif
