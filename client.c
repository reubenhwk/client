
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

typedef struct timespec stopwatch_t;

/* Start a "timer" object */
stopwatch_t start_ms_timer(void)
{
	stopwatch_t t = {};
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t;
}

/* Get ellapsed time from a "timer" object */
double get_ms_time(stopwatch_t t)
{
	stopwatch_t now = start_ms_timer();

	struct timespec total = {
		now.tv_sec - t.tv_sec -1,
		now.tv_nsec - t.tv_nsec + 1000000000,
	};
	if (total.tv_nsec >= 1000000000) {
		total.tv_sec += 1;
		total.tv_nsec -= 1000000000;
	}

	double ms = total.tv_sec * 1000 + total.tv_nsec / 1000000.0;

	return ms;
}

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


	int rc = connect(sock, (struct sockaddr*)&remote, sizeof(remote));
	if (-1 == rc) {
		goto cleanup;
	}



	return sock;
cleanup:
	if (sock)
		close(sock);

	return -1;
}

int main (int argc, char *argv[])
{
	if (argc < 2) {
		fprintf (stderr, "usage: client addr1 addr2\n");
		exit (1);
	}

	double conn_times[2] = {};
	double rt_times[2] = {};
	for (int a = 0; a < 2; ++a) {
		stopwatch_t timer;
		double ms;

		int sockfd;
		int const MAX_CONS = 5;
		for (int i = 0; i < MAX_CONS; ++i) {
			/* measure connection time */
			timer = start_ms_timer();
			sockfd = ezsocket (argv[a+1], PORT);
			if (sockfd == -1) {
				fprintf (stderr, "client: failed to connect\n");
				exit(1);
			}
			ms = get_ms_time(timer);
			conn_times[a] += ms;

			if (i+1 < MAX_CONS) {
				close(sockfd);
			}
		}
		conn_times[a] /= (double)MAX_CONS;

		/* measure round trip time */
		int MAX_RTT = 30;
		for (int i = 0; i < MAX_RTT; ++i) {
			timer = start_ms_timer();
			if (1 != write(sockfd, (char[]){0}, 1)) {
				perror ("write");
				exit(1);
			}
			if (-1 == read(sockfd, (char[1]){}, 1)) {
				perror ("read");
				exit(1);
			}
			ms = get_ms_time(timer);
			rt_times[a] += ms;
		}
		rt_times[a] /= (double)MAX_RTT;

	}

	printf("% 4.3f, % 4.3f ms average connection time (% 4.3f ms diff)\n", conn_times[0], conn_times[1],
		conn_times[1] - conn_times[0]);
	printf("% 4.3f, % 4.3f ms average round trip time (% 4.3f ms diff)\n", rt_times[0], rt_times[1],
		rt_times[1] - rt_times[0]);

	return 0;
}
