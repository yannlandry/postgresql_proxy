/*************************************************************************
 *
 * pg_proxy.c - PostgreSQL proxy
 *
 * by Robert (bob) Edwards, April 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pg_proxy.h"

#define OPT_STR "h:l:p:"

void usage (char *prog, char *msg) {
	fprintf (stderr,
		"usage: %s [-l localport] [-h server] [-p server port]\n", prog);
}

int main (int argc, char *argv[]) {
	char c;
	int localport = PG_DEF_PORT;
	int remoteport = PG_DEF_PORT;
	char *remotehost = DEFAULT_HOST;

	while ((c = getopt (argc, argv, OPT_STR)) != EOF) {
		switch (c) {
			case 'h' : remotehost = optarg;
				break;
			case 'l' : localport = atoi (optarg);
				break;
			case 'p' : remoteport = atoi (optarg);
				break;
			default : usage (argv[0], "");
				return -1;
		}
	}

	printf ("%s - listening on TCP port %d and will connect to %s:%d\n",
		argv[0], localport, remotehost, remoteport);
	return 1;
}
