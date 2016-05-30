/***************************************************************************
*
* Copyright 2010 by Sean Conner.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*************************************************************************/

#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>

#include "eglobals.h"
#include "util.h"

static unsigned long m_logseq;

/*****************************************************************/

int create_socket(const char *host,int port,int type)
{
  struct sockaddr_in  sin;
  struct hostent     *localhost;
  in_addr_t           lh;
  int                 s;
  int                 rc;
  int                 reuse = 1;

  assert(host != NULL);
  assert(port >= 0);
  assert(port <= 65535);
  
  lh = inet_addr(host);

  if (lh == (in_addr_t)-1)
  {
    localhost = gethostbyname(host);
    if (localhost == NULL)
    {
      syslog(LOG_ERR,"gethostbyname(%s) = %s",c_host,strerror(errno));
      return -1;
    }
    memcpy(&sin.sin_addr.s_addr,localhost->h_addr,localhost->h_length);
  }
  else
    memcpy(&sin.sin_addr.s_addr,&lh,sizeof(in_addr_t));

  sin.sin_family = AF_INET;
  sin.sin_port   = htons(port);
  
  s = socket(AF_INET,type,0);
  if (s < -1)
  {
    syslog(LOG_ERR,"socket() = %s",strerror(errno));
    return -1;
  }
  
  rc = setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
  if (rc == -1)
  {
    syslog(LOG_ERR,"setsockopt(SO_REUSEADDR) = %s",strerror(errno));
    return -1;
  }
  
  if (bind(s,(struct sockaddr *)&sin,sizeof(sin)))
  {
    syslog(LOG_ERR,"bind() = %s",strerror(errno));
    return -1;
  }
  
  log_address((struct sockaddr *)&sin);
  
  return s;
}

/*******************************************************************/

int ci_map_int(const char *name,const struct chars_int list[],size_t size)
{
  size_t i;

  assert(name != NULL);
  assert(list != NULL);
  assert(size >  0);

  for (i = 0 ; i < size ; i++)
  {
    if (strcmp(name,list[i].name) == 0)
      return(list[i].value);
  }
  return -1;
}

/************************************************************************/

const char *ci_map_chars(int value,const struct chars_int list[],size_t size)
{
  size_t i;

  assert(list != NULL);
  assert(size >  0);

  for (i = 0 ; i < size ; i++)
  {
    if (value == list[i].value)
      return(list[i].name);
  }
  return "";
}

/***********************************************************************/

void log_address(struct sockaddr *pin)
{
  struct sockaddr_in *pi;
  
  if (pin->sa_family != AF_INET)
  {
    syslog(LOG_INFO,"wrong family of packet: %d",pin->sa_family);
    return;
  }
  
  pi = (struct sockaddr_in *)pin;
  
  syslog(
        LOG_DEBUG,
        "Address: %s:%d",
        ipv4((const byte *)&pi->sin_addr.s_addr),
        ntohs(pi->sin_port)
  );
}

/***********************************************************************/

char *ipv4(const byte *ip)
{
  static char buffer[20];
  
  assert(ip != NULL);
  
  sprintf(buffer,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
  return buffer;
}

/***********************************************************************/

int set_signal(int sig,void (*handler)(int))
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
    syslog(LOG_ERR,"sigaction() = %s",strerror(errno));
  
  return rc;
}

/**********************************************************************/

double read_dtime(char *arg,double defaultval)
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
           fprintf(stderr,"Bad time specifier '%s'---using default\n",arg);
           return defaultval;
    }
    time += val;
  } while (*p);

  return time;
}

/*******************************************************************/

size_t read_size(char *arg)
{
  size_t value;
  char   *p;
  
  assert(arg != NULL);
  
  value = strtoul(arg,&p,10);
  
  switch(*p)
  {
    case 'k': value *= 1024uL ; break;
    case 'm': value *= (1024uL * 1024uL); break;
    case 'g': value *= (1024uL * 1024uL * 1024uL) ; break;
    case 'b':
    default:   break;
  }
  return value;
}

/*******************************************************************/

char *iptoa(IP addr)
{
  char buffer[16];
  
  assert(addr != 0);
  
  snprintf(
    buffer,
    sizeof(buffer),
    "%d.%d.%d.%d",
    (addr >> 24) & 0xFF,
    (addr >> 16) & 0xFF,
    (addr >>  8) & 0xFF,
    (addr      ) & 0xFF
  );

  return strdup(buffer);
}

/*********************************************************************/

char *ipptoa(IP addr,Port p)
{
  char buffer[23];
  
  snprintf(
    buffer,
    sizeof(buffer),
    "%d.%d.%d.%d:%d",
    (addr >> 24) & 0xFF,
    (addr >> 16) & 0xFF,
    (addr >>  8) & 0xFF,
    (addr      ) & 0xFF,
    p
  );
  
  return strdup(buffer);
}

/**************************************************************/

char *timetoa(time_t stamp)
{
  char buffer[BUFSIZ];
  
  strftime(buffer,BUFSIZ,"%Y-%m-%dT%H:%M:%S",localtime(&stamp));
  return strdup(buffer);
}

/**************************************************************/

char *report_time(time_t start,time_t end)
{
  char      *delta;
  char       txt_start[BUFSIZ];
  char       txt_end  [BUFSIZ];
  char       msg      [BUFSIZ];
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

  delta = report_delta(difftime(end,start));
  
  snprintf(msg,sizeof(msg),"Start: %s End: %s Running: %s",ptstart,ptend,delta);
  free(delta);
  return strdup(msg);
}
  
/******************************************************************/

char *report_delta(double diff)
{
  char   buffer[BUFSIZ];
  size_t idx;
  int    year;
  int    day;
  int    hour;
  int    min;
  int    sec;

  assert(diff >= 0.0);
  
  year  = (int)(diff / SECSYEAR);
  diff -= ((double)year) * SECSYEAR;
  
  day   = (int)(diff / SECSDAY);
  diff -= ((double)day) * SECSDAY;
  
  hour  = (int)(diff / SECSHOUR);
  diff -= ((double)hour) * SECSHOUR;
  
  min   = (int)(diff / SECSMIN);
  diff -= ((double)min) * SECSMIN;
  
  sec   = (int)(diff);
  idx   = 0;
  
  if (year) idx += sprintf(&buffer[idx],"%dy",year);
  if (day)  idx += sprintf(&buffer[idx],"%dd",day);
  if (hour) idx += sprintf(&buffer[idx],"%dh",hour);
  if (min)  idx += sprintf(&buffer[idx],"%dm",min);
  if (sec)  idx += sprintf(&buffer[idx],"%ds",sec);
  
  assert(idx < BUFSIZ);
  return strdup(buffer);
}

/*********************************************************************/

String *split(size_t *pnum,char *txt)
{
  size_t  num  = 0;
  size_t  max  = 0;
  String *pool = NULL;
  char   *p;
  
  assert(pnum != NULL);
  assert(txt  != NULL);
  
  while(*txt)
  {
    if (num == max)
    {
      max += 1024;
      pool = realloc(pool,max * sizeof(String));
    }
    
    for ( ; (*txt) && isspace(*txt) ; txt++)
      ;

    for ( p = txt ; (*p) && !isspace(*p) ; p++)
      ;
    
    if (p == txt) break;
    
    pool[num].d   = txt;
    pool[num++].s = p - txt;

    
    if (*p == '\0')
      break;
    
    *p++ = '\0';
    txt  = p;
    syslog(
        LOG_DEBUG,
        "%lu '%s' %lu %lu",
        (unsigned long)num - 1,
        pool[num - 1].d,
        (unsigned long)pool[num - 1].s,
        (unsigned long)strlen(pool[num - 1].d)
      );
  }
  
  syslog(
        LOG_DEBUG,
        "%lu '%s' %lu %lu",
        (unsigned long)num - 1,
        pool[num - 1].d,
        (unsigned long)pool[num - 1].s,
        (unsigned long)strlen(pool[num - 1].d)
      );
  *pnum = num;
  return pool;
}

/********************************************************************/

void write_pidfile(const char *fname)
{
  FILE *fp;
  
  assert(fname != NULL);
  
  fp = fopen(fname,"w");
  if (fp)
  {
    fprintf(fp,"%lu\n",(unsigned long)getpid());
    fclose(fp);
  }
  else
    syslog(LOG_ERR,"fopen(%s,WRITE) = %s",fname,strerror(errno));    
}

/*********************************************************************/

int parse_ip(byte *ip,int *mask,char *text)
{
  size_t octetcount = 0;
  
  assert(ip   != NULL);
  assert(mask != NULL);
  assert(text != NULL);

  do  
  {
    ip[octetcount] = strtoul(text,&text,10);
    if (text[0] == '\0') break;
    if (text[0] == '/')  break;
    if (text[0] != '.')
      return(ERR_ERR);
    
    octetcount++;
    text++;
  } while(octetcount < 4);
  
  if (*text++ == '/')
  {
    *mask = strtoul(text,&text,10);
    if (*mask > 32) return(ERR_ERR);
  }
  else
    *mask = 32;
  
  return ERR_OKAY;
}

/************************************************************************/

