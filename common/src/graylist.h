
#ifndef GRAYLIST_COMMON_H
#define GRAYLIST_COMMON_H

#include <time.h>

#define VERSION		0x0100

#define DEF_HOST	"localhost"
#define DEF_PORT	9990

/********************************************************************/

typedef unsigned char  byte;
typedef unsigned long  IP;
typedef unsigned long  flags;
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

enum
{
  GLERR_OKAY,
  GLERR_VERSION_NOT_SUPPORTED,
  GLERR_MTA_NOT_SUPPORTED,
  GLERR_TYPE_NOT_SUPPORTED,
  GLERR_BAD_DATA,
  GLERR_MAX
};

enum
{
  GRAYLIST_NAY,
  GRAYLIST_YEA
};

struct graylist_response
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 response;
};

struct graylist_request
{
  unet16 version;
  unet16 MTA;
  unet16 type;
  unet16 ipsize;
  unet16 fromsize;
  unet16 tosize;
  byte   data[1];
};

#define F_WHITELIST	(1uL << 0)
#define F_GRAYLIST	(1uL << 1)
#define F_TRUNCFROM	(1uL << 2)
#define F_TRUNCTO	(1uL << 3)
#define F_IPv6		(1uL << 4)

typedef struct tuple
{
  time_t       ctime;
  time_t       atime;
  size_t       fromsize;
  size_t       tosize;
  flags        f;
  byte         ip  [16];
  char         from[114];
  char         to  [114];
} *Tuple;

/***************************************************************/

#endif
