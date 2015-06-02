/*
 * proxy.c - CS:APP Web proxy
 *
 * Student ID: 2013-11419 
 *         Name: Suhyun Lee
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csapp.h"

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"

/* Undefine this if you don't want debugging output */
#define DEBUG

/*
 * Functions to define
 */
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);

void openserver(int port) {
	int listenfd, connfd;
	struct sockaddr_in server, client;
	int clientlen;
	char *msg_send[100], *msg_recv[100];
	
	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		printf("Failed to create server socket.\n");
		exit(1);
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(listenfd, (struct sockaddr*)&server, sizeof(server)) == -1) {
		printf("Failed to bind.\n");
		exit(1);
	}

	if (listen(listenfd, 1) == -1) { // FIXME modify the connection number
		printf("Failed to listen.\n");
		exit(1);
	}	

	while (1) {
		clientlen = sizeof(client);
		if ((connfd = accept(listenfd,
						(struct sockaddr*)&client, &clientlen)) == -1) {
			printf("Failed to accept.\n");
			exit(1);
		}
		
		if (read(connfd, msg_recv, 100) == -1) {
			printf("Failed to receive.\n");
			exit(1);
		}
		printf("recv: %s", msg_recv);
		
		if (close(connfd) == -1) {
			printf("Failed to close.\n");
			exit(1);
		}
	}
}


/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
	FILE* fp;

	/* open a file for logging */
	fp = fopen("./proxy.log", "w+");

	/* Check arguments */
	if (argc != 2) {
			fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
			exit(0);
	}

	openserver(atoi(argv[1]));
		
	/* close log file */
	fclose(fp);
	return 0;
}


