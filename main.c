#include "automat.h"
#include "klient.h"

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
	int do_automatu[2];
	int od_automatu[2];

	if (pipe(do_automatu) != 0 || pipe(od_automatu) != 0) {
		perror("pipe");
		return 1;
	}

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return 1;
	}

	if (pid == 0) {
		close(do_automatu[1]);
		close(od_automatu[0]);
		automat_run(do_automatu[0], od_automatu[1]);
		close(do_automatu[0]);
		close(od_automatu[1]);
		_exit(0);
	}

	close(do_automatu[0]);
	close(od_automatu[1]);

	int kod = klient_run(do_automatu[1], od_automatu[0], pid);

	close(do_automatu[1]);
	close(od_automatu[0]);
	(void)waitpid(pid, NULL, 0);

	if (kod != 0) {
		fprintf(stderr, "blad klienta lub automatu\n");
		return 1;
	}
	puts("do widzenia!!");
	return 0;
}
