
#ifndef GRAYLIST_H
#define GRAYLIST_H

#include <time.h>

#define VERSION	0x0100

/********************************************************************/

typedef unsigned char  byte;
typedef unsigned long  IP;
typedef unsigned short Port;
typedef unsigned short unet16;

enum 
{
  MTA_SENDMAIL,
  MTA_QMAIL,
  MTA_POSTFIX,
  MTA_USER = 16
};

enum
{
  CMD_NONE,
  CMD_NONE_RESP,
  CMD_GRAYLIST,
  CMD_GRAYLIST_RESP,
  CMD_MAX
};

typedef struct graylist_response
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 response;
};

typedef struct graylist_request
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  uint16 ipsize;
  uint16 fromsize;
  uint16 tosize;
  byte   data[1];
};

typedef struct tuple
{
  time_t ctime;
  time_t atime;
  size_t fsize;
  size_t tsize;
  int    white;
  byte   ip  [16];
  char   from[114];
  char   to  [114];
} *Tuple;

/***************************************************************/

#endif
