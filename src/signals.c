
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

#include "globals.h"

/***********************************************************************/

static void	handle_sigint		(void);
static void	handle_sigquit		(void);
static void	handle_sigterm		(void);
static void	handle_sigpipe		(void);

/***********************************************************************/

static volatile sig_atomic_t mf_sigint;
static volatile sig_atomic_t mf_sigquit;
static volatile sig_atomic_t mf_sigterm;
static volatile sig_atomic_t mf_sigpipe;

/**********************************************************************/

void set_extsignal(int sig,void (*handler)(int,siginfo_t *,void *))
{
  struct sigaction act;
  struct sigaction oact;
  int              rc;
  
  sigemptyset(&act.sa_mask);
  act.sa_handler = (void (*)(int))handler;
  act.sa_flags   = SA_SIGINFO;
  
  rc = sigaction(sig,&act,&oact);
  if (rc == -1)
  {
    (*cv_report)(LOG_ERR,"$","sigaction() returned %a",strerror(errno));
    exit(EXIT_FAILURE);
  }
}

/*******************************************************************/

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

void check_signals(void)
{
  if (mf_sigint)   handle_sigint(); 
  if (mf_sigquit)  handle_sigquit();
  if (mf_sigterm)  handle_sigterm();
  if (mf_sigpipe)  handle_sigpipe();
}

/********************************************************************/

void sighandler_critical(int sig,siginfo_t *si,void *dummy)
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
       "DANGER!  %a @ %b - restarting program",
       sys_siglist[sig],
       si->si_addr
    );

  /*---------------------------------------------------------
  ; unblock all signals.
  ;--------------------------------------------------------*/
 
  sigfillset(&sigset);
  sigprocmask(SIG_UNBLOCK,&sigset,NULL);
  
  /*---------------------------------------------------------
  ; close everything down - I'm marking all file descriptors
  ; as "close on exec" so this shouldn't be needed.
  ;---------------------------------------------------------*/

#if 0
  {
    int i;

    for (i = 3 ; (i < OPEN_MAX) && (close(i) == 0) ; i++)
      ;
  }
#endif

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
    default:
         (*cv_report)(LOG_ERR,"i","someone forgot to handle signal %a",signal);
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
  GlobalsDeinit();
  exit(EXIT_SUCCESS);
}

/************************************************************/

static void handle_sigquit(void)
{
  (*cv_report)(LOG_DEBUG,"","Quit signal");
  GlobalsDeinit();
  exit(EXIT_SUCCESS);
}

/**********************************************************/

static void handle_sigterm(void)
{
  (*cv_report)(LOG_DEBUG,"","Terminate Signal");
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

