/***************************************************************************
*
* Copyright 2007 by Sean Conner.
*
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
/*#include <arpa/inet.h>*/
#include <unistd.h>

#include <cgilib/memory.h>
#include <cgilib/types.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>

#include "../../common/src/graylist.h"
#include "../../common/src/util.h"
#include "../../common/src/crc32.h"
#include "../../common/src/globals.h"
#include "globals.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

/***********************************************************/

int		check_graylist	(int,char *,char *,char *);	
static int	process_request	(int);
static void	handler_sigalrm	(int);

/**********************************************************/

static volatile sig_atomic_t mf_sigalrm;
static char                  m_dunno [] = "action=dunno\n\n";
static char                  m_reject[] = "action=reject I reject your reality and substitute my own\n\n";
static char                  m_defer [] = "action=defer_if_permit Service temporarily unavailable\n\n";

/**********************************************************/

int main(int argc,char *argv[])
{
  int sock;
  
  MemInit();
  DdtInit();
  StreamInit();
  
  if (GlobalsInit(argc,argv) != EXIT_SUCCESS)
    return(EXIT_FAILURE);  

  set_signal(SIGALRM,handler_sigalrm);
  
  sock = create_socket(c_host,c_port,SOCK_DGRAM);
  
  while(process_request(sock))
    ;
  
  return(EXIT_SUCCESS);
}

/*****************************************************/

static int process_request(int sock)
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
      ddt(bufsiz <= BUFSIZ);
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
      int response;
      
      syslog(LOG_INFO,"tuple: [%s , %s , %s]",ip,from,to);
      response = check_graylist(sock,ip,from,to);
      
      switch(response)
      {
        default: 
	     ddt(0);
        case IFT_ACCEPT:
	     rrc = write(STDOUT_FILENO,m_dunno,sizeof(m_dunno) - 1);
	     ddt(rrc == sizeof(m_dunno) - 1);
	     break;
	case IFT_REJECT:
	     rrc = write(STDOUT_FILENO,m_reject,sizeof(m_reject) - 1);
	     ddt(rrc == sizeof(m_reject) - 1);
	     break;
	case IFT_GRAYLIST:
	     rrc = write(STDOUT_FILENO,m_defer,sizeof(m_defer) - 1);
	     ddt(rrc == sizeof(m_defer) - 1);
	     break;
      }

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

/********************************************************/

int check_graylist(int sock,char *ip,char *from,char *to)
{
  byte                      outpacket[600];
  byte                      inpacket [1500];
  struct graylist_request  *glq;
  struct graylist_response *glr;
  struct sockaddr_in        sip;
  socklen_t                 sipsize;
  size_t                    sfrom;
  size_t                    sto;
  byte                     *p;
  char                     *d;
  ssize_t                   rrc;
  size_t                    packetsize; 
  CRC32                     crc;

  sfrom = min(strlen(from),200);
  sto   = min(strlen(to),  200);
  
  glq = (struct graylist_request *)outpacket;
  glr = (struct graylist_response *)inpacket;
  
  p = glq->data;
  
  glq->version  = htons(VERSION);
  glq->MTA      = htons(MTA_POSTFIX);
  glq->type     = htons(CMD_GRAYLIST);
  glq->ipsize   = htons(4);
  glq->fromsize = htons(sfrom);
  glq->tosize   = htons(sto);
  
  *p++ = strtoul(ip,&d,10); d++;	/* pack IP */
  *p++ = strtoul(d ,&d,10); d++;
  *p++ = strtoul(d ,&d,10); d++;
  *p++ = strtoul(d ,&d,10); d++;
  
  memcpy(p,from,sfrom);	p += sfrom;
  memcpy(p,to,sto);     p += sto;

  packetsize = (size_t)(p - outpacket);

  crc        = crc32(INIT_CRC32,&glq->version,packetsize - sizeof(CRC32));
  crc        = crc32(crc,c_secret,c_secretsize);
  glq->crc   = htonl(crc);

  rrc = sendto(
  		sock,
  		outpacket,
  		packetsize,
  		0,
  		(const struct sockaddr *)&c_raddr,
  		c_raddrsize
  	);
  if (rrc == -1)
  {
    (*cv_report)(LOG_ERR,"$","sendto() = %a",strerror(errno));
    return(IFT_ACCEPT);
  }
  
  alarm(c_timeout);
  
  sipsize = sizeof(struct sockaddr_in);
  rrc     = recvfrom(
  		sock,
  		inpacket,
  		sizeof(inpacket),
  		0,
  		(struct sockaddr *)&sip,
  		&sipsize
  	);
  
  alarm(0);
  
  /*--------------------------------------------------------
  ; if we get anything we don't understand, return TRUE---better
  ; to err on the side of caution than to be overly strict
  ; and miss an important email.
  ;---------------------------------------------------------*/
  
  if (rrc == -1)
  {
    /*---------------------------------------------------
    ; we expect EINTR (from SIG_ALRM), but if the error
    ; is anything else, we record it.
    ;---------------------------------------------------*/
    
    if (errno != EINTR)
      (*cv_report)(LOG_ERR,"$","recvfrom() = %a",strerror(errno));
    else
      (*cv_report)(LOG_DEBUG,"","timeout");
    return(IFT_ACCEPT);
  }
  
  crc = crc32(INIT_CRC32,&glr->version,rrc - sizeof(unet32));
  crc = crc32(crc,c_secret,c_secretsize);
  
  if (crc != ntohl(glr->crc))
  {
    (*cv_report)(LOG_ERR,"","received back packet");
    return(IFT_ACCEPT);
  }
  
  if (ntohs(glr->version) != VERSION)
  {
    (*cv_report)(LOG_ERR,"","received response from wrong version");
    return(IFT_ACCEPT);
  }

  if (ntohs(glr->MTA) != MTA_POSTFIX)
  {
    (*cv_report)(LOG_ERR,"","are we running another MTA here?");
    return(IFT_ACCEPT);
  }
  
  if (ntohs(glr->type) != CMD_GRAYLIST_RESP)
  {
    (*cv_report)(LOG_ERR,"i","received error %a",ntohs(glr->response));
    return(IFT_ACCEPT);
  }
  
  glr->response = ntohs(glr->response);
  
  (*cv_report)(
  	LOG_DEBUG,
	"$",
	"received %a",
	ci_map_chars(glr->response,c_ift,C_IFT)
  );
  return(glr->response);

}

/*******************************************************************/

static void handler_sigalrm(int sig)
{
  mf_sigalrm = 1;
}

/******************************************************************/

