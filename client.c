/*
** client.c -- a stream socket client demo
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PORT "80"		// the port client will be connecting to
#define MAXDATASIZE (16*1024)	// max number of bytes we can get at once

static int ezsocket(char const * addrstr, char const * port)
{
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == sock) {
		return -1;
	}

	struct sockaddr_in remote = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = inet_addr(addrstr),
		.sin_port = htons(atoi(port)),
	};

	struct timespec start = {};
	clock_gettime(CLOCK_MONOTONIC, &start);
	int rc = connect(sock, (struct sockaddr*)&remote, sizeof(remote));
	if (-1 == rc) {
		goto cleanup;
	}
	struct timespec end = {};
	clock_gettime(CLOCK_MONOTONIC, &end);

	struct timespec total = {
		end.tv_sec - start.tv_sec -1,
		end.tv_nsec - start.tv_nsec + 1000000000,
	};
	if (total.tv_nsec >= 1000000000) {
		total.tv_sec += 1;
		total.tv_nsec -= 1000000000;
	}
	double millis = total.tv_sec * 1000 + total.tv_nsec / 1000000.0;

	printf("%u.%09u\n", (unsigned)total.tv_sec, (unsigned)total.tv_nsec);
	printf("%.3f ms\n", millis);

	return sock;
cleanup:
	if (sock)
		close(sock);

	return -1;
}

int main (int argc, char *argv[])
{
	int numbytes;
	char buf[MAXDATASIZE];

	if (argc < 2) {
		fprintf (stderr, "usage: client addr\n");
		exit (1);
	}
	int sockfd = ezsocket (argv[1], PORT);

	if (sockfd == -1) {
		fprintf (stderr, "client: failed to connect\n");
		return 2;
	}

	struct timespec start = {};
	clock_gettime(CLOCK_MONOTONIC, &start);
	char const * request = "GET / HTTP/1.1\n\n";
	write(sockfd, request, strlen(request));

	shutdown(sockfd, SHUT_WR);

	if ((numbytes = recv (sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
		perror ("recv");
		exit (1);
	}

	buf[numbytes] = '\0';
	struct timespec end = {};
	clock_gettime(CLOCK_MONOTONIC, &end);

	printf ("client: received '%s'\n", buf);
	struct timespec total = {
		end.tv_sec - start.tv_sec -1,
		end.tv_nsec - start.tv_nsec + 1000000000,
	};
	if (total.tv_nsec >= 1000000000) {
		total.tv_sec += 1;
		total.tv_nsec -= 1000000000;
	}
	double millis = total.tv_sec * 1000 + total.tv_nsec / 1000000.0;

	printf("%u.%09u\n", (unsigned)total.tv_sec, (unsigned)total.tv_nsec);
	printf("%.3f ms\n", millis);


	close (sockfd);

	return 0;
}
