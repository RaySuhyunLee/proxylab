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
#include <time.h>

#include "csapp.h"

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"

/* Undefine this if you don't want debugging output */
//#define DEBUG

struct connection_info {
	int fd;
	struct sockaddr_in addr;
};

static sem_t key; /* used in open_clientfd_ts() (for gethostbyname) */
static sem_t logkey; /* used in process_request() (for logging) */
static FILE *logp;


/*
 * Functions to define
 */
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);

int sendtoserver(char* host, int port, char* msg_send, char* msg_recv, size_t buffersize);
void begin(int port);
int parse_line(char* input, char* output, size_t buffersize);

void get_time_str(char* buf, int size) {
	time_t t;
	char* str;
	char* cut[5];
	t = time(NULL);
	str = strtok(ctime(&t), "\n");
	cut[0] = strtok(str, " ");
	cut[1] = strtok(NULL, " ");
	cut[2] = strtok(NULL, " ");
	cut[3] = strtok(NULL, " ");
	cut[4] = strtok(NULL, " ");
	snprintf(buf, size, "%s %s %s %s %s %s",
			cut[0], cut[2], cut[1], cut[4], cut[3], tzname[0]);
}

void* process_request(void* vargp) {
	struct connection_info cinfo;
  ssize_t readlen, writelen;
	int SEND_BUFFER_SIZE = 100, RECV_BUFFER_SIZE = 100;
	char msg_send[SEND_BUFFER_SIZE], msg_recv[RECV_BUFFER_SIZE];
	rio_t rp;
	const char* usagestr = "proxy usage: <host> <port> <message>\n";
	char timebuf[50];

	cinfo = *(struct connection_info*)vargp;
	free(vargp);

	Rio_readinitb(&rp, cinfo.fd);
	
	while(1) {
		readlen = Rio_readlineb_w(&rp, msg_recv, RECV_BUFFER_SIZE-1);
		if (readlen == 0)
			break;

		msg_recv[readlen] = '\0';
#ifdef DEBUG
		printf("thread %u | len: %lu, recv: %s", pthread_self(), readlen, msg_recv);
		fflush(stdout);
#endif

		if((writelen = parse_line(msg_recv, msg_send, SEND_BUFFER_SIZE-1)) >= 0) {
			/* lock to print log */
			sem_wait(&logkey);
			get_time_str(timebuf, sizeof(timebuf));
			fprintf(logp, "%s: %s %d %d %s", 
					timebuf,
					inet_ntoa(cinfo.addr.sin_addr), /* inet_ntoa: thread unsafe */
					htons(cinfo.addr.sin_port), writelen, msg_send);
			fflush(logp);
			/* unlock */
			sem_post(&logkey);
			Rio_writen_w(cinfo.fd, msg_send, writelen);
		} else {
			Rio_writen_w(cinfo.fd, (void*)usagestr, strlen(usagestr));
		}
	}
	
	/* close */
	Close(cinfo.fd);
#ifdef DEBUG
	printf("thread %u | I'm done. Bye~\n", pthread_self());
#endif

	return NULL;
}

int open_clientfd_ts(char *hostname, int port, sem_t *mutexp) {
	int clientfd;
	struct hostent *hep;
	struct sockaddr_in server;

	if ((clientfd = Socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	
	bzero((char*)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	/* get the address */
	sem_wait(mutexp);
	hep = gethostbyname(hostname); /* thread unsafe */
	if (hep == NULL) {
		bcopy(hostname,
				(char*)&server.sin_addr.s_addr,
				strlen(hostname));
	} else {
		bcopy((char*)hep->h_addr_list[0],
				(char*)&server.sin_addr.s_addr,
				hep->h_length);
	}
	sem_post(mutexp);

	if (connect(clientfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
		Close(clientfd);
		return -1;
	}
	return clientfd;
}

ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes) {
	ssize_t recvlen;
	if ((recvlen = rio_readn(fd, ptr, nbytes)) == -1) {
		printf("Rio_readn error\n");
		return 0;
	}
	return recvlen;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) {
	ssize_t readcnt;

	if ((readcnt = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
		printf("Rio_readlineb error\n");
		return 0;
	}
	return readcnt;
}

void Rio_writen_w(int fd, void *usrbuf, size_t n) {
	if (rio_writen(fd, usrbuf, n) == -1) {
		printf("Rio_writen error\n");
	}
}

/** parse_line()
 * parses a line of string and sends a proper request to
 * real server.
 */
int parse_line(char* input, char* output, size_t buffersize) {
	char* parsed[3] = {NULL, NULL, NULL};
	int host_max = 30;
	char host[host_max+1];
	int port;
	int cnt=0, i, len;
	len = strlen(input);
	parsed[cnt++] = input;

	/* parse the input */
	for(i = 0; i < len - 1 && cnt < 3; i++) {
		if (input[i] == ' ') {
			if (cnt == 1) {
				strncpy(host, parsed[0], i<host_max? i : host_max);
				host[i<host_max? i:host_max] = '\0';
			}
			parsed[cnt++] = input+i+1;
		}
	}
	
	if ((parsed[2] == NULL)  /* arguments less than 3 */
			|| (port = atoi(parsed[1])) == 0) { /* second argument is not a number */
		return -1;
	} else {
		return sendtoserver(host, port, parsed[2], output, buffersize);
	}
}

/** sendtoserver()
 * Opens a new client that connects to the real server.
 * Sends request to the server and get response.
 */
int sendtoserver(char* host, int port, char* msg_send, char* msg_recv, size_t buffersize) {
	int sockfd;
	rio_t rp;
	ssize_t recvlen;

	/* connect to the server */
	if ((sockfd = open_clientfd_ts(host, port, &key)) < 0) {
		printf("failed to open client(errno: %d)\n", errno);
		return 0;
	}
#ifdef DEBUG
	printf("opened socket to server: %d\n", sockfd);
#endif

	/* initialize rio_t */
	Rio_readinitb(&rp, sockfd);

  /* write to real server */
	Rio_writen_w(sockfd, msg_send, strlen(msg_send));
#ifdef DEBUG
	printf("sent %lu bytes to server\n", strlen(msg_send));
#endif

	/* read from real server */
	recvlen = Rio_readlineb_w(&rp, msg_recv, buffersize);
#ifdef DEBUG
	printf("received %zd bytes from server\n", recvlen);
#endif
	/* close */
	Close(sockfd);
#ifdef DEBUG
	printf("closed socket %d to server\n", sockfd);
#endif
	return recvlen;
}

void begin(int port) {
	int listenfd;
	struct connection_info *cinfop;
	struct sockaddr_in server;
	int clientlen;
	pthread_t pt;
	
	/* get a socket */
	listenfd = Socket(PF_INET, SOCK_STREAM, 0);

	/* configure proxy server settings */
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	/* bind */
	Bind(listenfd, (struct sockaddr*)&server, sizeof(server));

	/* listen to the connection from the client */
	Listen(listenfd, 1); // FIXME modify the connection number

	while (1) {
		clientlen = sizeof(cinfop->addr);
		cinfop = malloc(sizeof(struct connection_info));
		cinfop->fd = Accept(listenfd,
						(struct sockaddr*)&(cinfop->addr), &clientlen);
		/* create a new thread */
		pthread_create(&pt, NULL, process_request, cinfop);
		pthread_detach(pt);
	}

	Close(listenfd);
}


/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
	/* open a file for logging */
	logp = fopen("./proxy.log", "w+");
	if (!logp)
		perror("fopen failed.\n");

	/* Check arguments */
	if (argc != 2) {
			fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
			exit(0);
	}

	/* init semaphore */
	sem_init(&key, 0, 1);
	sem_init(&logkey, 0, 1);
	begin(atoi(argv[1]));
		
	/* close log file */
	fclose(logp);
	return 0;
}
