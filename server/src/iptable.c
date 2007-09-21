

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <syslog.h>

#include <cgilib/ddt.h>
#include <cgilib/memory.h>

#include "../../common/src/globals.h"
#include "../../common/src/util.h"

#include "globals.h"
#include "iplist.h"
#include "iptable.h"

/*************************************************************/

#if 0
static void		 ip_prune	(struct ipnode *,int);
#endif

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

/*************************************************************/

int ip_match(byte *ip,size_t size)
{
  struct ipnode *match;
  struct ipnode *p   = g_tree;
  int            off = -1;
  int            bit =  0;
  
  ddt(ip   != NULL);
  ddt(size == 4);
  
  while(p != NULL)
  {
    if (p->match != IPCMD_NONE)
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
      return (match->match);
    }
    
    bit >>= 1;
  }
  
  return(IPCMD_GRAYLIST);
}

/******************************************************************/

int ip_add_sm(byte *ip,size_t size,int mask,int cmd)
{
  struct ipnode *new;
  struct ipnode *p   = g_tree;
  int            off = -1;
  int            bit =  0;
  int            b;
  
  ddt(ip   != NULL);
  ddt(size == 4);
  ddt(mask >  -1);
  ddt(mask <  33);
  ddt(mask != 31);
  
  if (mask == 0)
  {
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
        new = malloc(sizeof(struct ipnode));
        new->parent = p;
        new->zero   = NULL;
        new->one    = NULL;
        new->count  = 0;
        new->match  = IPCMD_NONE;
        p->zero     = new;
      }
      p    = p->zero;
    }
    else
    {
      if (p->one == NULL)
      {
        new = malloc(sizeof(struct ipnode));
        new->parent = p;
        new->zero   = NULL;
        new->one    = NULL;
        new->count  = 0;
        new->match  = IPCMD_NONE;
        p->one      = new;
      }
      p    = p->one;
    }
    
    mask--;
    bit >>= 1;
  }
  
  if (p->match == IPCMD_NONE)
    g_ipcnt++;

  p->match = cmd;

#if 0
  ip_prune(p->zero,cmd);
  ip_prune(p->one,cmd);
#endif

  return(0);
}

/*******************************************************************/

#if 0
static void ip_prune(struct ipnode *p,int match)
{
  if (p == NULL)
    return;

  if (p->match == IPCMD_NONE)
  {
    ip_prune(p->zero,match);
    ip_prune(p->one,match);
    return;
  }
  
  if (p->match == match)
  {
    p->match = IPCMD_NONE;
    g_ipcnt--;
  }
}  
#endif

/******************************************************************/

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
      	ci_map_chars(array[i].cmd,c_ipcmds,C_IPCMDS),
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
  qsort(array,rc,sizeof(struct ipblock),iplist_cmp);
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
  
  if (p->match != IPCMD_NONE)
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
  
  mask[off] ^= mbit;
  ip  [off] ^= bit;
  return(i);
}

/********************************************************************/
