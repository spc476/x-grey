
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

  GlobalsInit(argc,argv);
  
  {
    char *t = timetoa(c_starttime);
    (*cv_report)(LOG_INFO,"$","start time: %a",t);
    MemFree(t);
  }
  
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
  snode->bufsiz  = 0;
  snode->from    = dup_string("");
  snode->to      = dup_string("");
  snode->ip      = dup_string("");
  
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
  char       *buffer;
  char       *p;
  char       *v;
  size_t      amount;
  ssize_t     rrc;
  
  ddt(pev != NULL);
  
  data = pev->data.ptr;
  
  if ((pev->events & EPOLLHUP) == EPOLLHUP)
  {
    event_socket_remove(pev);
    return;
  }
  
  buffer = &data->buffer[data->bufsiz];
  rrc    = read(data->conn,buffer,sizeof(data->buffer) - data->bufsiz);
  
  if (rrc == -1)
  {
    if (errno != EINTR)
      (*cv_report)(LOG_WARNING,"$","read() = %a",strerror(errno));
    return;
  }

#if 0  
  if (rrc == 0)
  {
    /* is this normal? */
    return;
  }
#endif

  data->bufsiz += rrc;
  
  p = memchr(buffer,'\n',rrc);
  
  if (p == NULL)
  {
    if (data->bufsiz == BUFSIZ)
    {
      (*cv_report)(LOG_WARNING,"","buffer overflow, aborting connection");
      event_socket_remove(pev);
    }
    return;
  }
  
  if (p == data->buffer)
  {
    /*-----------------------------------
    ; we're done collecting the request,
    ; now process it.
    ;-----------------------------------*/
    (*cv_report)(LOG_INFO,"$ $ $","process [ %a , %b , %c ]",data->ip,data->from,data->to);
    write(data->conn,"action=dunno\n\n",14);
    event_socket_remove(pev);
    return;
  }
    
  v = memchr(data->buffer,'=',data->bufsiz);
  if (v == NULL)
  {
    (*cv_report)(LOG_WARNING,"","invalid format, aborting connection");
    event_socket_remove(pev);
    return;
  }
  
  *p++ = '\0';	/* points to remains of buffer */
  *v++ = '\0';	/* points to value portion */
  
  (*cv_report)(LOG_DEBUG,"$ $","read in %a = %b",data->buffer,v);
  
  if (strcmp(data->buffer,"request") == 0)
    data->request = dup_string(v);
  else if (strcmp(data->buffer,"sender") == 0)
    data->from = dup_string(v);
  else if (strcmp(data->buffer,"recipient") == 0)
    data->to = dup_string(v);
  else if (strcmp(data->buffer,"client_address") == 0)
    data->ip = dup_string(v);

  amount = data->bufsiz - (size_t)(p - data->buffer);
  memmove(data->buffer,p,amount);
  data->bufsiz = amount;
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
  
  MemFree(data->ip);
  MemFree(data->to);
  MemFree(data->from);
  close(data->conn);
  MemFree(data);
}

/**********************************************************************/

