
#include <stdlib.h>
#include <errno.h>
#include <string.h>

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

#include "../../common/src/graylist.h"
#include "../../common/src/util.h"

#include "graylist.h"
#include "globals.h"
#include "signals.h"
#include "util.h"
#include "server.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

/********************************************************************/

void 		 type_graylist		(struct request *);

static void	 mainloop		(int);
static void	 send_reply		(struct request *,int,int);
static void 	 my_exit		(void);

/********************************************************************/

int main(int argc,char *argv[])
{
  int sock;
  
  MemInit();
  DdtInit();
  StreamInit();

  /*----------------------------------------------------
  ; A bit of a hack to set the close on exec flags for
  ; various streams opened up by the CGILIB.
  ;----------------------------------------------------*/

  atexit(my_exit);
  close_stream_on_exec(ddtstream->output);
  GlobalsInit(argc,argv);

  sock = create_socket(c_host,c_port);
  
  {
    char *t = timetoa(c_starttime);
    (*cv_report)(LOG_INFO,"$","start time: %a",t);
    MemFree(t);
  }

  mainloop(sock);
  
  return(EXIT_SUCCESS);
}

/********************************************************************/

static void mainloop(int sock)
{
  struct request req;
  ssize_t        rrc;
  
  while(1)
  {
    check_signals();

    memset(&req.remote,0,sizeof(req.remote));
    req.rsize = sizeof(req.remote);

    rrc = recvfrom(
    	sock,
    	req.packet,
    	sizeof(req.packet),
    	0,
    	&req.remote,
    	&req.rsize
    );
    
    if (rrc == -1)
    {
      if (errno != EINTR)
        (*cv_report)(LOG_ERR,"$","recvfrom() = %a",strerror(errno));
      continue;
    }
    
    req.now  = time(NULL);
    req.sock = sock;
    req.glr  = (struct graylist_request *)req.packet;
    req.size = rrc;
    
    if (ntohs(req.glr->version) != VERSION)
    {
      send_reply(&req,CMD_NONE_RESP,GLERR_VERSION_NOT_SUPPORTED);
      continue;
    }
    
    switch(ntohs(req.glr->type))
    {
      case CMD_NONE:
           send_reply(&req,CMD_NONE_RESP,GLERR_OKAY);
           break;
      case CMD_GRAYLIST:
           type_graylist(&req);
           break;
      default:
           send_reply(&req,CMD_NONE_RESP,GLERR_TYPE_NOT_SUPPORTED);
           break;
    }
  }
}

/*******************************************************************/

void type_graylist(struct request *req)
{
  struct graylist_request *glr;
  struct tuple             tuple;
  Tuple                    stored;
  size_t                   rsize;
  size_t                   idx;
  byte                    *p;

  ddt(req != NULL);

  /*---------------------------------------------------
  ; sanitize the request, and form a tuple from it.
  ;---------------------------------------------------*/
  
  glr = req->glr;
  p   = glr->data;

  glr->ipsize    = ntohs(glr->ipsize);
  glr->fromsize  = ntohs(glr->fromsize);
  glr->tosize    = ntohs(glr->tosize);

  if ((glr->ipsize != 4) && (glr->ipsize != 16))
  {
    send_reply(req,CMD_NONE_RESP,GLERR_BAD_DATA);
    return;
  }

  rsize = glr->ipsize 
        + glr->fromsize 
	+ glr->tosize 
	+ sizeof(struct graylist_request) 
	- 2;	/* empiracally found --- this is bad XXX */

  if (rsize > req->size)
  {
    send_reply(req,CMD_NONE_RESP,GLERR_BAD_DATA);
    return;
  }
  
  memset(&tuple,0,sizeof(struct tuple));
  D(tuple.pad    = 0xDECAFBAD;)
  tuple.ctime    = tuple.atime = req->now;
  tuple.fromsize = min(sizeof(tuple.from) - 1,glr->fromsize);
  tuple.tosize   = min(sizeof(tuple.to)   - 1,glr->tosize);
  tuple.f        = 0;

  memcpy(tuple.ip,  p,glr->ipsize);    p += glr->ipsize;
  memcpy(tuple.from,p,tuple.fromsize); p += glr->fromsize;
  memcpy(tuple.to,  p,tuple.tosize);

  tuple.from[tuple.fromsize] = '\0';
  tuple.to  [tuple.tosize]   = '\0';

  if (tuple.fromsize <  glr->fromsize) tuple.f |= F_TRUNCFROM;
  if (tuple.tosize   <  glr->tosize)   tuple.f |= F_TRUNCTO;
  if (glr->ipsize    == 16)            tuple.f |= F_IPv6;

  (*cv_report)(
  	LOG_INFO,
  	"$ $ $ $ $",
  	"tuple: [%a , %b , %c]%d%e",
  	ipv4(tuple.ip),
  	tuple.from,
  	tuple.to,
  	(tuple.f & F_TRUNCFROM) ? " Tf" : "",
  	(tuple.f & F_TRUNCTO)   ? " Tt" : ""
  );

  /*------------------------------------------------
  ; at some point, we'll add a check for sender IP
  ; and see if we can accept/reject it at this 
  ; point, otherwise, we do the tuple check.
  ;-----------------------------------------------*/
  
  /*-------------------------------------------------------
  ; second half of routine---look up the tuple.  If not found,
  ; add it, else update the access time, and if less than the
  ; embargo period, return LATER, else accept.
  ;---------------------------------------------------------*/
  
  stored = tuple_search(&tuple,&idx);
  
  if (stored == NULL)
  {
    stored  = tuple_allocate();
    memcpy(stored,&tuple,sizeof(struct tuple));
    tuple_add(stored,idx);
    g_graylisted++;
    send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_LATER);
    return;
  }
  
  if (difftime(req->now,stored->atime) < c_timeout_embargo)
  {
    stored->atime = req->now;
    send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_LATER);
    return;
  }
  
  stored->atime = req->now;
  
  if ((stored->f & F_WHITELIST) == 0)
  {
    stored->f |= F_WHITELIST;
    g_whitelisted++;
  }
    
  send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
}

/*********************************************************************/

static void send_reply(struct request *req,int type,int response)
{
  struct graylist_response resp;
  ssize_t                  rrc;
  
  ddt(req      != NULL);
  ddt(type     >= 0);
  ddt(response >= 0);
  
  resp.version  = htons(VERSION);
  resp.MTA      = req->glr->MTA;
  resp.type     = htons(type);
  resp.response = htons(response);
  
  do
  {
    rrc = sendto(
  		req->sock,
  		&resp,
  		sizeof(struct graylist_response),
  		0,
  		&req->remote,
  		req->rsize
  	);
  
    if (rrc == -1)
    {
      if (errno == EINTR)
        continue;
      (*cv_report)(LOG_ERR,"$","sendto() = %a",strerror(errno));
      if (errno == EINVAL)
        break;
      sleep(1);
    }
  } while (rrc == -1);
}

/********************************************************************/

static void my_exit(void)
{
  StreamFlush(StdoutStream);
  StreamFlush(StderrStream);
}

/**********************************************************************/

