
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>

#include "globals.h"
#include "signals.h"
#include "util.h"

/********************************************************************/

static void	mainloop		(void);
static void	event_listen		(struct epoll_event *);
static void	event_socket		(struct epoll_event *);
static void	event_socket_remove	(struct epoll_event *);

/********************************************************************/

int main(int argc,char *argv[])
{
  ListenNode         lnode;
  struct epoll_event ev;
  int                rc;
  
  MemInit();
  DdtInit();
  StreamInit();

  /*----------------------------------------------------
  ; A bit of a hack to set the close on exec flags for
  ; various streams opened up by the CGILIB.
  ;----------------------------------------------------*/

  close_stream_on_exec(ddtstream->output);

#if 0
  /*---------------------------------------------------------
  ; these I either need to keep open ; across an exec, when 
  ; running in foreground mode, or they're explicitely
  ; closed when going into daemon mode, so these are no longer
  ; needed.
  ;--------------------------------------------------------*/

  close_stream_on_exec(StdinStream);
  close_stream_on_exec(StdoutStream);
  close_stream_on_exec(StderrStream);
#endif

  GlobalsInit(argc,argv);
  
  lnode          = MemAlloc(sizeof(struct listen_node));
  lnode->node.fn = event_listen;
  lnode->listen  = create_socket(c_host,c_pfport);
  
  ev.events   = EPOLLIN;
  ev.data.ptr = lnode;
  
  rc = epoll_ctl(g_queue,EPOLL_CTL_ADD,lnode->listen,&ev);
  if (rc == -1)
  {
    (*cv_report)(LOG_DEBUG,"$","epoll_ctl(add listen) = %a",strerror(errno));
    return(EXIT_FAILURE);
  }

  mainloop();
  
  return(EXIT_SUCCESS);
}

/********************************************************************/

static void mainloop(void)
{
  struct epoll_event *list;
  int                 events;
  int                 i;
  PollNode            node;
  
  list = MemAlloc(sizeof(struct epoll_event) * c_pollqueue);
  
  while(1)
  {
    check_signals();
    events = epoll_wait(g_queue,list,c_pollqueue,60);

    if (events == -1)
      continue;
  
    for (i = 0 ; i < events ; i++)
    {
      node = list[i].data.ptr;
      (*node->fn)(&list[i]);
    }
  }
}  

/*******************************************************************/

static void event_listen(struct epoll_event *pev)
{
  ListenNode         data;
  SocketNode         snode;
  struct sockaddr_in sin;
  struct epoll_event ev;
  socklen_t          length;
  int                conn;
  int                rc;
  
  ddt(pev != NULL);
  
  data   = pev->data.ptr;
  length = sizeof(sin);
  conn   = accept(data->listen,(struct sockaddr *)&sin,&length);
  
  if (conn < 0)
  {
    (*cv_report)(LOG_DEBUG,"$","accept() = %a",strerror(errno));
    return;
  }

  close_on_exec(conn);
  
  snode          = MemAlloc(sizeof(struct socket_node));
  snode->node.fn = event_socket;
  snode->remote  = sin;
  snode->conn    = conn;
  snode->in      = FHStreamRead(conn);
  snode->out     = FHStreamWrite(conn);
  
  ev.events   = EPOLLIN | EPOLLHUP;
  ev.data.ptr = snode;
  
  rc = epoll_ctl(g_queue,EPOLL_CTL_ADD,conn,&ev);
  if (rc == -1)
  {
    (*cv_report)(LOG_DEBUG,"$","epoll_ctl(add) = %a",strerror(errno));
    return;
  }
}

/*******************************************************************/

static void event_socket(struct epoll_event *pev)
{
  SocketNode data;
  char       *input;
  
  ddt(pev != NULL);
  
  data = pev->data.ptr;
  
  if ((pev->events & EPOLLHUP) == EPOLLHUP)
  {
    event_socket_remove(pev);
    return;
  }
  
  input = LineSRead(data->in);
  LineS(data->out,input);
  StreamWrite(data->out,'\n');
  up_string(input);
  if (strcmp(input,"QUIT") == 0)
    event_socket_remove(pev);
  else if (strcmp(input,"CRASH") == 0)
  {
    char *crashme = NULL;
    
    *crashme = '\0';
  }
}  

/***************************************************************/

static void event_socket_remove(struct epoll_event *pev)
{
  SocketNode data;
  int        rc;
  
  ddt(pev != NULL);
  
  data = pev->data.ptr;
  (*cv_report)(LOG_DEBUG,"","event_socket_remove()");
  
  pev->events = EPOLLIN | EPOLLHUP;
  rc = epoll_ctl(g_queue,EPOLL_CTL_DEL,data->conn,pev);
  if (rc == -1)
  {
    (*cv_report)(LOG_ERR,"$","epoll_ctl(delete) = %a",strerror(errno));
    return;
  }
  
  StreamFree(data->out);
  StreamFree(data->in);
  close(data->conn);
  MemFree(data);
}

/**********************************************************************/

