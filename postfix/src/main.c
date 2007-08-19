
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "globals.h"

static int	process_request		(void);

/**********************************************************/

int main(int argc,char *argv[])
{
  if (GlobalsInit(argc,argv) != EXIT_SUCCESS)
    return(EXIT_FAILURE);  

  while(process_request())
    ;
  
  return(EXIT_SUCCESS);
}

/*****************************************************/

static int process_request(void)
{
  static char    buffer[BUFSIZ];
  static size_t  bufsiz    = 0;
  static int     refill    = 1;
  char   *p;
  char   *v;
  char   *request = NULL;
  char   *from    = NULL;
  char   *to      = NULL;
  char   *ip      = NULL;
  size_t  amount;
  ssize_t rrc;
  
  /*------------------------------------------------
  ; this whole mess because of a bug in the Stream
  ; module of CGILib (still haven't gotten that down,
  ; and getting bitten by leaky abstractions yet again!
  ;
  ; Anyway, this whole mess (which is quite fast actually)
  ; reads in a bunch of data, finds each line, breaks
  ; each line into "name" = "value", the sets the variables
  ; we are interested in, all using ANSI C and POSIX calls.
  ; 
  ; Even when I fix CGILib, I doubt I'll fix this code, as
  ; it works, and it works fast.
  ;--------------------------------------------------*/
  
  while(1)
  {
    if (refill)
    {
      if (bufsiz == BUFSIZ)
      {
        syslog(LOG_ERR,"buffer overflow-aborting");
        return(0);
      }
      
      rrc = read(STDIN_FILENO,&buffer[bufsiz],BUFSIZ - bufsiz);
      if (rrc == -1)
      {
        syslog(LOG_DEBUG,"read() = %s",strerror(errno));
	return(0);
      }
      if (rrc ==  0) return(0);
      
      bufsiz += rrc;
      assert(bufsiz <= BUFSIZ);
      refill  = 0;
    }
    
    p = memchr(buffer,'\n',bufsiz);
    if (p == NULL)
    {
      refill = 1;
      continue;
    }
    
    if (p == buffer)
    {
      syslog(LOG_INFO,"tuple: [%s , %s , %s]",ip,from,to);
      rrc = write(STDOUT_FILENO,"action=dunno\n\n",14);
      assert(rrc == 14);
      free(request);
      free(from);
      free(to);
      free(ip);
      p++;
      amount = bufsiz - (size_t)(p - buffer);
      memmove(buffer,p,amount);
      bufsiz = amount;
      return(1);
    }

    v = memchr(buffer,'=',bufsiz);

    if (v == NULL)
    {
      syslog(LOG_ERR,"bad input format");
      return(0);
    }

    *p++ = '\0';
    *v++ = '\0';

    syslog(LOG_DEBUG,"request: %s = %s",buffer,v);

    if (strcmp(buffer,"request") == 0)
      request = strdup(v);
    else if (strcmp(buffer,"sender") == 0)
      from = strdup(v);
    else if (strcmp(buffer,"recipient") == 0)
      to = strdup(v);
    else if (strcmp(buffer,"client_address") == 0)
      ip = strdup(v);
    
    amount = bufsiz - (size_t)(p - buffer);
    memmove(buffer,p,amount);
    bufsiz = amount;
  }
}

