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

#ifndef __GNUC__
#  define __attribute__(x)
#endif

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
#include <unistd.h>

#include <libmilter/mfapi.h>

#include <cgilib/memory.h>
#include <cgilib/types.h>
#include <cgilib/ddt.h>
#include <cgilib/util.h>
#include <cgilib/stream.h>

#include "../../common/src/greylist.h"
#include "../../common/src/util.h"
#include "../../common/src/crc32.h"
#include "../../common/src/globals.h"
#include "globals.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

struct mfprivate
{
  size_t  sip;
  byte    ip    [16];
  char    sender[200];
};

/***********************************************************/

static sfsistat		mf_connect	(SMFICTX *,char *,_SOCK_ADDR *);
static sfsistat		mf_mail_from	(SMFICTX *,char **);
static sfsistat		mf_rcpt_to	(SMFICTX *,char **);
static sfsistat		mf_close	(SMFICTX *);

static int		check_greylist	(int,byte *,char *,char *);	
static void		handler_sigalrm	(int);
static int		isbracket	(int);

/**********************************************************/

static volatile sig_atomic_t mf_sigalrm;
static struct smfiDesc       m_smfilter = 
{
  "Greylist Daemon",
  SMFI_VERSION,
  SMFIF_NONE,
  mf_connect,
  NULL,
  mf_mail_from,
  mf_rcpt_to,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  mf_close
};

/**********************************************************/

int main(int argc,char *argv[])
{
  int rc;
  
  MemInit();
  DdtInit();
  StreamInit();
  
  if (GlobalsInit(argc,argv) != EXIT_SUCCESS)
    return(EXIT_FAILURE);  

  set_signal(SIGALRM,handler_sigalrm);
  rc      = smfi_setconn((char *)c_filterchannel);
  rc      = smfi_register(m_smfilter);
  gl_sock = create_socket(c_host,c_port,SOCK_DGRAM);

  if (gl_sock == -1)
    return EXIT_FAILURE;

  return (smfi_main());
}

/*****************************************************/

static sfsistat mf_connect(SMFICTX *ctx,char *hostname __attribute__((unused)),_SOCK_ADDR *addr)
{
  struct mfprivate   *data;
  struct sockaddr_in *ip = (struct sockaddr_in *)addr;

  ddt(ctx  != NULL);
  ddt(addr != NULL);

  data = smfi_getpriv(ctx);
  if (data == NULL)
  {
    data = MemAlloc(sizeof(struct mfprivate));
    smfi_setpriv(ctx,data);
  }

  data->sip    = 4;
  memset(data->sender,0,sizeof(data->sender));
  memcpy(data->ip,&ip->sin_addr.s_addr,4);
  
  return(SMFIS_CONTINUE);
}

/***********************************************************/

static sfsistat mf_mail_from(SMFICTX *ctx,char **argv)
{
  struct mfprivate *data;
  size_t            size;
  char             *tauth;
  
  ddt(ctx  != NULL);
  ddt(argv != NULL);

  /*---------------------------------------------------
  ; check to see if the user has authenticated, if so
  ; return OK and skip the rest of the processing (we
  ; hope)
  ;---------------------------------------------------*/

  tauth = smfi_getsymval(ctx,"{auth_authen}");
  if (!emptynull_string(tauth))
    return(SMFIS_ACCEPT);
  
  /*-------------------------------------------------
  ; continue saving information for later ... 
  ;--------------------------------------------------*/

  data = smfi_getpriv(ctx);
  size = min(sizeof(data->sender) - 1,strlen(argv[0]));
  memcpy(data->sender,argv[0],size);
  remove_char(data->sender,isbracket);

  return(SMFIS_CONTINUE);
}

/**********************************************************/

static sfsistat mf_rcpt_to(SMFICTX *ctx,char **argv)
{
  struct mfprivate *data;
  char             *to;
  int               response;
  
  ddt(ctx  != NULL);
  ddt(argv != NULL);
  
  data     = smfi_getpriv(ctx);
  to       = remove_char(dup_string(argv[0]),isbracket);
  response = check_greylist(gl_sock,data->ip,data->sender,to);
  
  MemFree(to);
  
  switch(response)
  {
    case IFT_ACCEPT:   return (SMFIS_ACCEPT);
    case IFT_REJECT:   return (SMFIS_REJECT);
    case IFT_GREYLIST: return (SMFIS_TEMPFAIL);
    default: ddt(0);   return (SMFIS_ACCEPT);
  }
}

/****************************************************************/

static sfsistat mf_close(SMFICTX *ctx)
{
  struct mfprivate *data;
  
  ddt(ctx != NULL);
  
  data = smfi_getpriv(ctx);

  if (data != NULL)
    MemFree(data);
  
  smfi_setpriv(ctx,NULL);
  return(SMFIS_CONTINUE);
}

/****************************************************************/

static int check_greylist(int sock,byte *ip,char *from,char *to)
{
  union greylist_all_packets  outpacket;
  union greylist_all_packets  inpacket;
  struct greylist_request    *glq;
  struct greylist_response   *glr;
  struct sockaddr_in          sip;
  socklen_t                   sipsize;
  size_t                      sfrom;
  size_t                      sto;
  byte                       *p;
  ssize_t                     rrc;
  size_t                      packetsize;
  CRC32                       crc;
  
  sfrom = min(strlen(from),200);
  sto   = min(strlen(to),  200);
  
  glq = &outpacket.req;
  glr = &inpacket.res;
  
  p = glq->data;

  glq->crc      = htons(0);  
  glq->version  = htons(VERSION);
  glq->MTA      = htons(MTA_POSTFIX);
  glq->type     = htons(CMD_GREYLIST);
  glq->ipsize   = htons(4);
  glq->fromsize = htons(sfrom);
  glq->tosize   = htons(sto);

  memcpy(p,ip,4);	p += 4;
  memcpy(p,from,sfrom);	p += sfrom;
  memcpy(p,to,sto);     p += sto;
  
  packetsize = (size_t)(p - outpacket.data);

  crc      = crc32(INIT_CRC32,&glq->version,packetsize - sizeof(CRC32));
  crc      = crc32(crc,c_secret,c_secretsize);
  glq->crc = htonl(crc);
  
  rrc = sendto(
  		sock,
  		glq,
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
  		inpacket.data,
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
    (*cv_report)(LOG_ERR,"","received bad packet");
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
  
  if (ntohs(glr->type) != CMD_GREYLIST_RESP)
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

static void handler_sigalrm(int sig __attribute__((unused)))
{
  mf_sigalrm = 1;
}

/******************************************************************/

static int isbracket(int c)
{
  if (c == '>') return(1);
  if (c == '<') return(1);
  return(0);
}

/******************************************************************/

