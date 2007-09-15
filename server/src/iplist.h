
#ifndef IPLIST_H
#define IPLIST_H

#include <cgilib/stream.h>
#include "../../common/src/graylist.h"

enum
{
  IPCMD_ACCEPT,
  IPCMD_REJECT,
  IPCMD_DUNNO
};

struct ipblock
{
  size_t size;
  byte   addr[16];
  byte   mask[16];
  int    cmd;
};

/******************************************************/

int	iplist_read		(const char *);
int	iplist_check		(byte *,size_t);
int	iplist_dump_stream	(Stream);
int	whitelist_dump		(void);
int	whitelist_dump_stream	(Stream);
int	tuple_dump		(void);
int	tuple_dump_stream	(Stream);
int	whitelist_load		(void);

#endif
