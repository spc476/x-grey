
#ifndef GRAYLIST_H
#define GRAYLIST_H

#include <time.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <cgilib/stream.h>
#include <cgilib/nodelist.h>

#define SECSMIN         (60.0)
#define SECSHOUR        (60.0 * 60.0)
#define SECSDAY         (60.0 * 60.0 * 24.0)
#define SECSYEAR        (60.0 * 60.0 * 24.0 * 365.2422)

/********************************************************************/

typedef unsigned char  byte;
typedef unsigned long  IP;
typedef unsigned short Port;

typedef struct tuple
{
  time_t ctime;
  time_t atime;
  size_t fsize;
  size_t tsize;
  int    white;
  byte   ip  [16];
  char   from[128];
  char   to  [128];
} *Tuple;

typedef struct pollnode
{
  Node node;
  void (*fn)(struct epoll_event *);
} *PollNode;

typedef struct listen_node
{
  struct pollnode node;
  int             listen;
} *ListenNode;

typedef struct socket_node
{
  struct pollnode    node;
  int                conn;
  struct sockaddr_in remote;
  Stream             in;
  Stream             out;
} *SocketNode;

/***************************************************************/

char	*iptoa		(IP);
char	*ipptoa		(IP,Port);
char	*timetoa	(time_t);
char	*report_time	(time_t,time_t);
char	*report_delta	(double);

#endif
