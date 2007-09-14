
#ifndef SERVER_H
#define SERVER_H

#include <time.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../../common/src/graylist.h"

/************************************************************/

#define UDP_MAX		1500

#define F_WHITELIST     (1uL << 0)
#define F_GRAYLIST      (1uL << 1)
#define F_TRUNCFROM     (1uL << 2)
#define F_TRUNCTO       (1uL << 3)
#define F_IPv6          (1uL << 4)

/************************************************************/

typedef struct tuple
{
  time_t       ctime;
  time_t       atime;
  size_t       fromsize;
  size_t       tosize;
  unsigned int pad;
  flags        f;
  byte         ip  [16];
  char         from[108];
  char         to  [108];
} *Tuple;

struct request
{
  int                      sock;
  time_t                   now;
  struct sockaddr          remote;
  socklen_t                rsize;
  char                     packet[1500];
  struct graylist_request *glr;
  size_t                   size;
};

#endif


  
  
