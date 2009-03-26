
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>

#include "../../common/src/greylist.h"
#include "../../common/src/crc32.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

/***********************************************************************/

enum
{
  OPT_INPUT_FILE = OPT_USER,
  OPT_OUTPUT_FILE,
  OPT_COUNT
};

/************************************************************************/

char   *g_inputfile  = NULL;
char   *g_outputfile = "precanned-tuple.%02d";
char   *g_secret     = "decafbad";
size_t  g_secretsize = 8;
size_t  g_count      = 65536uL;

static const struct option mc_options[] =
{
  { "input-file"	, required_argument	, NULL	, OPT_INPUT_FILE  } ,
  { "output-file"	, required_argument	, NULL	, OPT_OUTPUT_FILE } ,
  { "count"		, required_argument	, NULL	, OPT_COUNT	  } ,
  { "secret"		, required_argument	, NULL	, OPT_SECRET	  } ,
  { "help"		, no_argument		, NULL	, OPT_HELP	  } ,
  { NULL		, 0			, NULL	, 0		  }
};

/**********************************************************************/

void	parse_command_line	(int,char *[]);

/**********************************************************************/

int main(int argc,char *argv[])
{
  Stream                   input;
  int                      out       = -1;
  int                      filecount = 1;
  size_t                   count;
  byte                     outpacket[1500];
  struct greylist_request *glq;
  
  MemInit();
  DdtInit();
  StreamInit();

  input = StdinStream;
  
  parse_command_line(argc,argv);
  
  if (g_inputfile)
    input = FileStreamRead(g_inputfile);
  
  count = g_count;
  
  while(!StreamEOF(input))
  {
    char   *line;
    char   *from;
    char   *to;
    size_t  sfrom;
    size_t  sto;
    size_t  packetsize;
    byte   *p;
    CRC32   crc;

    if (count == g_count)
    {
      char filename[BUFSIZ];
      
      close(out);
      sprintf(filename,g_outputfile,filecount++);
      out = open(filename,O_WRONLY | O_CREAT | O_TRUNC,0644);
      if (out == -1)
      {
        perror(filename);
        exit(EXIT_FAILURE);
      }
      count = 0;
    }
      
    line = LineSRead(input);
    if (empty_string(line)) { MemFree(line); continue; }
    
    glq = (struct greylist_request *)outpacket;
    p   = glq->data;
    
    glq->version = htons(VERSION);
    glq->MTA     = htons(MTA_POSTFIX);
    glq->type    = htons(CMD_GREYLIST);
    glq->ipsize  = htons(4);
    
    *p++ = strtoul(line,&from,10); from++;
    *p++ = strtoul(from,&from,10); from++;
    *p++ = strtoul(from,&from,10); from++;
    *p++ = strtoul(from,&from,10); from++;
    
    to = strchr(from,' ');
    if (to == NULL) continue;
    *to++ = '\0';
    
    if (strcmp(from,"-") == 0)
      from = "";
    
    if (strcmp(to,"-") == 0)
      to = "";
    
    sfrom = min(strlen(from),200);
    sto   = min(strlen(to),200);
    
    glq->fromsize = htons(sfrom);
    glq->tosize   = htons(sto);
    
    memcpy(p,from,sfrom); p += sfrom;
    memcpy(p,to,  sto);   p += sto;
    
    packetsize = (size_t)(p - outpacket);
    crc        = crc32(INIT_CRC32,&glq->version,packetsize - sizeof(CRC32));
    crc        = crc32(crc,g_secret,g_secretsize);
    glq->crc   = htonl(crc);
    
    write(out,&packetsize,sizeof(packetsize));
    write(out,outpacket,packetsize);
    MemFree(line);
    count++;
  }
  
  close(out);
  StreamFree(input);
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
      case OPT_OUTPUT_FILE:
           g_outputfile = dup_string(optarg);
           break;
      case OPT_COUNT:
           g_count = strtoul(optarg,NULL,10);
           break;
      case OPT_SECRET:
           g_secret = dup_string(optarg);
           g_secretsize = strlen(g_secret);
           break;
      case OPT_HELP:
      default:
           LineSFormat(
           	StderrStream,
           	"$ $ $ L",
           	"usage: %a [options]\n",
           	"\t--input-file <file>	(stdin)\n"
           	"\t--output-file <file>	(%b)\n"
           	"\t--secret <string>	(%c)\n"
           	"\t--count <num>	(%d)\n"
           	"\t--help		(this text)\n"
           	"\n",
           	argv[0],
           	g_outputfile,
           	g_secret,
           	(unsigned long)g_count
           );
           exit(EXIT_FAILURE);
    }
  }
}

/***********************************************************************/

