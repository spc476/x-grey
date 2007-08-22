
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>

#include <cgilib/ddt.h>

#include "globals.h"
#include "util.h"

/*****************************************************************/

int create_socket(const char *host,int port)
{
  struct sockaddr_in  sin;
  struct hostent     *localhost;
  int                 s;
  int                 rc;
  int                 reuse = 1;

  ddt(host != NULL);
  ddt(port >= 0);
  ddt(port <= 65535);
  
  localhost = gethostbyname(host);
  if (localhost == NULL)
  {
    (*cv_report)(LOG_ERR,"$ $","gethostbyname(%a) = %b",c_host,strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  memcpy(&sin.sin_addr.s_addr,localhost->h_addr,localhost->h_length);
  sin.sin_family = AF_INET;
  sin.sin_port   = htons(port);
  
  s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  if (s < -1)
  {
    (*cv_report)(LOG_ERR,"$","socket() = %a",strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  rc = setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
  if (rc == -1)
  {
    (*cv_report)(LOG_ERR,"$","setsockopt(SO_REUSEADDR) = %a",strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  if (bind(s,(struct sockaddr *)&sin,sizeof(sin)))
  {
    (*cv_report)(LOG_ERR,"$","bind() = %a",strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  close_on_exec(s);
  
  return(s);
}

/*******************************************************************/

void close_on_exec(int fh)
{
  int val;

  ddt(fh >= 0);
  
  val = fcntl(fh,F_GETFD,0);
  if (val < 0)
  {
    (*cv_report)(LOG_ERR,"$","fcntl(get) = %a",strerror(errno));
    return;
  }
  
  val |= FD_CLOEXEC;
  
  if (fcntl(fh,F_SETFD,val) < 0)
    (*cv_report)(LOG_ERR,"$","fcntl(set) = %a",strerror(errno));
}

/*****************************************************************/

void close_stream_on_exec(Stream s)
{
  close_on_exec(((struct fhmod *)s)->fh);
}

/******************************************************************/

