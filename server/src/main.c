
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

#include "../../common/src/graylist.h"

#include "graylist.h"
#include "globals.h"
#include "signals.h"
#include "util.h"
#include "server.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

/********************************************************************/

void 		 type_graylist		(struct request *);

static void	 mainloop		(int);
static char	*ipv4			(const byte *);
static void	 send_reply		(struct request *,int,int);

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

  close_stream_on_exec(ddtstream->output);

  sock = create_socket(c_host,c_port);
  
  GlobalsInit(argc,argv);
  
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
  byte                    *p;

  ddt(req != NULL);

  glr = req->glr;
  p   = glr->data;

  if ((glr->ipsize != 4) || (glr->ipsize != 16))
  {
    send_reply(req,CMD_NONE_RESP,GLERR_BAD_DATA);
    return;
  }

  glr->ipsize    = ntohl(glr->ipsize);
  glr->fromsize  = ntohl(glr->fromsize);
  glr->tosize    = ntohl(glr->tosize);

  tuple.ctime    = time(NULL);
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
  	"$ $ $",
  	"tuple: [%a , %b , %c]",
  	ipv4(tuple.ip),
  	tuple.from,
  	tuple.to
  );

  send_reply(req,CMD_GRAYLIST_RESP,GRAYLIST_YEA);
}

/*********************************************************************/

static char *ipv4(const byte *ip)
{
  static char buffer[20];
  
  ddt(ip != NULL);
  
  sprintf(buffer,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
  return(buffer);
}

/***********************************************************************/

static void send_reply(struct request *req,int type,int response)
{
#if 0
  struct graylist_response resp;
  ssize_t                  rrc;
  
  ddt(req      != NULL);
  ddt(type     >= 0);
  ddt(response >= 0);
  
  resp.version  = htons(VERSION);
  resp.MTA      = req->glr->MTA;
  resp.type     = htons(type);
  resp.respnose = htons(response);
  
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
    }
  } while (rrc == -1);
#endif
}

/********************************************************************/

