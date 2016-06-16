
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int
all_sock (char const * port)
{
	int sock = socket (AF_INET6, SOCK_STREAM, 0);

	if (sock < 0) {
		exit (1);
	}

	struct sockaddr_in6 saddr = {};
	saddr.sin6_port = htons(atoi(port));
	saddr.sin6_family = AF_INET6;

	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));

	if (0 != bind (sock, (struct sockaddr *)&saddr, sizeof(saddr))) {
		perror("bind");
		exit(-1);
	}

	listen (sock, 15);

	return sock;
}



int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;

	sockfd = all_sock(PORT);

	if (sockfd < 0) {
		perror("sockfd");
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof(their_addr);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			char buffer[4*1024];

			while (read(new_fd, buffer, sizeof(buffer)) > 0) {
				int rc = write(new_fd, (char[1]){0}, 1);
				if (1 != rc) {
					exit(1);
				}
			}

			close(new_fd);

			exit(0);
		}

		close(new_fd);  // parent doesn't need this
#if 0
		char s[INET6_ADDRSTRLEN];
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof(s));

		printf("server: got connection from %s\n", s);
#endif
	}

	return 0;
}

