/*
** client.c -- a stream socket client demo
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <poll.h>

#ifndef countof
# define countof(x) (sizeof(x)/sizeof(x[0]))
#endif

// get sockaddr, IPv4 or IPv6:
void * get_in_addr (struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}


int ezsocket (char const *name, char const *port, int socktype, int timeout_ms)
{
	struct pollfd socks[20];
	int i, rv, sock, spent = 0, failed = 0, count = 0;
	struct timeval now, start;
	char s[INET6_ADDRSTRLEN];
	struct addrinfo hints, *p;
	struct gaicb sgaicb;
	struct gaicb * gaicb[1];

	memset (&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socktype;

	gaicb[0] = &sgaicb;
	gaicb[0]->ar_name = name;
	gaicb[0]->ar_service = port;
	gaicb[0]->ar_request = &hints;
	gaicb[0]->ar_result = 0;

	if ((rv = getaddrinfo_a (GAI_WAIT, gaicb, 1, 0)) != 0) {
		fprintf (stderr, "getaddrinfo_a: %s\n", gai_strerror (rv));
		return -1;
	}

	// loop through all the results and connect to the first we can
	memset (socks, 0, sizeof(socks));
	for (p = gaicb[0]->ar_result; p != NULL && count < countof(socks); p = p->ai_next) {

		sock = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock == -1) {
			perror ("client: socket");
			continue;
		}

		/* Set non-blocking on this socket so we can poll it. */
		fcntl (sock, F_SETFL, fcntl (sock, F_GETFL, 0) | O_NONBLOCK);
		inet_ntop (p->ai_family, get_in_addr ((struct sockaddr *) p->ai_addr), s, sizeof s);
		printf ("client: connecting to %s...\n", s);

		if (-1 == connect (sock, p->ai_addr, p->ai_addrlen) && EINPROGRESS != errno) {
			perror ("client: connect");
			close(sock);
			continue;
		}
		fcntl (sock, F_SETFL, fcntl (sock, F_GETFL, 0) & ~O_NONBLOCK);
		socks[count].fd = sock;
		socks[count].events = POLLOUT;
		socks[count].revents = 0;
		++count;
	}

	if (gaicb[0]->ar_result);
		freeaddrinfo (gaicb[0]->ar_result);

	gettimeofday(&start, 0);
	sock = -1;
	if(count) do {
		rv = poll(socks, count, timeout_ms - spent);

		if(rv < 0){
			perror ("client: poll");
			goto DONE;
		}

		for(i = 0; i < count; ++i){
			if(socks[i].revents & ~POLLOUT){
				close(socks[i].fd);
				socks[i].fd = -1;
				socks[i].events = 0;
				socks[i].revents = 0;
				printf("connection %d failed.\n", i);
				++failed;
			}
			if(socks[i].revents & POLLOUT){
				sock = socks[i].fd;
				printf("connection %d connected.\n", i);
				goto DONE;
			}
		}

		gettimeofday(&now, 0);
		now.tv_sec -= 1;
		now.tv_usec += 1000000;
		spent = 1000 * (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1000;

	} while(failed < count && timeout_ms - spent > 0);

DONE:
	gettimeofday(&now, 0);
	now.tv_sec -= 1;
	now.tv_usec += 1000000;
	spent = 1000 * (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1000;

	printf("results:\n");
	for(i = 0; i < count; ++i){
		if(socks[i].fd == -1){
			printf("\tconnection %d failed.\n", i);
		}
		else if(socks[i].fd == sock){
			printf("\tconnection %d connected.\n", i);
		}
		else{
			close(socks[i].fd);
			printf("\tconnection %d closed.\n", i);
		}
	}
	printf("wait time: %gs\n", spent / 1000.0);

	return sock;

}

#define PORT "80"		// the port client will be connecting to
#define MAXDATASIZE 1000	// max number of bytes we can get at once

int main (int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	int rv;

	if (argc < 2) {
		fprintf (stderr, "usage: client hostname [timeout_ms]\n");
		exit (1);
	}
	sockfd = ezsocket (argv[1], PORT, SOCK_STREAM, argc >= 3 ? atoi(argv[2]) : 1000);

	if (sockfd == -1) {
		fprintf (stderr, "client: failed to connect\n");
		return 2;
	}

	shutdown(sockfd, SHUT_WR);

	if ((numbytes = recv (sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
		perror ("recv");
		exit (1);
	}

	buf[numbytes] = '\0';

	printf ("client: received '%s'\n", buf);

	close (sockfd);

	return 0;
}
