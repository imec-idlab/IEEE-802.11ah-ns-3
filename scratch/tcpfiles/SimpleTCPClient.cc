#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define MAXDATASIZE 4096 // max number of bytes we can get at once

#include "SimpleTCPClient.h"


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}



int stat_connect(const char* hostname, const char* port) {
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect: \n");
		return -1;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), s,
			sizeof s);

	freeaddrinfo(servinfo); // all done with this structure

	return sockfd;
}

bool stat_send(int sockfd, const char* buf) {
	int length = strlen(buf);
	if(length < MAXDATASIZE) {
		int bytesSent = send(sockfd, buf, length, 0);
		if(bytesSent < 0) fprintf(stderr, "socket send failed: %m\n");
		return bytesSent != -1;
	}
	else {
		int pos = 0;
		while(pos < length) {
			int size = pos +  MAXDATASIZE - 1 < length ?  MAXDATASIZE - 1 : (length - pos-1);
			int bytesSent = send(sockfd, buf,size, 0);
			if(bytesSent == -1)
				return false;

			pos+=bytesSent;
		}
	}
}

void stat_close(int sockfd) {
	::close(sockfd);
}
