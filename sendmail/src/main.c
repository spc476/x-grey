
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <libmilter/mfapi.h>

#include <cgilib/memory.h>
#include <cgilib/types.h>
#include <cgilib/ddt.h>
#include <cgilib/util.h>
#include <cgilib/stream.h>

#include "../../common/src/graylist.h"
#include "../../common/src/util.h"
#include "../../common/src/crc32.h"
#include "globals.h"

#define min(a,b)	((a) < (b)) ? (a) : (b)

struct mfprivate
{
  size_t  sip;
  byte    ip[16];
  char   *sender;
};

/***********************************************************/

static sfsistat		mf_connect	(SMFICTX *,char *,_SOCK_ADDR *);
static sfsistat		mf_mail_from	(SMFICTX *,char **);
static sfsistat		mf_rcpt_to	(SMFICTX *,char **);
static sfsistat		mf_close	(SMFICTX *);

static int		check_graylist	(int,byte *,char *,char *);	
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

  return (smfi_main());
}

/*****************************************************/

static sfsistat mf_connect(SMFICTX *ctx,char *hostname,_SOCK_ADDR *addr)
{
  struct mfprivate   *data;
  struct sockaddr_in *ip = (struct sockaddr_in *)addr;

  ddt(ctx  != NULL);
  ddt(addr != NULL);

  data = MemAlloc(sizeof(struct mfprivate));
  data->sip    = 4;
  data->sender = NULL;
  memcpy(data->ip,&ip->sin_addr.s_addr,4);
  
  smfi_setpriv(ctx,data);
  return(SMFIS_CONTINUE);
}

/***********************************************************/

static sfsistat mf_mail_from(SMFICTX *ctx,char **argv)
{
  struct mfprivate *data;
  
  ddt(ctx  != NULL);
  ddt(argv != NULL);
  
  data         = smfi_getpriv(ctx);
  data->sender = remove_char(dup_string(argv[0]),isbracket);
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
  response = check_graylist(gl_sock,data->ip,data->sender,to);
  
  MemFree(to);
  
  switch(response)
  {
    case GRAYLIST_YEA:   return (SMFIS_ACCEPT);
    case GRAYLIST_NAY:   return (SMFIS_REJECT);
    case GRAYLIST_LATER: return (SMFIS_TEMPFAIL);
    default: ddt(0);     return (SMFIS_ACCEPT);
  }
}

/****************************************************************/

static sfsistat mf_close(SMFICTX *ctx)
{
  struct mfprivate *data;
  
  ddt(ctx != NULL);
  
  data = smfi_getpriv(ctx);
  if (data != NULL)
  {
    if (data->sender != NULL) MemFree(data->sender);
    MemFree(data);
  }
  
  return(SMFIS_CONTINUE);
}

/****************************************************************/

static int check_graylist(int sock,byte *ip,char *from,char *to)
{
#if 0
  char *p;

  p = strchr(to,'@');
  if (p == NULL)
    return(GRAYLIST_YEA);

  if (strcmp(p+1,"armigeron.com") == 0)
#endif
#if 0

    syslog(LOG_INFO,"tuple: [%s , %s , %s]",ipv4(ip),from,to);

  return(GRAYLIST_YEA);

#else

  byte                      outpacket[600];
  byte                      inpacket [1500];
  struct graylist_request  *glq;
  struct graylist_response *glr;
  struct sockaddr_in        sip;
  socklen_t                 sipsize;
  size_t                    sfrom;
  size_t                    sto;
  byte                     *p;
  ssize_t                   rrc;
  size_t                    packetsize;
  CRC32                     crc;
  
  {
    char *t;
    t = strchr(to,'@');
    if (t == NULL)
      return(GRAYLIST_YEA);
    
    if (strcmp(t+1,"armigeron.com") != 0)
      return(GRAYLIST_YEA);
  }
  
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

  memcpy(p,ip,4);	p += 4;
  memcpy(p,from,sfrom);	p += sfrom;
  memcpy(p,to,sto);     p += sto;
  
  packetsize = (size_t)(p - outpacket);

  (*cv_report)(LOG_INFO,"i i","sizeof CRC: %a sizeof unet32: %b",sizeof(CRC32),sizeof(unet32));
  
  crc      = crc32(INIT_CRC32,&glq->version,packetsize - sizeof(CRC32));
  crc      = crc32(crc,c_secret,c_secretsize);
  glq->crc = htonl(crc);
  
  rrc = sendto(
  		sock,
  		outpacket,
  		(size_t)(p - outpacket),
  		0,
  		(const struct sockaddr *)&c_raddr,
  		c_raddrsize
  	);
  if (rrc == -1)
  {
    (*cv_report)(LOG_ERR,"$","sendto() = %a",strerror(errno));
    return(GRAYLIST_YEA);
  }
  
  alarm(c_timeout);
  
  rrc = recvfrom(
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
    return(GRAYLIST_YEA);
  }
  
  crc = crc32(INIT_CRC32,&glr->version,rrc - sizeof(unet32));
  crc = crc32(crc,c_secret,c_secretsize);
  
  if (crc != ntohl(glr->crc))
  {
    (*cv_report)(LOG_ERR,"","received bad packet");
    return(GRAYLIST_YEA);
  }
  
  if (ntohs(glr->version) != VERSION)
  {
    (*cv_report)(LOG_ERR,"","received response from wrong version");
    return(GRAYLIST_YEA);
  }

  if (ntohs(glr->MTA) != MTA_POSTFIX)
  {
    (*cv_report)(LOG_ERR,"","are we running another MTA here?");
    return(GRAYLIST_YEA);
  }
  
  if (ntohs(glr->type) != CMD_GRAYLIST_RESP)
  {
    (*cv_report)(LOG_ERR,"i","received error %a",ntohs(glr->response));
    return(GRAYLIST_YEA);
  }
  
  glr->response = ntohs(glr->response);
  
  if (glr->response == GRAYLIST_YEA)
  {
    (*cv_report)(LOG_DEBUG,"","received okay");
    return(GRAYLIST_YEA);
  }
  else if (glr->response == GRAYLIST_NAY)
  {
    (*cv_report)(LOG_DEBUG,"","received nay");
    return(GRAYLIST_NAY);
  }
  else if (glr->response == GRAYLIST_LATER)
  {
    (*cv_report)(LOG_DEBUG,"","received later");
    return(GRAYLIST_LATER);
  }
  else
  {
    (*cv_report)(LOG_DEBUG,"","received na?");
    return(GRAYLIST_YEA);
  }
  ddt(0);
  return(GRAYLIST_YEA);
#endif
}

/*******************************************************************/

static void handler_sigalrm(int sig)
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

