
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
int		to_dump			(void);
int		tod_dump		(void);
int		from_dump		(void);
int		fromd_dump		(void);
int		to_dump_stream		(Stream);
int		tod_dump_stream		(Stream);
int		from_dump_stream	(Stream);
int		fromd_dump_stream	(Stream);
		
#endif

