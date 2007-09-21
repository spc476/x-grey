
#ifndef EMAIL_DOMAIN_H
#define EMAIL_DOMAIN_H

#include <stdlib.h>

#include <cgilib/stream.h>

/******************************************************************/

typedef struct emaildomain
{
  char   *text;
  size_t  tsize;
  size_t  count;
  int     cmd;
} *EDomain;

/******************************************************************/

int		edomain_cmp		(EDomain,EDomain);
EDomain		edomain_search		(EDomain,size_t *,EDomain,size_t);
void		edomain_add_from	(EDomain,size_t);
void		edomain_add_fromd	(EDomain,size_t);
void		edomain_add_to		(EDomain,size_t);
void		edomain_add_tod		(EDomain,size_t);
void		to_dump			(void);
void		tod_dump		(void);
void		from_dump		(void);
void		fromd_dump		(void);
void		to_dump_stream		(Stream);
void		tod_dump_stream		(Stream);
void		from_dump_stream	(Stream);
void		fromd_dump_stream	(Stream);
		
#endif

