/************************************************************************
*
* Copyright 2007 by Sean Conner.  All Rights Reserved.
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
#include <string.h>

#include <cgilib/stream.h>
#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/util.h>

#include "graylist.h"
#include "globals.h"

/***************************************************************/

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
    strftime(txt_start,BUFSIZ,c_timestamp,ptime);
  else
    ptstart = "[ERROR in timestamp]";
  
  ptime = localtime(&end);
  if (ptime)
    strftime(txt_end,BUFSIZ,c_timestamp,ptime);
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

