
#ifndef GRAYLIST_H
#define GRAYLIST_H

typedef struct tuple
{
  time_t ctime;
  time_t atime;
  size_t fsize;
  size_t tsize;
  int    white;
  byte_t ip  [16];
  char   from[128];
  char   to  [128];
} *Tuple;


  
















#endif
