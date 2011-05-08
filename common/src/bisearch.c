
#include <stddef.h>
#include <assert.h>

#include "bisearch.h"

/*************************************************************************/

bisearch__t bisearch(
	const void *const restrict key,
	const void *const restrict base,
	const size_t               nelem,
	const size_t               size,
	int (*compare)(const void *restrict,const void *restrict)
)
{
  size_t    first;
  size_t    len;
  
  assert(key  != NULL);
  assert(base != NULL);
  assert(size >  0);
  
  first = 0;
  len   = nelem;
  
  while(len > 0)
  {
    const char *pivot;
    size_t      half;
    size_t      middle;
    int         q;
    
    half   = len / 2;
    middle = first + half;
    pivot  = (const char *)base + (size * middle);
    q      = (*compare)(key,pivot);
    
    if (q > 0)
    {
      first = middle + 1;
      len   = len - half - 1;
    }
    else if (q == 0)
      return (bisearch__t){ .datum = (void *)pivot , .idx = middle };
    else
      len = half;
  }
  
  return (bisearch__t){ .datum = NULL , .idx = first };
}

/**************************************************************************/

