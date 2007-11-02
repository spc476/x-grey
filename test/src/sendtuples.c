
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>

#include "../../common/src/graylist.h"
#include "../../common/src/crc32.h"
#include "../../common/src/util.h"
#include "../../conf.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

/***********************************************************************/

enum
{
  OPT_INPUT_FILE = OPT_USER
};

/************************************************************************/

	/*----------------------
	; required for linkage
	;-----------------------*/
	
char   *c_host         = "0.0.0.0";
int     c_port         = 0;
char   *c_timeformat   = "%c";
int     c_log_facility = 0;
int     c_log_level    = 0;
char   *c_log_id       = NULL;
int     cf_debug       = 0;
void  (*cv_report)(int,char *,char *,...) = report_syslog;

	/*--------------------------
	; our global variables
	;--------------------------*/

struct sockaddr_in  g_raddr;
socklen_t           g_raddrsize  = sizeof(struct sockaddr_in);
char               *g_rhost      = SERVER_HOST;
int                 g_rport      = SERVER_PORT;
char               *g_inputfile  = "precanned-tuple.01";

static const struct option mc_options[] =
{
  { "server"		, required_argument	, NULL	, OPT_RHOST	  } ,
  { "server-port"	, required_argument	, NULL	, OPT_RPORT	  } ,
  { "host"		, required_argument	, NULL	, OPT_HOST	  } ,
  { "port"		, required_argument	, NULL	, OPT_PORT	  } ,
  { "input-file"	, required_argument	, NULL	, OPT_INPUT_FILE  } ,
  { "help"		, no_argument		, NULL	, OPT_HELP	  } ,
  { NULL		, 0			, NULL	, 0		  }
};

/**********************************************************************/

void	parse_command_line	(int,char *[]);

/**********************************************************************/

int main(int argc,char *argv[])
{
  struct stat     status;
  struct hostent *remote;
  int             fh;
  void           *vba;
  size_t         *pps;
  char           *packet;
  int		  sock;
  pid_t           child;
  int             rc;
  
  MemInit();
  DdtInit();
  StreamInit();
  
  parse_command_line(argc,argv);
  
  /*-----------------------------
  ; cache server address:port
  ;----------------------------*/
  
  remote = gethostbyname(g_rhost);
  if (remote == NULL)
  {
    perror(g_rhost);
    return(EXIT_FAILURE);
  }
  
  memcpy(&g_raddr.sin_addr.s_addr,remote->h_addr,remote->h_length);
  g_raddr.sin_family = AF_INET;
  g_raddr.sin_port   = htons(g_rport);
  
  /*--------------------------------------------------
  ; open the precanned packets, and map into memory.
  ;---------------------------------------------------*/
  
  fh = open(g_inputfile,O_RDONLY);
  if (fh == -1)
  {
    perror(g_inputfile);
    return(EXIT_FAILURE);
  }
  
  rc = fstat(fh,&status);
  if (rc == -1)
  {
    perror("stat()");
    close(fh);
    return(EXIT_FAILURE);
  }
  
  vba = mmap(NULL,status.st_size, PROT_READ,MAP_PRIVATE,fh,0);
  if (vba == (void *)-1)
  {
    perror("mmap()");
    close(fh);
    return(EXIT_FAILURE);
  }
  
  packet = vba;
  sock   = create_socket(c_host,c_port,SOCK_DGRAM);

#if 1
  child = fork();
  if (child == (pid_t)-1)
  {
    perror("fork()");
    close(sock);
    munmap(vba,status.st_size);
    close(fh);
    return(EXIT_FAILURE);
  }
#else
  child = 0;
#endif

  if (child == 0)	/* parent */
  {
    size_t  size  = status.st_size;
    size_t  count = 0;
    ssize_t rrc;
    
    while(size)
    {
      pps = (size_t *)packet;
      packet += sizeof(size_t);
      rrc = sendto(
    		sock,
    		packet,
    		*pps,
    		0,
    		(const struct sockaddr *)&g_raddr,
    		g_raddrsize
    	);
      packet += *pps;
      size -= (sizeof(size_t) + *pps);
      count++;

#if 0
      {
        byte inpacket[1500];
        struct sockaddr_in sip;
        socklen_t          sipsize;
       
        sipsize = sizeof(struct sockaddr_in);
        rrc     = recvfrom(
       			sock,
       			inpacket,
       			sizeof(inpacket),
       			0,
       			(struct sockaddr *)&sip,
       			&sipsize
       		);
      }
#endif

    }
    
    LineSFormat(StdoutStream,"L","%a packets sent\n",(unsigned long)count);
  }
  else			/* child */
  {
    byte               inpacket[1500];
    struct sockaddr_in sip;
    socklen_t          sipsize;
    ssize_t            rrc;
    
    while(1)
    {
      sipsize = sizeof(struct sockaddr_in);
      rrc     = recvfrom(
      			sock,
      			inpacket,
      			sizeof(inpacket),
      			0,
      			(struct sockaddr *)&sip,
      			&sipsize
      		);
    }
    _exit(EXIT_SUCCESS);
  }  

#if 1
  /*sleep(1);*/
  kill(child,SIGTERM);
#endif

  close(sock);
  munmap(vba,status.st_size);
  close(fh);
  return(EXIT_SUCCESS);
}

/**********************************************************************/

void parse_command_line(int argc,char *argv[])
{
  while(1)
  {
    int option = 0;
    
    switch(getopt_long_only(argc,argv,"",mc_options,&option))
    {
      case EOF:
           return;
      case OPT_NONE:
           break;
      case OPT_RHOST:
           g_rhost = dup_string(optarg);
           break;
      case OPT_RPORT:
           g_rport = strtoul(optarg,NULL,10);
           break;
      case OPT_HOST:
           c_host = dup_string(optarg);
           break;
      case OPT_PORT:
           c_port = strtoul(optarg,NULL,10);
           break;
      case OPT_INPUT_FILE:
           g_inputfile = dup_string(optarg);
           break;
      case OPT_HELP:
      default:
           LineSFormat(
           	StderrStream,
           	"$ $ $ i $ i",
           	"usage: %a [options]\n"
           	"\t--input-file <file>\t(%b)\n"
           	"\t--server <host>\t\t(%c)\n"
           	"\t--server-port <num>\t(%d)\n"
           	"\t--host <host>\t\t(%e)\n"
           	"\t--port <num>\t\t(%f)\n"
           	"\t--help\n"
           	"\n",
           	argv[0],
           	g_inputfile,
           	g_rhost,
           	g_rport,
           	c_host,
           	c_port
           );
           exit(EXIT_FAILURE);
    }
  }
}

/***********************************************************************/

