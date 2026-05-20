#ifndef KLIENT_H
#define KLIENT_H

#include <sys/types.h>

/* Interfejs ncurses + komunikacja z procesem automatu. Zwraca 0 OK. */
int klient_run(int fd_do_automatu, int fd_od_automatu, pid_t pid_automatu);

#endif
