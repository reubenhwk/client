/*
** client.c -- a stream socket client demo
*/

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

#define PORT "80"		// the port client will be connecting to

#define MAXDATASIZE 100		// max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *
get_in_addr (struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}


int
allconnect (char const *name, char const *port, int socktype, int timeout_ms)
{
	struct pollfd socks[100];
	int i, rv, sock, failed = 0, count = 0;
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];

	memset (socks, 0, sizeof(socks));
	memset (&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socktype;

	if ((rv = getaddrinfo (name, port, &hints, &servinfo)) != 0) {
		fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (rv));
		return -1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {

		sock = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock == -1) {
			perror ("client: socket");
			continue;
		}

		fcntl (sock, F_SETFL, fcntl (sock, F_GETFL, 0) | O_NONBLOCK);
		inet_ntop (p->ai_family, get_in_addr ((struct sockaddr *) p->ai_addr), s, sizeof s);
		printf ("client: connecting to %s...\n", s);

		connect (sock, p->ai_addr, p->ai_addrlen);
		socks[count].fd = sock;
		socks[count].events = POLLIN|POLLOUT;
		socks[count].revents = 0;
		++count;			
	}

	freeaddrinfo (servinfo);	// all done with this structure

	sock = -1;
	while(failed < count){
		rv = poll(socks, count, timeout_ms);
		fprintf(stderr, "poll returned %d.\n", rv);
		if(rv <= 0)
			goto DONE;

		for(i = 0; i < count; ++i){
			if(socks[i].revents == POLLHUP || socks[i].revents == POLLERR){
				socks[i].events = 0;
				socks[i].revents = 0;
				printf("socket %d failed.\n", i);
				++failed;
			}
			if(socks[i].revents & POLLOUT){
				sock = socks[i].fd;
				printf("connection %d connected first.\n", i);
				goto DONE;
			}
		}
	}
DONE:
	for(i = 0; i < count; ++i){
		if(socks[i].fd != sock && socks[i].fd != -1){
			close(socks[i].fd);
			printf("socket %d too late.\n", i);
		}
	}

	if(sock >= 0){
		fcntl (sock, F_SETFL, fcntl (sock, F_GETFL, 0) & ~O_NONBLOCK);
	}

	return sock;

}

int
main (int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	int rv;

	if (argc != 2) {
		fprintf (stderr, "usage: client hostname\n");
		exit (1);
	}
	sockfd = allconnect (argv[1], PORT, SOCK_STREAM, 1000);

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
