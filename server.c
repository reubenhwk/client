/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 80  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

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
all_sock (unsigned short port)
{
        struct sockaddr_in6 saddr;
        int sock = socket (AF_INET6, SOCK_STREAM, 0);
        int const one = 1;

        if (sock < 0) {
                exit (1);
        }

        memset (&saddr, 0, sizeof (saddr));
        saddr.sin6_port = htons (port);

        setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (one));

        bind (sock, (struct sockaddr *) &saddr, sizeof (saddr));
        listen (sock, 15);

        return sock;
}



int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    char s[INET6_ADDRSTRLEN];

	sockfd = all_sock(PORT);

	if(sockfd < 0){
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
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
                struct linger linger = {1, 1};
		char buffer[4000];

        	close(sockfd); // child doesn't need the listener

		read(new_fd, buffer, sizeof(buffer));
                shutdown(new_fd, SHUT_RD);
                setsockopt(new_fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));

		sprintf(buffer, "Hi there %s!", s);
		if (send(new_fd, buffer, strlen(buffer), 0) == -1)
                	perror("send");

		shutdown(new_fd, SHUT_WR);
		close(new_fd);
		exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

