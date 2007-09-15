
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>

#include <cgilib/memory.h>
#include <cgilib/util.h>
#include <cgilib/ddt.h>

#include "eglobals.h"
#include "util.h"

/*****************************************************************/

int create_socket(const char *host,int port,int type)
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
  
  s = socket(AF_INET,type,0);
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
  
  log_address((struct sockaddr *)&sin);
  
  return(s);
}

/*******************************************************************/

int ci_map_int(const char *name,const struct chars_int *list,size_t size)
{
  int i;

  ddt(name != NULL);
  ddt(list != NULL);
  ddt(size >  0);

  for (i = 0 ; i < size ; i++)
  {
    if (strcmp(name,list[i].name) == 0)
      return(list[i].value);
  }
  return(-1);
}

/************************************************************************/

const char *ci_map_chars(int value,const struct chars_int *list,size_t size)
{
  int i;

  ddt(list != NULL);
  ddt(size >  0);

  for (i = 0 ; i < size ; i++)
  {
    if (value == list[i].value)
      return(list[i].name);
  }
  return("");
}

/***********************************************************************/

void report_syslog(int level,char *format,char *msg, ... )
{
  Stream   out;
  va_list  arg;
  char    *txt;
  
  ddt(level  >= 0);
  ddt(format != NULL);
  ddt(msg    != NULL);
  
  va_start(arg,msg);
  out = StringStreamWrite();
  LineSFormatv(out,format,msg,arg);
  txt = StringFromStream(out);
  syslog(level,"%s",txt);
  MemFree(txt);
  StreamFree(out);
  va_end(arg);
}

/********************************************************************/

void report_stderr(int level,char *format,char *msg, ... )
{
  va_list arg;
  
  ddt(level  >= 0);
  ddt(format != NULL);
  ddt(msg    != NULL);
  
  va_start(arg,msg);
  if (level <= c_log_level)
  {
    LineSFormatv(StderrStream,format,msg,arg);
    StreamWrite(StderrStream,'\n');
  }
  va_end(arg);
}

/*******************************************************************/

void log_address(struct sockaddr *pin)
{
  struct sockaddr_in *pi;
  
  if (pin->sa_family != AF_INET)
  {
    (*cv_report)(LOG_INFO,"S","wrong family of packet: %a",pin->sa_family);
    return;
  }
  
  pi = (struct sockaddr_in *)pin;
  
  (*cv_report)(
  	LOG_INFO,
  	"$ S",
  	"Address: %a:%b",
  	ipv4((const byte *)&pi->sin_addr.s_addr),
  	ntohs(pi->sin_port)
  );
}

/***********************************************************************/

char *ipv4(const byte *ip)
{
  static char buffer[20];
  
  ddt(ip != NULL);
  
  sprintf(buffer,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
  return(buffer);
}

/***********************************************************************/

void set_signal(int sig,void (*handler)(int))
{
  struct sigaction act;
  struct sigaction oact;
  int              rc;

  /*-----------------------------------------------
  ; Do NOT set SA_RESTART!  This will cause the program
  ; to fail in mysterious ways.  I've tried the "restart
  ; system calls" method and it doesn't work for this
  ; program.  So please don't bother adding it.
  ; 
  ; You have been warned ... 
  ;-----------------------------------------------*/
  
  sigemptyset(&act.sa_mask);
  act.sa_handler = handler;
  act.sa_flags   = 0;
  
  rc = sigaction(sig,&act,&oact);
  if (rc == -1)
  {
    (*cv_report)(LOG_ERR,"$","sigaction() returned %a",strerror(errno));
    exit(EXIT_FAILURE);
  }
}

/**********************************************************************/

double read_dtime(char *arg)
{
  double time = 0.0;
  double val;
  char   *p;
 
  p = arg;
  do
  {
    val = strtod(p,&p);
    switch(toupper(*p))
    {
      case 'Y':
           val *= SECSYEAR;
           p++;
           break;
      case 'D':
           val *= SECSDAY;
           p++;
           break;
      case 'H':
           val *= SECSHOUR;
           p++;
           break;
      case 'M':
           val *= SECSMIN;
           p++;
           break;
      case 'S':
           p++;
           break;
      case '\0':
           break;
      default:
           LineSFormat(
                StderrStream,
                "$",
                "Bad time specifier '%a'\n",
                arg
           );
           exit(EXIT_FAILURE);
    }
    time += val;
  } while (*p);

  return(time);
}

/*******************************************************************/

char *iptoa(IP addr)
{
  Stream ip;
  char   *tip;
  
  ddt(addr != 0);
  
  ip = StringStreamWrite();
  LineSFormat(
  	ip,
  	"i i i i",
  	"%a.%b.%c.%d",
  	(addr >> 24) & 0xFF,
  	(addr >> 16) & 0xFF,
  	(addr >>  8) & 0xFF,
  	(addr      ) & 0xFF
  );
  
  tip = StringFromStream(ip);
  StreamFree(ip);
  return(tip);
}

/*********************************************************************/

char *ipptoa(IP addr,Port p)
{
  Stream  ip;
  char   *tip;
  
  ddt(addr != 0);
  
  ip = StringStreamWrite();
  LineSFormat(
  	ip,
  	"i i i i S",
  	"%a.%b.%c.%d:%e",
  	(addr >> 24) & 0xFF,
  	(addr >> 16) & 0xFF,
  	(addr >>  8) & 0xFF,
  	(addr      ) & 0xFF,
  	p
  );
  
  tip = StringFromStream(ip);
  StreamFree(ip);
  return(tip);
}

/**************************************************************/

char *timetoa(time_t stamp)
{
  char buffer[BUFSIZ];
  
  strftime(buffer,BUFSIZ,"%Y-%m-%dT%H:%M:%S",localtime(&stamp));
  return(dup_string(buffer));
}

/**************************************************************/

char *report_time(time_t start,time_t end)
{
  Stream     out;
  char      *ret;
  char      *delta;
  char       txt_start[BUFSIZ];
  char       txt_end  [BUFSIZ];
  char      *ptstart;
  char      *ptend;
  struct tm *ptime;

  ptstart = txt_start;
  ptend   = txt_end;
  
  ptime = localtime(&start);
  if (ptime)
    strftime(txt_start,BUFSIZ,c_timeformat,ptime);
  else
    ptstart = "[ERROR in timestamp]";
  
  ptime = localtime(&end);
  if (ptime)
    strftime(txt_end,BUFSIZ,c_timeformat,ptime);
  else
    ptend = "[ERROR in timestamp]";

  out   = StringStreamWrite();
  delta = report_delta(difftime(end,start));
  
  LineSFormat(out,"$ $ $","Start: %a End: %b Running time: %c",ptstart,ptend,delta);
    
  ret = StringFromStream(out);
  
  MemFree(delta);
  StreamFree(out);
  return(ret);
}
  
/******************************************************************/

char *report_delta(double diff)
{
  Stream  out;
  char   *ret;
  int     year;
  int     day;
  int     hour;
  int     min;
  int     sec;

  ddt(diff >= 0.0);
  
  out = StringStreamWrite();
  
  if (out == NULL)
    return(dup_string("[error reporting delta]"));
    
  year  = (int)(diff / SECSYEAR);
  diff -= ((double)year) * SECSYEAR;
  
  day   = (int)(diff / SECSDAY);
  diff -= ((double)day) * SECSDAY;
  
  hour  = (int)(diff / SECSHOUR);
  diff -= ((double)hour) * SECSHOUR;
  
  min   = (int)(diff / SECSMIN);
  diff -= ((double)min) * SECSMIN;
  
  sec   = (int)(diff);
  
  if (year) LineSFormat(out,"i"," %ay",year);
  if (day)  LineSFormat(out,"i"," %ad",day);
  if (hour) LineSFormat(out,"i"," %ah",hour);
  if (min)  LineSFormat(out,"i"," %am",min);
  if (sec)  LineSFormat(out,"i"," %as",sec);
  
  ret = StringFromStream(out);
  StreamFree(out);
  return(ret);
}

/*********************************************************************/

