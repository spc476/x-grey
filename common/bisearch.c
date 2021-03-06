/***************************************************************************
*
* Copyright 2011 by Sean Conner.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*************************************************************************/

#include <stddef.h>
#include <assert.h>

#include "bisearch.h"

/*************************************************************************/

bisearch__t bisearch(
        void   const *const restrict key,
        void   const *const restrict base,
        size_t const                 nelem,
        size_t const                 size,
        int (*compare)(void const *restrict,void const *restrict)
)
{
  size_t    first;
  size_t    len;
  
  assert(key  != NULL);
  assert(size >  0);
  
  first = 0;
  len   = nelem;
  
  while(len > 0)
  {
    char const *pivot;
    size_t      half;
    size_t      middle;
    int         q;
    
    assert(base != NULL);
    
    half   = len / 2;
    middle = first + half;
    pivot  = (char const *)base + (middle * size);
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

