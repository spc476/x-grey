/***************************************************************************
*
* Copyright 2007 by Sean Conner.
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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <syslog.h>

#include <cgilib/stream.h>
#include <cgilib/memory.h>
#include <cgilib/util.h>
#include <cgilib/errors.h>
#include <cgilib/ddt.h>

#include "../../common/src/globals.h"
#include "../../common/src/util.h"
#include "globals.h"
#include "iplist.h"

/*****************************************************************/

static size_t	         ip_collect	(
				  	  byte *,
				  	  byte *,
				  	  size_t,
				  	  struct ipnode *,
				  	  int,
				  	  int,
				          int,
				  	  struct ipblock *,
				  	  size_t
					);

/*****************************************************************/

int iplist_read(const char *fname)
{
  byte           ip   [16];
  Stream         in;
  char          *line;
  char          *tline;
  int            lcount = 0;
  size_t         octetcount;
  int            mask;
  int            cmd;
  String        *tokens;
  size_t         numtoks;
  
  ddt(fname != NULL);
  
  in = FileStreamRead(fname);
  if (in == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",fname);
    return(ERR_ERR);
  }
  
  line = dup_string("");
  
  while(!StreamEOF(in))
  {
    MemFree(line);
    line  = LineSRead(in);    
    line  = trim_space(line);
    lcount++;

    if (empty_string(line)) 
      continue;    

    if (line[0] == '#') 
      continue;	/* comments */

    up_string(line);
    
    tokens = split(&numtoks,line);
    if (!isdigit(tokens[0].d[0]))  
    {
      (*cv_report)(LOG_ERR,"$ i","syntax error on %a(%b)-needs to be an IP address",fname,lcount);
      MemFree(line);
      StreamFree(in);
      return(ERR_ERR);
    }
    
    octetcount  = 0;  
    tline       = tokens[0].d;
    
    do
    {
      ip[octetcount] = strtoul(tline,&tline,10);
      if (tline[0] == '\0') break;
      if (tline[0] == '/') break;
      if (tline[0] != '.')
      {
        (*cv_report)(LOG_ERR,"$ i","syntax error on %a(%b)-needs to be an IP address",fname,lcount);
        MemFree(line);
        StreamFree(in);
        return(ERR_ERR);
      }
      
      octetcount ++;
      tline++;
    } while(octetcount < 4);
    
    if (*tline++ == '/')
      mask = strtoul(tline,&tline,10);
    else 
      mask = 32;
    
    if (mask > 32)
    {
      (*cv_report)(LOG_ERR,"$ i i","syntax error on %a(%b)-bad mask value %c",fname,lcount,mask);
      MemFree(line);
      StreamFree(in);
      return(ERR_ERR);
    }

    cmd = ci_map_int(tokens[1].d,c_ift,C_IFT);
    if (cmd == -1)    
    {
      (*cv_report)(LOG_ERR,"$ i $","syntax error on %a(%b)-needs to be ACCEPT or REJECT but not %c",fname,lcount,tokens[1].d);
      MemFree(line);
      StreamFree(in);
      return(ERR_ERR);
    }

    ip_add_sm(ip,4,mask,cmd);
    MemFree(tokens);
  }

  MemFree(line);
  StreamFree(in);

  if (cf_debug)
  {
    struct ipblock *iplist;
    size_t          ipsize;
    size_t          i;
    
    iplist = ip_table(&ipsize);

    for (i = 0 ; i < ipsize ; i++)
    {
      char        tip  [20];
      char        tmask[20];
      char       *t;
      const char *tcmd;
      
      t = ipv4(iplist[i].addr);
      strcpy(tip,t);
      t = ipv4(iplist[i].mask);
      strcpy(tmask,t);
      
      tcmd = ci_map_chars(iplist[i].cmd,c_ift,C_IFT);
      
      (*cv_report)(
      		LOG_DEBUG,
      		"$8.8l $15.15l $15.15l",
      		"%a %b %c",
      		tcmd,
		tip,
		tmask
    	);
    }

    MemFree(iplist);
  }
  
  return(ERR_OKAY);
}

/**************************************************************/

int iplist_dump(void)
{
  Stream out;

  out = FileStreamWrite(c_iplistfile,FILE_CREATE | FILE_TRUNCATE);
  if (out == NULL)
  {
    (*cv_report)(LOG_ERR,"$","could not open %a",c_iplistfile);
    return(ERR_ERR);
  }
  
  iplist_dump_stream(out);
  
  StreamFree(out);
  return(ERR_OKAY);
}

/*****************************************************************/

int iplist_dump_stream(Stream out)
{
  struct ipblock *array;
  size_t          asize;
  size_t          i;
  char            ipaddr[20];
  
  array = ip_table(&asize);
  
  for (i = 0 ; i < asize ; i++)
  {
    sprintf(ipaddr,"%s/%d",ipv4(array[i].addr),array[i].smask);

    LineSFormat(
    	out,
    	"$18.18l $",
    	"%a %b\n",
	ipaddr,
    	ci_map_chars(array[i].cmd,c_ift,C_IFT)
    );
  }

  MemFree(array);  
  return(ERR_OKAY);
}

/*************************************************************/

int ip_match(byte *ip,size_t size __attribute__((unused)))
{
  struct ipnode *match = g_tree;
  struct ipnode *p     = g_tree;
  int            off   = -1;
  int            bit   =  0;
  
  ddt(ip   != NULL);
  ddt(size == 4);
  
  while(p != NULL)
  {
    if (p->match != IFT_NONE)
      match = p;
    
    if (bit == 0)
    {
      bit = 0x80;
      off++;
    }
    
    if (((ip[off] & bit) == 0) && (p->zero != NULL))
      p = p->zero;
    else if (((ip[off] & bit) != 0) && (p->one != NULL))
      p = p->one;
    else
    {
      match->count++;
      ddt(match->match != IFT_NONE);
      return (match->match);
    }
    
    bit >>= 1;
  }

  return(g_tree->match);
}

/******************************************************************/

int ip_add_sm(byte *ip,size_t size __attribute__((unused)),int mask,int cmd)
{
  struct ipnode *new;
  struct ipnode *p     = g_tree;
  int            off   = -1;
  int            bit   =  0;
  int            b;
  
  ddt(ip   != NULL);
  ddt(size == 4);
  ddt(mask >  -1);
  ddt(mask <  33);

  /*----------------------------------
  ; if the mask is 0, we don't support
  ; removing the default match.  Sorry
  ;----------------------------------*/
  
  if (mask == 0)
  {
    if (cmd != IFT_REMOVE)
      g_tree->match = cmd;
    return(0);
  }

  while(mask)
  {
    if (bit == 0)
    {
      bit = 0x80;
      off++;
    }
    
    b = ip[off] & bit;
    
    if (b == 0)
    {
      if (p->zero == NULL)
      {
        new         = MemAlloc(sizeof(struct ipnode));
        new->parent = p;
        new->zero   = NULL;
        new->one    = NULL;
        new->count  = 0;
        new->match  = IFT_NONE;
        p->zero     = new;
      }
      p    = p->zero;
    }
    else
    {
      if (p->one == NULL)
      {
        new         = MemAlloc(sizeof(struct ipnode));
        new->parent = p;
        new->zero   = NULL;
        new->one    = NULL;
        new->count  = 0;
        new->match  = IFT_NONE;
        p->one      = new;
      }
      p    = p->one;
    }
    
    mask--;
    bit >>= 1;
  }
  
  if (p->match == IFT_NONE)
  {
    if (cmd != IFT_REMOVE)
      g_ipcnt++;
    else
      return(0);
  }

  if (cmd == IFT_REMOVE)
  {
    cmd = IFT_NONE;
    g_ipcnt--;
  }

  p->match = cmd;
  return(0);
}

/*******************************************************************/

int ip_print(Stream out)
{
  struct ipblock *array;
  size_t          asize;
  size_t          i;
  char            tip  [20];
  char            tmask[20];
  char           *t;

  array = ip_table(&asize);
    
  for (i = 0 ; (!StreamEOF(out)) && (i < asize) ; i++)
  {
    t = ipv4(array[i].addr);
    strcpy(tip,t);
    t = ipv4(array[i].mask);
    strcpy(tmask,t);
      
    LineSFormat(
      	out,
      	"$15.15l $15.15l $8.8l L10",
      	"%d %c %a %b\n",
      	tip,
      	tmask,
      	ci_map_chars(array[i].cmd,c_ift,C_IFT),
      	(unsigned long)array[i].count
      );
  }
  MemFree(array);
  return(ERR_OKAY);
}

/*******************************************************************/

struct ipblock *ip_table(size_t *ps)
{
  struct ipblock *array;
  byte            addr[16];
  byte            mask[16];
  size_t          rc;

  ddt(ps != NULL);
  
  rc             = 1;
  array          = MemAlloc(g_ipcnt * sizeof(struct ipblock));
  array[0].size  = 4;
  array[0].count = g_tree->count;
  array[0].smask = 0;
  array[0].cmd   = g_tree->match;
  memset(array[0].mask,0,4);
  memset(array[0].addr,0,4);

  memset(addr,0,sizeof(addr));
  memset(mask,0,sizeof(mask));
  if (g_tree->zero)
    rc = ip_collect(addr,mask,4,g_tree->zero,0,0x80,   0,array,1);

  memset(addr,0,sizeof(addr));
  memset(mask,0,sizeof(mask));
  if (g_tree->one)
    rc = ip_collect(addr,mask,4,g_tree->one, 0,0x80,0x80,array,rc);

  (*cv_report)(LOG_DEBUG,"L L","g: %a rc: %b",g_ipcnt,rc);
  *ps = rc;
  return(array);
}

/*******************************************************************/

static size_t ip_collect(
                   byte           *ip,
                   byte           *mask,
                   size_t          size,
                   struct ipnode  *p,
                   int             off,
                   int             mbit,
                   int             bit,
                   struct ipblock *array,
		   size_t          i
                 )
{
  int nmb;
  int noff;
  
  ddt(ip    != NULL);
  ddt(mask  != NULL);
  ddt(size  == 4);
  ddt(p     != NULL);
  ddt(off   >= 0);
  ddt(off   <   4);
  ddt(array != NULL);
  ddt(
             (bit == 0x00)
  	  || (bit == 0x01)
  	  || (bit == 0x02)
  	  || (bit == 0x04)
  	  || (bit == 0x08)
  	  || (bit == 0x10)
  	  || (bit == 0x20)
  	  || (bit == 0x40)
  	  || (bit == 0x80)
  	);
  ddt(
             (mbit == 0x01)
          || (mbit == 0x02)
          || (mbit == 0x04)
          || (mbit == 0x08)
          || (mbit == 0x10)
          || (mbit == 0x20)
          || (mbit == 0x40)
          || (mbit == 0x80)
        );

  mask[off] |= mbit;
  ip[off]   |= bit;

  noff = off;
  nmb  = mbit >> 1;
  if (nmb == 0) 
  {
    nmb = 0x80;
    noff ++;
  }
  
  if (p->zero)
    i = ip_collect(ip,mask,4,p->zero,noff,nmb,0,array,i);
    
  if (p->one)
    i = ip_collect(ip,mask,4,p->one,noff,nmb,nmb,array,i);

  if (p->match != IFT_NONE)
  {
    array[i].size  = size;
    array[i].count = p->count;
    array[i].cmd   = p->match;
    array[i].smask = (off * 8)
    			+ ((mask[off] & 0x80) != 0)
    			+ ((mask[off] & 0x40) != 0)
    			+ ((mask[off] & 0x20) != 0)
    			+ ((mask[off] & 0x10) != 0)
    			+ ((mask[off] & 0x08) != 0)
    			+ ((mask[off] & 0x04) != 0)
    			+ ((mask[off] & 0x02) != 0)
    			+ ((mask[off] & 0x01) != 0);
    memcpy(array[i].mask,mask,size);
    memcpy(array[i].addr,ip,  size);
    i++;
  }
  
  mask[off] ^= mbit;
  ip  [off] ^= bit;
  return(i);
}

/********************************************************************/
