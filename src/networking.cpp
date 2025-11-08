#ifdef _WIN32
#include <Ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#include "networking.h"
using namespace std;

void set_nonblocking(int sock){
#ifdef _WIN32
	u_long yes = 1;
	ioctlsocket(sock, FIONBIO, &yes);
#else
	fcntl(sock, F_SETFL, O_NONBLOCK); 
#endif
}

void socket_error(const char *msg){
#ifdef _WIN32
	char buf[1024];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), 0, buf, sizeof(buf), NULL);
	printf ("%s: %s", msg, buf);
#else
	perror(msg);
#endif
}

int init_server() {
	struct sockaddr_in serveraddr;
	int request_sd;
	//int yes = 1;
	if ((request_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		socket_error("socket()");
		return -1;
	}
	
	/*
	if (setsockopt(request_sd, SOL_SOCKET, SO_REUSEADDR, &yes,
		sizeof(int)) < 1) { 
		socket_error("setsockopt()");
		return -1;
	}*/


	memset(&serveraddr, 0, sizeof(sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(HOSTPORT);

	if (bind(request_sd, (struct sockaddr *) &serveraddr, 
		sizeof (struct sockaddr_in)) < 0) { 
		socket_error("bind()");
		return -1;
	}

	if (listen(request_sd, SOMAXCONN) < 0) {
		socket_error("listen()");
		return -1;
	}

	return request_sd;
}

int accept_or_die(int request_sd) {
	struct sockaddr_in addr;
	socklen_t addrsize = sizeof (sockaddr_in);
	int res = accept (request_sd, (struct sockaddr*) &addr, &addrsize);
	if (res < 0) {
		socket_error("accept()");
		exit (EXIT_FAILURE);
	}
	set_nonblocking(res);
	return res;
}

int init_client(const char *hostname) {
	int sd;
	sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in serveraddr;
	
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, hostname, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(HOSTPORT);
	
	if (connect(sd, (struct sockaddr *) &serveraddr, sizeof(struct sockaddr_in)) < 0) {
		socket_error("connect()");
		return -1;
	}
	
	// Lesson learn: set_nonblocking shall not be called before the connect call.
	set_nonblocking(sd);
	return sd;
}

void get_command(int sfd, sPlayerAction *action) {
	char buf[1024];
	int res = recv(sfd, buf, 32, 0);
	if (res > 0){
		int * buf_ptr = (int *)&buf[0];
		action->command = (Command) ntohl(*buf_ptr++);
		int size = ntohl(*buf_ptr++);
		int x, y, c;
		for (int i = 0; i < size; i++) {
			x = ntohl(*buf_ptr++);
			y = ntohl(*buf_ptr++);
			c = ntohl(*buf_ptr++);
			action->moves.emplace_back(x, y, c);
		}
	} else {
		action->command = CmdNothing;
	}
}

void send_command(int sfd, const sPlayerAction& action){
	char buf[1024];
	int * buf_ptr = (int *)&buf[0];
	*buf_ptr++ = htonl((int)action.command);
	*buf_ptr++ = htonl((int)action.moves.size());
	for (const Move & move : action.moves) {
		*buf_ptr++ = htonl(move.x);
		*buf_ptr++ = htonl(move.y);
		*buf_ptr++ = htonl(move.stoneColour);
	}
	int res = send(sfd, buf, 32, 0);
	if (res < 0) {
		socket_error("send_command()");
	}
}
