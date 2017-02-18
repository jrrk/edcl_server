#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "main.h"

void dump(int nread, uint8_t *buf)
{
  int i;
  for (i = 0; i < nread; i++)
    {
      if ((i&15)==0) printf("\n%.3x:", i);
      printf(" %.2x", buf[i]);
    }
  putchar('\n');
}

static rw_struct_t rw_struct;

int main(int argc, char *argv[])
       {
           struct addrinfo hints;
           struct addrinfo *result, *rp;
           int sfd, s;
           struct sockaddr_storage peer_addr;
           socklen_t peer_addr_len;
           ssize_t nread;

           if (argc != 2) {
               fprintf(stderr, "Usage: %s port\n", argv[0]);
               exit(EXIT_FAILURE);
           }

           memset(&hints, 0, sizeof(struct addrinfo));
           hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
           hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
           hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
           hints.ai_protocol = 0;          /* Any protocol */
           hints.ai_canonname = NULL;
           hints.ai_addr = NULL;
           hints.ai_next = NULL;

           s = getaddrinfo(NULL, argv[1], &hints, &result);
           if (s != 0) {
               fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
               exit(EXIT_FAILURE);
           }

           /* getaddrinfo() returns a list of address structures.
              Try each address until we successfully bind(2).
              If socket(2) (or bind(2)) fails, we (close the socket
              and) try the next address. */
           for (rp = result; rp != NULL; rp = rp->ai_next) {
               sfd = socket(rp->ai_family, rp->ai_socktype,
                       rp->ai_protocol);
               if (sfd == -1)
                   continue;

               if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
                   break;                  /* Success */

               close(sfd);
           }

           if (rp == NULL) {               /* No address succeeded */
               fprintf(stderr, "Could not bind\n");
               exit(EXIT_FAILURE);
           }

           freeaddrinfo(result);           /* No longer needed */

	   queue_flush_cond();
	   
           for (;;) {
	     memset(&rw_struct, 0, sizeof(rw_struct));
               peer_addr_len = sizeof(struct sockaddr_storage);
               nread = recvfrom(sfd, &rw_struct, sizeof(rw_struct), 0,
                       (struct sockaddr *) &peer_addr, &peer_addr_len);
               if (nread == -1)
                   continue;               /* Ignore failed request */

               char host[NI_MAXHOST], service[NI_MAXSERV];
//#define VERBOSE1
#ifdef VERBOSE1
	       dump(nread, (uint8_t*)&rw_struct);
#else
	       //	       printf("in=%ld: ", nread);
#endif	       
	       s = getnameinfo((struct sockaddr *) &peer_addr,
                               peer_addr_len, host, NI_MAXHOST,
                               service, NI_MAXSERV, NI_NUMERICSERV);
	       if (s == 0) switch(rw_struct.cmd)
		{
		case 'w':
		  {
		    queue_flush_cond();
		    log_edcl_write(rw_struct.addr, rw_struct.bytes, rw_struct.iobuf);
#ifdef VERBOSE1
		    printf("\nedcl_write(%lx, %x, ..\n", rw_struct.addr, rw_struct.bytes);
		    dump(rw_struct.bytes, rw_struct.iobuf);
#else		    
		    //	       printf("out=%d\n", 0);
#endif
		    if (sendto(sfd, rw_struct.iobuf, 0, 0,
			       (struct sockaddr *) &peer_addr,
			       peer_addr_len) != 0)
		      fprintf(stderr, "Error sending response\n");
		    break;
		  }
		case 'r':
		  {
		    queue_flush_cond();
		    log_edcl_read(rw_struct.addr, rw_struct.bytes, rw_struct.iobuf);
#ifdef VERBOSE1
		    printf("\nedcl_read(%lx, %x, ...\n",
			   rw_struct.addr, rw_struct.bytes);
		    dump(rw_struct.bytes, rw_struct.iobuf);
#else
		    //		    printf("out=%d\n", rw_struct.bytes);
#endif		    
		    if (sendto(sfd, rw_struct.iobuf, rw_struct.bytes, 0,
			       (struct sockaddr *) &peer_addr,
			       peer_addr_len) != rw_struct.bytes)
		      fprintf(stderr, "Error sending response\n");
		    break;
		  }
		case 'W':
		  {
		    uint64_t ptr;
		    uint32_t data;
		    int flush;
		    if (3==sscanf(((char *)&rw_struct)+1, "%lX,%X,%X", &ptr, &data, &flush))
		      {
			volatile uint32_t * const base = (volatile uint32_t*)ptr;
			queue_write(base, data, flush);
			printf("\nqueue_write(%p, %x, %x);\n", base, data, flush);
			if (sendto(sfd, &data, 0, 0,
                           (struct sockaddr *) &peer_addr,
                           peer_addr_len) != 0)
			  fprintf(stderr, "Error sending response\n");
		      }
		    else
		      printf("Received %zd bytes from %s:%s\n", nread, host, service);
		    break;
		  }
		case 'R':
		  {
		    uint64_t ptr;
		    uint32_t data;
		    int flush;
		    if (1==sscanf(((char *)&rw_struct)+1, "%lX", &ptr))
		      {
			volatile uint32_t * const base = (volatile uint32_t*)ptr;
			data = queue_read(base);
			printf("\nqueue_read(%p) => %x;\n", base, data);
			if (sendto(sfd, &data, sizeof(data), 0,
                           (struct sockaddr *) &peer_addr,
                           peer_addr_len) != sizeof(data))
			  fprintf(stderr, "Error sending response\n");
		      }
		    else
		      printf("\nReceived %zd bytes from %s:%s\n", nread, host, service);
		    break;
		  }
		default:
		      printf("\nReceived %zd bytes from %s:%s\n", nread, host, service);
		    break;
		}
               else
                   fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
           }
       }
