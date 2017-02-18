#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "main.h"

static int sfd;

int simple_write(volatile uint32_t *const addr, int data)
{
  char buf[30];
  sprintf(buf, "W%p,%.8X,1", addr, data);
  write(sfd, buf, strlen(buf)+1);
}

int simple_read(volatile uint32_t *const addr)
{
  char buf[30];
  sprintf(buf, "R%p", addr);
  write(sfd, buf, strlen(buf)+1);
}

int client_main(char *host, char *port)
       {
           struct addrinfo hints;
           struct addrinfo *result, *rp;
           int s;

           /* Obtain address(es) matching host/port */

           memset(&hints, 0, sizeof(struct addrinfo));
           hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
           hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
           hints.ai_flags = 0;
           hints.ai_protocol = 0;          /* Any protocol */

           s = getaddrinfo(host, port, &hints, &result);
           if (s != 0) {
               fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
               exit(EXIT_FAILURE);
           }

           /* getaddrinfo() returns a list of address structures.
              Try each address until we successfully connect(2).
              If socket(2) (or connect(2)) fails, we (close the socket
              and) try the next address. */

           for (rp = result; rp != NULL; rp = rp->ai_next) {
               sfd = socket(rp->ai_family, rp->ai_socktype,
                            rp->ai_protocol);
               if (sfd == -1)
                   continue;
               if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
                   break;                  /* Success */

               close(sfd);
           }

           if (rp == NULL) {               /* No address succeeded */
               fprintf(stderr, "Could not connect\n");
               exit(EXIT_FAILURE);
           }

           freeaddrinfo(result);           /* No longer needed */

	   simple_write(led_base,0x55);
	   simple_write(led_base,0xAA);
	   simple_write(led_base,0x00);
	   simple_read(sd_base+5);
	   return sfd;
       }

static rw_struct_t rw_struct;

int client_write(uint64_t addr, int bytes, const uint8_t *ibuf)
{
  int nread = bytes+__builtin_offsetof (rw_struct_t, iobuf);
  memset(&rw_struct, 0, sizeof(rw_struct_t));
  rw_struct.cmd = 'w';
  rw_struct.addr = addr;
  rw_struct.bytes = bytes;
  memcpy(rw_struct.iobuf, ibuf, bytes);
  if (memcmp(rw_struct.iobuf, ibuf, bytes))
    abort();
  if (write(sfd, &rw_struct, nread) != nread) {
    perror("write");
    return -1;
  }
  usleep(10000);
  nread = read(sfd, rw_struct.iobuf, 0);
  if (nread == -1) {
    perror("read");
    return -1;
  }
  return 0;
}

int client_read(uint64_t addr, int bytes, uint8_t *obuf)
{
  int cnt, nread = __builtin_offsetof (rw_struct_t, iobuf);
  memset(&rw_struct, 0, sizeof(rw_struct_t));
  rw_struct.cmd = 'r';
  rw_struct.addr = addr;
  rw_struct.bytes = bytes;
  if (write(sfd, &rw_struct, nread) != nread) {
    perror("write");
    return -1;
  }
  cnt = 0;
  do {
    usleep(10000);
    nread = read(sfd, obuf+cnt, bytes-cnt);
    if (nread == -1) {
      perror("read");
      return -1;
    }
    cnt += nread;
  } while (cnt < bytes);

  return 0;
}
