
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

static void	mainloop		(int);

/********************************************************************/

int main(int argc,char *argv[])
{
  int sock;
  int rc;
  
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

  mainloop(sock)
  
  return(EXIT_SUCCESS);
}

/********************************************************************/

static void mainloop(int sock)
{
}

/**********************************************************************/

