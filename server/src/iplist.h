
#ifndef IPLIST_H
#define IPLIST_H

#include <stddef.h>
#include <cgilib/stream.h>
#include "../../common/src/graylist.h"

struct ipblock
{
  size_t size;
  byte   addr[16];
  byte   mask[16];
  int    smask;
  size_t count;
  int    cmd;
};

struct ipnode
{
  struct ipnode *parent;
  struct ipnode *zero;
  struct ipnode *one;
  size_t         count;
  int            match;
};

/******************************************************/

int		 iplist_read		(const char *);
int		 iplist_cmp		(const void *,const void *);
int		 iplist_check		(byte *,size_t);
int		 iplist_dump		(void);
int		 iplist_dump_stream	(Stream);
struct ipblock	*ip_table		(size_t *);
int		 ip_match		(byte *,size_t);
int		 ip_add_sm		(byte *,size_t,int,int);
int		 ip_print		(Stream);

#endif
