#ifndef _NETWORKING_H
#define _NETWORKING_H

#include "game.h"

#define HOSTPORT 1337

int init_client(const char *hostname);
int init_server(void);
int accept_or_die(int rq_sock);

void get_command(int sfd, sPlayerAction *action);
void send_command(int sfd, const sPlayerAction &action);

#endif
