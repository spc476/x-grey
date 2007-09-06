
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>

#include <cgilib/memory.h>
#include <cgilib/stream.h>

#include "../../common/src/util.h"
#include "globals.h"
#include "iplist.h"

/***********************************************************************/

static void	handle_sigint		(void);
static void	handle_sigquit		(void);
static void	handle_sigterm		(void);
static void	handle_sigpipe		(void);
static void	handle_sigalrm		(void);
static void	handle_sigusr1		(void);
static void	handle_sigusr2		(void);
static void	handle_sighup		(void);

/***********************************************************************/

static volatile sig_atomic_t mf_sigint;
static volatile sig_atomic_t mf_sigquit;
static volatile sig_atomic_t mf_sigterm;
static volatile sig_atomic_t mf_sigpipe;
static volatile sig_atomic_t mf_sigalrm;
static volatile sig_atomic_t mf_sigusr1;
static volatile sig_atomic_t mf_sigusr2;
static volatile sig_atomic_t mf_sighup;

/**********************************************************************/

void check_signals(void)
{
  if (mf_sigalrm)  handle_sigalrm();
  if (mf_sigint)   handle_sigint(); 
  if (mf_sigquit)  handle_sigquit();
  if (mf_sigterm)  handle_sigterm();
  if (mf_sigpipe)  handle_sigpipe();
  if (mf_sigusr1)  handle_sigusr1();
  if (mf_sigusr2)  handle_sigusr2();
  if (mf_sighup)   handle_sighup();
}

/********************************************************************/

void sighandler_critical(int sig)
{
  extern char **environ;
  sigset_t      sigset;
  
  /*--------------------------------------------------------
  ; we've got a signal that would normally terminate with a 
  ; core dump.  Since this thing has to run, and keep running,
  ; we log the problem, close all open files, and re-exec
  ; ourselves.  Not the best thing, but better than getting a 
  ; call at 5:00 am in the morning!
  ;--------------------------------------------------------*/

  (*cv_report)(
       LOG_ERR,
       "$ p",
       "DANGER!  %a - restarting program",
       sys_siglist[sig]
    );

  whitelist_dump();	/* attempt to do this */
  
  /*---------------------------------------------------------
  ; unblock all signals.
  ;--------------------------------------------------------*/
 
  sigfillset(&sigset);
  sigprocmask(SIG_UNBLOCK,&sigset,NULL);
  
  execve(g_argv[0],g_argv,environ);
  (*cv_report)(LOG_ERR,"$ $","exec('%a') = %b",g_argv[0],strerror(errno));
  _exit(EXIT_FAILURE);	/* when all else fails! */
}

/******************************************************************/

void sighandler_sigs(int sig)
{
  switch(sig)
  {
    case SIGINT   : mf_sigint   = 1; break;
    case SIGQUIT  : mf_sigquit  = 1; break;
    case SIGTERM  : mf_sigterm  = 1; break;
    case SIGPIPE  : mf_sigpipe  = 1; break;
    case SIGALRM  : mf_sigalrm  = 1; break;
    case SIGUSR1  : mf_sigusr1  = 1; break;
    case SIGUSR2  : mf_sigusr2  = 1; break;
    case SIGHUP   : mf_sighup   = 1; break;
    default:
         (*cv_report)(LOG_ERR,"i","someone forgot to handle signal %a",sig);
         break;
  }
}

/***********************************************************/

void sighandler_chld(int sig)
{
  pid_t child;
  int   ret;
  int   pusherr;
  int   status;
  
  (*cv_report)(LOG_DEBUG,"","child process ended");

  if (sig != SIGCHLD)
  {
    (*cv_report)(LOG_CRIT,"i","sighandler_chld() 'Why am I handling signal %a",sig);
    return;
  }
  
  /*------------------------------------------
  ; since we do make a system call here, we
  ; cache the value of errno to prevent problems
  ; elsewhere in the code where we use it.  Then, just
  ; before we return, the saved value is stuffed
  ; back into errno.  Remember, this routine is called
  ; asynchronously with respect to the rest of the
  ; code in this program.
  ;----------------------------------------------*/
  
  pusherr = errno;
  child   = waitpid(0,&status,WNOHANG);
  
  if (child == (pid_t)-1)
  {
    (*cv_report)(LOG_ERR,"$","waitpid() returned %a",strerror(errno));
  }
  else if (WIFEXITED(status))
  {
    ret = WEXITSTATUS(status);
    (*cv_report)(LOG_DEBUG,"i","Child process returned %a",ret);
    if (ret != 0)
      (*cv_report)(LOG_ERR,"L i","Process %a returned status %b",child,ret); 
  }
  else if (WIFSIGNALED(status))
  {
    ret = WTERMSIG(status);
    (*cv_report)(LOG_ERR,"L i","Process %a terminated by signal(%b)",child,ret);
  }
  else if(WIFSTOPPED(status))
  {
    ret = WSTOPSIG(status);
    (*cv_report)(LOG_ERR,"L i","Process %a stopped by signal(%b)",child,ret);
  }
  errno = pusherr;
}

/**********************************************************************/

static void handle_sigint(void)
{
  (*cv_report)(LOG_DEBUG,"","Interrupt Signal");
  whitelist_dump();
  GlobalsDeinit();
  exit(EXIT_SUCCESS);
}

/************************************************************/

static void handle_sigquit(void)
{
  (*cv_report)(LOG_DEBUG,"","Quit signal");
  whitelist_dump();
  GlobalsDeinit();
  exit(EXIT_SUCCESS);
}

/**********************************************************/

static void handle_sigterm(void)
{
  (*cv_report)(LOG_DEBUG,"","Terminate Signal");
  whitelist_dump();
  GlobalsDeinit();
  exit(EXIT_SUCCESS);
}

/************************************************************/

static void handle_sigpipe(void)
{
  (*cv_report)(LOG_DEBUG,"","one side closed their connection");
  mf_sigpipe = 0;
}

/***********************************************************/

static void handle_sigalrm(void)
{
  (*cv_report)(LOG_DEBUG,"","Alarm-time for house cleaning!");
  mf_sigalrm = 0;
  tuple_expire(time(NULL));
  alarm(c_time_cleanup);	/* signal next cleanup */
}

/***********************************************************/

static void handle_sigusr1(void)
{
  Stream out;
  pid_t  child;
  size_t i;

  (*cv_report)(LOG_DEBUG,"","User 1 Signal");
  mf_sigusr1 = 0;
  
  child = fork();
  if (child == (pid_t)-1)
  {
    (*cv_report)(LOG_CRIT,"$","fork() = %a",strerror(errno));
    return;
  }
  
  if (child > 0)
    return;
  
  out = FileStreamWrite(c_dumpfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_dumpfile);
    _exit(0);
  }
  
  for (i = 0 ; i < g_poolnum ; i++)
  {
    LineSFormat(
    	out,
    	"$ $ $ $ $ $ $ $ L L",
    	"[%a , %b , %c] %d%e%f%g%h %i %j\n",
    	ipv4(g_tuplespace[i]->ip),
    	g_tuplespace[i]->from,
    	g_tuplespace[i]->to,
    	(g_tuplespace[i]->f & F_WHITELIST) ? "W" : "-",
    	(g_tuplespace[i]->f & F_GRAYLIST)  ? "G" : "-",
    	(g_tuplespace[i]->f & F_TRUNCFROM) ? "F" : "-",
    	(g_tuplespace[i]->f & F_TRUNCTO)   ? "T" : "-",
    	(g_tuplespace[i]->f & F_IPv6)      ? "6" : "-",
    	(unsigned long)g_tuplespace[i]->ctime,
    	(unsigned long)g_tuplespace[i]->atime
    );
  }
  
  StreamFree(out);
  _exit(0);
}

/***********************************************************************/

static void handle_sigusr2(void)
{
  (*cv_report)(LOG_DEBUG,"","User 2 Signal");
  mf_sigusr2 = 0;
  tuple_expire(time(NULL));
}

/**********************************************************************/

static void handle_sighup(void)
{
  (*cv_report)(LOG_DEBUG,"","Sighup");
  mf_sighup = 0;
  (*cv_report)(LOG_INFO,"L10","tuples:            %a",(unsigned long)g_poolnum);
  (*cv_report)(LOG_INFO,"L10","graylisted:        %a",(unsigned long)g_graylisted);
  (*cv_report)(LOG_INFO,"L10","whitelisted:       %a",(unsigned long)g_whitelisted);
  (*cv_report)(LOG_INFO,"L10","graylist expired:  %a",(unsigned long)g_graylist_expired);
  (*cv_report)(LOG_INFO,"L10","whitelist expired: %a",(unsigned long)g_whitelist_expired);
}

/*********************************************************************/

