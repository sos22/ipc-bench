/* 
    Measure latency of IPC using tcp sockets

    Copyright (c) 2010 Erik Rigtorp <erik@rigtorp.com>
    Copyright (c) 2011 Anil Madhavapeddy <anil@recoil.org>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/time.h>
#include <err.h>
#include <inttypes.h>
#include <netdb.h>

#include "xutil.h"

int
main(int argc, char *argv[])
{
  int size;
  char *buf;
  int64_t count, i, delta;
  struct timeval start, stop;

  int yes = 1;
  int ret;
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints;
  struct addrinfo *res;
  int sockfd, new_fd;

  if (argc != 3) {
    printf ("usage: tcp_lat <message-size> <roundtrip-count>\n");
    return 1;
  }

  size = atoi(argv[1]);
  count = atol(argv[2]);

  buf = xmalloc(size);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
  if ((ret = getaddrinfo("127.0.0.1", "3491", &hints, &res)) != 0)
    errx(1, "getaddrinfo: %s\n", gai_strerror(ret));

  if (!xfork()) {  /* child */

    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
      err(1, "socket");

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      err(1, "setsockopt");
    
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
      err(1, "bind");
    
    if (listen(sockfd, 1) == -1)
      err(1, "listen");
    
    addr_size = sizeof their_addr;
    
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1)
      err(1, "accept");

    for (i = 0; i < count; i++) {
      xread(new_fd, buf, size);
      xwrite(new_fd, buf, size);
    }
  } else { /* parent */
   
    sleep(1); 
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
      err(1, "socket");
    
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
      err(1, "connect");

    gettimeofday(&start, NULL); 
    for (i = 0; i < count; i++) {
      xwrite(sockfd, buf, size);
      xread(sockfd, buf, size);
    }
    gettimeofday(&stop, NULL);
    delta = ((stop.tv_sec - start.tv_sec) * (int64_t) 1e6 +
	     stop.tv_usec - start.tv_usec);
    printf("tcp_lat %d %" PRId64 " %" PRId64 "\n", size, count, delta / (count * 2));
  }
  return 0;
}
