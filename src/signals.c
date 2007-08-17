/************************************************************************
*
* Copyright 2006 by Sean Conner.  All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*************************************************************************/

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

#include "ltpstat.h"
#include "globals.h"

/***********************************************************************/

static void	handle_sigint		(void);
static void     handle_sigtrap          (void);
static void	handle_sighup		(void);
static void	handle_sigquit		(void);
static void	handle_sigterm		(void);
static void	handle_sigusr1		(void);
static void	handle_sigusr2		(void);
static void	handle_sigwinch		(void);

/***********************************************************************/

volatile sig_atomic_t mf_sigint;
volatile sig_atomic_t mf_sighup;
volatile sig_atomic_t mf_sigtrap;
volatile sig_atomic_t mf_sigquit;
volatile sig_atomic_t mf_sigterm;
volatile sig_atomic_t mf_sigusr1;
volatile sig_atomic_t mf_sigusr2;
volatile sig_atomic_t mf_sigwinch;

/**********************************************************************/

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
  if (mf_sighup)   handle_sighup();
  if (mf_sigtrap)  handle_sigtrap();
  if (mf_sigquit)  handle_sigquit();
  if (mf_sigterm)  handle_sigterm();
  if (mf_sigusr1)  handle_sigusr1();
  if (mf_sigusr2)  handle_sigusr2();
  if (mf_sigwinch) handle_sigwinch();
}

/***********************************************************/

void sighandler_sigs(int sig)
{
  switch(sig)
  {
    case SIGINT   : mf_sigint   = 1; break;
    case SIGHUP   : mf_sighup   = 1; break;
    case SIGTRAP  : mf_sigtrap  = 1; break;
    case SIGQUIT  : mf_sigquit  = 1; break;
    case SIGTERM  : mf_sigterm  = 1; break;
    case SIGUSR1  : mf_sigusr1  = 1; break;
    case SIGUSR2  : mf_sigusr2  = 1; break;
    case SIGWINCH : mf_sigwinch = 1; break;
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
  GlobalDeinit();
  exit(EXIT_SUCCESS);
}

/************************************************************/

static void handle_sighup(void)
{
  char   *elapse;
  
  elapse = report_time(c_starttime,time(NULL));

  (*cv_report)(LOG_DEBUG,"","Hangup Signal");
  (*cv_report)(LOG_INFO,"$","%a",elapse);
  
  MemFree(elapse);
  mf_sighup = 0;
}

/**********************************************************/

static void handle_sigtrap(void)
{
  (*cv_report)(LOG_DEBUG,"","Hangup Signal");
  mf_sigtrap = 0;
}

/************************************************************/

static void handle_sigquit(void)
{
  (*cv_report)(LOG_DEBUG,"","Quit signal");
  GlobalDeinit();
  exit(EXIT_SUCCESS);
}

/**********************************************************/

static void handle_sigterm(void)
{
  (*cv_report)(LOG_DEBUG,"","Terminate Signal");
  GlobalDeinit();
  exit(EXIT_SUCCESS);
}

/************************************************************/

static void handle_sigusr1(void)
{
  Stream out;
  pid_t child;
  
  (*cv_report)(LOG_DEBUG,"","User 1 Signal");
  mf_sigusr1 = 0;

  child      = fork();
  
  if (child == (pid_t)-1)
  {
    (*cv_report)(LOG_CRIT,"$","fork() returned %a",strerror(errno));
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
  generate_dump(out);
  StreamFree(out);
  _exit(0);
}

/*************************************************************/

static void handle_sigusr2(void)
{
  Stream out;
  pid_t  child;
  
  (*cv_report)(LOG_DEBUG,"","User 2 Signal");
  
  mf_sigusr2 = 0;
  child      = fork();
  
  if (child == (pid_t)-1)
  {
    (*cv_report)(LOG_CRIT,"$","fork() returned %a",strerror(errno));
    return;
  }

  if (child > 0)
    return;

  out = FileStreamWrite(c_reportfile,FILE_CREATE | FILE_TRUNCATE);
  generate_report(out);
  StreamFree(out);
  _exit(0);
}

/****************************************************************/

static void handle_sigwinch(void)
{
  Stream out;
  pid_t  child;
  
  (*cv_report)(LOG_DEBUG,"","Window Change Signal");
  
  mf_sigwinch = 0;
  child       = fork();
  
  if (child == (pid_t)-1)
  {
    (*cv_report)(LOG_CRIT,"$","fork() returned %a",strerror(errno));
    return;
  }
  
  if (child > 0)
    return;
  
  out = FileStreamWrite(c_ipnumfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_ipnumfile);
    _exit(0);
  }
  
  generate_ipnum(out);
  StreamFree(out);
  
  out = FileStreamWrite(c_portfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_portfile);
    _exit(0);
  }
  
  generate_ports(out);
  StreamFree(out);
  
  _exit(0);
}

/*****************************************************************/

