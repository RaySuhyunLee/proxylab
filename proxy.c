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

int parse_line(char* input, char* output) {
	char* parsed[3] = {NULL, NULL, NULL};
	int host_max = 30;
	char host[host_max+1];
	int port;
	int cnt=0, i, len;
	char usage[] = "proxy usage: <host> <port> <message>\n";
	len = strlen(input);
	parsed[cnt++] = input;
	for(i = 0; i < len - 1 && cnt < 3; i++) {
		if (input[i] == ' ') {
			if (cnt == 1) {
				strncpy(host, parsed[0], i<host_max? i : host_max);
				host[i<host_max? i:host_max] = '\0';
			}
			parsed[cnt++] = input+i+1;
		}
	}
	
	if ((host == NULL || parsed[1] == NULL || parsed[2] == NULL)
			|| (port = atoi(parsed[1])) == 0) {
		strncpy(output, usage, strlen(usage));
		return strlen(usage);
	} else {
		return openclient(host, port, parsed[2], output);
	}
}

int openclient(char* host, int port, char* msg_send, char* msg_recv) {
	int sockfd;
	struct sockaddr_in server;
	int recvlen;

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("Failed to create client socket.\n");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(host);

	if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) == -1) {
		printf("Failed to connect to server.\n");
		exit(1);
	}

	if (write(sockfd, msg_send, strlen(msg_send)) == -1) {
		printf("Failed to write.\n");
		exit(1);
	}

	if ((recvlen = read(sockfd, msg_recv, strlen(msg_recv))) == -1) {
		printf("Failed to read.\n");
		exit(1);
	}

	if (close(sockfd) == -1) {
		printf("Failed to close.\n");
		exit(1);
	}
	return recvlen;
}

void openserver(int port) {
	int listenfd, connfd;
	struct sockaddr_in server, client;
	int clientlen, readlen, writelen;
	int SEND_BUFFER_SIZE = 100, RECV_BUFFER_SIZE = 100;
	char msg_send[SEND_BUFFER_SIZE], msg_recv[RECV_BUFFER_SIZE];
	
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
		
		while(1) {
			if ((readlen = read(connfd, msg_recv, RECV_BUFFER_SIZE-1)) == -1) {
				printf("Failed to receive.\n");
				exit(1);
			}
			if (readlen == 0)
				break;

			msg_recv[readlen] = NULL;
			printf("len: %d, recv: %s", readlen, msg_recv);
			fflush(stdout);

			if((writelen = parse_line(msg_recv, msg_send)) >= 0) {
				write(connfd, msg_send, writelen);
			}
		}
		
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
	char test[100];

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


