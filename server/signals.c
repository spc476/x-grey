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

#include "../common/util.h"
#include "globals.h"
#include "iplist.h"
#include "signals.h"

/***********************************************************************/

static void     handle_sigchld          (void);
static void     handle_sigint           (void);
static void     handle_sigquit          (void);
static void     handle_sigterm          (void);
static void     handle_sigpipe          (void);
static void     handle_sigalrm          (void);
static void     handle_sigusr1          (void);
static void     handle_sigusr2          (void);
static void     handle_sighup           (void);

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
    syslog(LOG_ERR,"set a signal handler,but forgot to write code to handle it");
    mf_stupid = 0;
  }
}

/********************************************************************/

void sighandler_critical_child(int sig __attribute__((unused)))
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
  
  syslog(LOG_DEBUG,"child process ended");
  mf_sigchld = 0;
  
  while(1)
  {
    child = waitpid(-1,&status,WNOHANG);
    
    if (child == (pid_t)-1)
    {
      if (errno == ECHILD) return;
      if (errno == EINTR) continue;
      syslog(LOG_ERR,"waitpid() = %s",strerror(errno));
      return;
    }
    
    if (WIFEXITED(status))
    {
      ret = WEXITSTATUS(status);
      syslog(LOG_DEBUG,"Child process returned %d",ret);
    }
    else if (WIFSIGNALED(status))
    {
      ret = WTERMSIG(status);
      syslog(LOG_ERR,"Process %lu terminated by signal(%d)",(unsigned long)child,ret);
    }
    else if(WIFSTOPPED(status))
    {
      ret = WSTOPSIG(status);
      syslog(LOG_ERR,"Process %lu stopped by signal(%d)",(unsigned long)child,ret);
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
    syslog(LOG_CRIT,"fork() = %s",strerror(errno));
    return(pid);
  }
  else if (pid > (pid_t)0)      /* parent returns immediately */
    return(pid);
    
  /*---------------------------------------------------
  ; set the environment for the child process.
  ;--------------------------------------------------*/
  
  alarm(0);     /* turn off any pending alarms */
  
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
  syslog(LOG_DEBUG,"Interrupt Signal");
  
  save_state();
  GlobalsDeinit();
  set_signal(SIGINT,SIG_DFL);
  raise(SIGINT);
}

/************************************************************/

static void handle_sigquit(void)
{
  syslog(LOG_DEBUG,"Quit signal");
  
  save_state();
  GlobalsDeinit();
  set_signal(SIGQUIT,SIG_DFL);
  raise(SIGQUIT);
}

/**********************************************************/

static void handle_sigterm(void)
{
  syslog(LOG_DEBUG,"Terminate Signal");
  
  save_state();
  GlobalsDeinit();
  set_signal(SIGTERM,SIG_DFL);
  raise(SIGTERM);
}

/************************************************************/

static void handle_sigpipe(void)
{
  syslog(LOG_DEBUG,"one side closed their connection");
  mf_sigpipe = 0;
}

/***********************************************************/

static void handle_sigalrm(void)
{
  time_t now;
  
  syslog(LOG_DEBUG,"Alarm-time for house cleaning!");
  
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
    
  tuple_expire(now);
  
  if (difftime(now,g_time_savestate) >= c_time_savestate)
  {
    pid_t child;
    
    syslog(LOG_DEBUG,"saving state");
    
    g_time_savestate = now;
    child            = gld_fork();
    
    if (child == 0)     /* child process */
    {
      save_state();
      _exit(0);
    }
  }
  
  alarm(c_time_cleanup);        /* signal next cleanup */
}

/***********************************************************/

static void handle_sigusr1(void)
{
  pid_t  child;
  
  syslog(LOG_DEBUG,"User 1 Signal");
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
  syslog(LOG_DEBUG,"User 2 Signal");
  mf_sigusr2 = 0;
  tuple_expire(time(NULL));
}

/**********************************************************************/

static void handle_sighup(void)
{
  char *t;
  
  syslog(LOG_DEBUG,"Sighup");
  mf_sighup = 0;
  t = report_time(c_starttime,time(NULL));
  syslog(LOG_INFO,"%s",t);
  syslog(LOG_INFO,"tuples:            %10lu",(unsigned long)g_poolnum);
  syslog(LOG_INFO,"greylisted:        %10lu",(unsigned long)g_greylisted);
  syslog(LOG_INFO,"whitelisted:       %10lu",(unsigned long)g_whitelisted);
  syslog(LOG_INFO,"greylist expired:  %10lu",(unsigned long)g_greylist_expired);
  syslog(LOG_INFO,"whitelist expired: %10lu",(unsigned long)g_whitelist_expired);
  free(t);
}

/*********************************************************************/

