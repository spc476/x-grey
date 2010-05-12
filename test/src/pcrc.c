/************************************************************************
*
* Copyright 2010 by Sean Conner.  All Rights Reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
***********************************************************************/

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
#include <netinet/in.h>
#include <signal.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>

#include "../../common/src/greylist.h"
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

char *g_inputfile  = "precanned-tuple.01";

static const struct option mc_options[] =
{
  { "input-file"	, required_argument	, NULL	, OPT_INPUT_FILE  } ,
  { "help"		, no_argument		, NULL	, OPT_HELP	  } ,
  { NULL		, 0			, NULL	, 0		  }
};

/**********************************************************************/

void	parse_command_line	(int,char *[]);

/**********************************************************************/

int main(int argc,char *argv[])
{
  struct stat              status;
  int                      fh;
  void                    *vba;
  size_t                  *pps;
  char                    *packet;
  struct greylist_request *preq;
  int                      rc;
  
  MemInit();
  DdtInit();
  StreamInit();
  
  parse_command_line(argc,argv);
  
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
  
  while(status.st_size)
  {
    pps = (size_t *)packet;
    packet += sizeof(size_t);
    preq = (struct greylist_request *)packet;
    packet += *pps;
    status.st_size -= (sizeof(size_t) + *pps);
    LineSFormat(StdoutStream,"x10.10L0","%a\n",(unsigned long)ntohl(preq->crc));
  }
  
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
      case OPT_INPUT_FILE:
           g_inputfile = dup_string(optarg);
           break;
      case OPT_HELP:
      default:
           LineSFormat(
           	StderrStream,
           	"$ $",
           	"usage: %a [options]\n"
           	"\t--input-file <file>\t(%b)\n"
           	"\t--help\n"
           	"\n",
           	argv[0],
           	g_inputfile
           );
           exit(EXIT_FAILURE);
    }
  }
}

/***********************************************************************/

