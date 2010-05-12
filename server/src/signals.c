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

/********************************************************************
*
* _POSIX_SOURCE gets us sigset_t
* _BSD_SOURCE gets us sys_siglist
*
********************************************************************/

#define _POSIX_SOURCE
#define _BSD_SOURCE

#ifndef __GNUC__
#  define __attribute__(x)
#endif

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cgilib/memory.h>
#include <cgilib/stream.h>

#include "../../common/src/util.h"
#include "globals.h"
#include "iplist.h"
#include "signals.h"

/***********************************************************************/

static void	handle_sigchld		(void);
static void	handle_sigint		(void);
static void	handle_sigquit		(void);
static void	handle_sigterm		(void);
static void	handle_sigpipe		(void);
static void	handle_sigalrm		(void);
static void	handle_sigusr1		(void);
static void	handle_sigusr2		(void);
static void	handle_sighup		(void);

/***********************************************************************/

static volatile sig_atomic_t mf_sigchld;
static volatile sig_atomic_t mf_sigint;
static volatile sig_atomic_t mf_sigquit;
static volatile sig_atomic_t mf_sigterm;
static volatile sig_atomic_t mf_sigpipe;
static volatile sig_atomic_t mf_sigalrm;
static volatile sig_atomic_t mf_sigusr1;
static volatile sig_atomic_t mf_sigusr2;
static volatile sig_atomic_t mf_sighup;
static volatile sig_atomic_t mf_stupid;

/**********************************************************************/

void check_signals(void)
{
  if (mf_sigchld)  handle_sigchld();
  if (mf_sigalrm)  handle_sigalrm();
  if (mf_sigint)   handle_sigint(); 
  if (mf_sigquit)  handle_sigquit();
  if (mf_sigterm)  handle_sigterm();
  if (mf_sigpipe)  handle_sigpipe();
  if (mf_sigusr1)  handle_sigusr1();
  if (mf_sigusr2)  handle_sigusr2();
  if (mf_sighup)   handle_sighup();
  if (mf_stupid)
  {
    (*cv_report)(
    		LOG_ERR,
		"",
    		"set a signal handler,but forgot to write code to handle it"
    	);
    mf_stupid = 0;
  }
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
       "$",
       "DANGER!  %a - restarting program",
       sys_siglist[sig]
    );

  save_state();

  /*---------------------------------------------------------
  ; unblock all signals.
  ;--------------------------------------------------------*/
 
  sigfillset(&sigset);
  sigprocmask(SIG_UNBLOCK,&sigset,NULL);
  
  execve((const char *)g_argv[0],(char **const)g_argv,environ);
  (*cv_report)(LOG_ERR,"$ $","exec('%a') = %b",g_argv[0],strerror(errno));
  exit(EXIT_FAILURE);	/* when all else fails! */
}

/******************************************************************/

void sighandler_critical_child(int sig __attribute((unused)))
{
  /*----------------------------------------------
  ; just exit, no logging for now
  ;-----------------------------------------------*/
  
  _exit(EXIT_FAILURE);
}

/*****************************************************************/

void sighandler_sigs(int sig)
{
  switch(sig)
  {
    case SIGCHLD  : mf_sigchld  = 1; break;
    case SIGINT   : mf_sigint   = 1; break;
    case SIGQUIT  : mf_sigquit  = 1; break;
    case SIGTERM  : mf_sigterm  = 1; break;
    case SIGPIPE  : mf_sigpipe  = 1; break;
    case SIGALRM  : mf_sigalrm  = 1; break;
    case SIGUSR1  : mf_sigusr1  = 1; break;
    case SIGUSR2  : mf_sigusr2  = 1; break;
    case SIGHUP   : mf_sighup   = 1; break;
    default:        mf_stupid   = 1; break;
  }
}

/***********************************************************/

void handle_sigchld(void)
{
  pid_t child;
  int   ret;
  int   status;
  
  (*cv_report)(LOG_DEBUG,"","child process ended");
  mf_sigchld = 0;

  while(1)
  {
    child = waitpid(-1,&status,WNOHANG);
  
    if (child == (pid_t)-1)
    {
      if (errno == ECHILD) return;
      if (errno == EINTR) continue;
      (*cv_report)(LOG_ERR,"$","waitpid() returned %a",strerror(errno));
      return;
    }
  
    if (WIFEXITED(status))
    {
      ret = WEXITSTATUS(status);
      (*cv_report)(LOG_DEBUG,"i","Child process returned %a",ret);
    }
    else if (WIFSIGNALED(status))
    {
      ret = WTERMSIG(status);
      (*cv_report)(LOG_ERR,"L i","Process %a terminated by signal(%b)",(unsigned long)child,ret);
    }
    else if(WIFSTOPPED(status))
    {
      ret = WSTOPSIG(status);
      (*cv_report)(LOG_ERR,"L i","Process %a stopped by signal(%b)",(unsigned long)child,ret);
    }
  }
}

/**********************************************************************/

pid_t gld_fork(void)
{
  pid_t pid;
  
  pid = fork();
  if (pid == (pid_t)-1)
  {
    (*cv_report)(LOG_CRIT,"$","fork() = %a",strerror(errno));
    return(pid);
  }
  else if (pid > (pid_t)0)	/* parent returns immediately */
    return(pid);
  
  /*---------------------------------------------------
  ; set the environment for the child process.
  ;--------------------------------------------------*/

  alarm(0);	/* turn off any pending alarms */
  
  set_signal(SIGSEGV,sighandler_critical_child);
  set_signal(SIGBUS, sighandler_critical_child);
  set_signal(SIGFPE, sighandler_critical_child);
  set_signal(SIGILL, sighandler_critical_child);
  set_signal(SIGXCPU,sighandler_critical_child);
  set_signal(SIGXFSZ,sighandler_critical_child);
  set_signal(SIGPIPE,sighandler_critical_child);

  return(pid);
}

/*****************************************************************/

void save_state(void)
{
  whitelist_dump();
  iplist_dump();
  to_dump();
  tod_dump();
  from_dump();
  fromd_dump();
}

/*******************************************************************/ 
 
static void handle_sigint(void)
{
  (*cv_report)(LOG_DEBUG,"","Interrupt Signal");

  save_state();
  GlobalsDeinit();
  exit(EXIT_SUCCESS);
}

/************************************************************/

static void handle_sigquit(void)
{
  (*cv_report)(LOG_DEBUG,"","Quit signal");

  save_state();
  GlobalsDeinit();
  exit(EXIT_SUCCESS);
}

/**********************************************************/

static void handle_sigterm(void)
{
  (*cv_report)(LOG_DEBUG,"","Terminate Signal");

  save_state();
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
  time_t now;
  
  (*cv_report)(LOG_DEBUG,"","Alarm-time for house cleaning!");

  mf_sigalrm      = 0;
  now             = time(NULL);
  g_cleanup_count++;
  
  g_req_cu        = g_req_cucurrent;
  g_req_cucurrent = 0;
  if (g_req_cu > g_req_cumax)
    g_req_cumax = g_req_cu;
    
  g_tuples_read_cu        = g_tuples_read_cucurrent;
  g_tuples_read_cucurrent = 0;
  if (g_tuples_read_cu > g_tuples_read_cumax)
    g_tuples_read_cumax = g_tuples_read_cu;
    
  g_tuples_write_cu        = g_tuples_write_cucurrent;
  g_tuples_write_cucurrent = 0;
  if (g_tuples_write_cu > g_tuples_write_cumax)
    g_tuples_write_cumax = g_tuples_write_cu;

  (*cv_report)(LOG_INFO,"L","cu-expire: %a",(unsigned long)g_req_cu);

  tuple_expire(now);

  if (difftime(now,g_time_savestate) >= c_time_savestate)
  {
    pid_t child;
  
    (*cv_report)(LOG_DEBUG,"","saving state");
  
    g_time_savestate = now;
    child            = gld_fork();
    
    if (child == 0)	/* child process */
    {
      save_state();
      _exit(0);
    }
  }  

  alarm(c_time_cleanup);	/* signal next cleanup */
}

/***********************************************************/

static void handle_sigusr1(void)
{
  pid_t  child;

  (*cv_report)(LOG_DEBUG,"","User 1 Signal");
  mf_sigusr1 = 0;

  child = gld_fork();

  if (child == 0)
  {
    tuple_all_dump();
    _exit(0);
  }
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
  char *t;

  (*cv_report)(LOG_DEBUG,"","Sighup");
  mf_sighup = 0;
  t = report_time(c_starttime,time(NULL));
  (*cv_report)(LOG_INFO,"$","%a",t);
  (*cv_report)(LOG_INFO,"L10","tuples:            %a",(unsigned long)g_poolnum);
  (*cv_report)(LOG_INFO,"L10","greylisted:        %a",(unsigned long)g_greylisted);
  (*cv_report)(LOG_INFO,"L10","whitelisted:       %a",(unsigned long)g_whitelisted);
  (*cv_report)(LOG_INFO,"L10","greylist expired:  %a",(unsigned long)g_greylist_expired);
  (*cv_report)(LOG_INFO,"L10","whitelist expired: %a",(unsigned long)g_whitelist_expired);
  MemFree(t);
}

/*********************************************************************/

