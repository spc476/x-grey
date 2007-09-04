
#ifndef SERVER_H
#define SERVER_H

#define UDP_MAX		1500

struct request
{
  int                      sock;
  time_t                   now;
  struct sockaddr          remote;
  socklen_t                rsize;
  char                     packet[1500];
  struct graylist_request *glr;
  size_t                   size;
};

#endif


  
  
