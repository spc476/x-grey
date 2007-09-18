
#ifndef IPTABLE_H
#define IPTABLE_H

#include <stddef.h>
#include "../../common/src/graylist.h"

struct ipnode
{
  struct ipnode *parent;
  struct ipnode *zero;
  struct ipnode *one;
  size_t         count;
  int            match;
};

struct ipblock	*ip_table	(size_t *);
int		 ip_match	(byte *,size_t);
int		 ip_add_sm	(byte *,size_t,int,int);
int		 ip_print	(Stream);

#endif

