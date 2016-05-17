/*************************************************************************
 *
 * pg_proxy.c - PostgreSQL proxy
 *
 * by Robert (bob) Edwards, April 2016
 */

#include "pg_proxy.h"


int dostuff() { printf("\nhello there\n"); return 1; }


int main (int argc, char *argv[]) {
	// no initial declarations in for loop blah blah blah
	int i;

	// for debug purposes
	setbuf(stdout, NULL);

	/* Arguments n stuff */
	int localport = PG_DEF_PORT;
	int remoteport = PG_DEF_PORT;
	char* remotehost = DEFAULT_HOST;
	parse_options(argc, argv, &localport, &remoteport, &remotehost);

	printf("%s - listening on TCP port %d and will connect to %s:%d\n",
		argv[0], localport, remotehost, remoteport);

	/* Make a few things regarding clients */
	ClientList clients;
	bzero((void*) &clients, sizeof(clients));

	SOCKET initsock = make_initial_socket(localport);

	fd_set sockset;
	int highest_fd = initsock; // for now

	/* Make a few things regarding the db server */
	ADDRINFO* serverinfo = find_remote_server(remotehost, remoteport);

	while(1) {
		FD_ZERO(&sockset);

		FD_SET(initsock, &sockset);
		for(i = 0; i < clients.count; ++i) {
			FD_SET(clients.list[i].clientsock, &sockset);
			FD_SET(clients.list[i].serversock, &sockset);
		}

		if(select(highest_fd+1, &sockset, NULL, NULL, NULL) < 0) {
			perror("Error at select");
			exit(-1);
		}

		// if it's our initial socket, add connection
		if(FD_ISSET(initsock, &sockset)) {
			if(add_client(initsock, serverinfo, &clients, &highest_fd) == 0) {
				perror("Problem while connecting client to server");
				continue;
			}
		}

		// otherwise find what's talking
		else {
			for(i = 0; i < clients.count; ++i) {
				if(FD_ISSET(clients.list[i].clientsock, &sockset)) {
					if(transfer_data(clients.list[i].clientsock, clients.list[i].serversock) == 0) {
						remove_client(&clients, i);
					}
				}
				else if(FD_ISSET(clients.list[i].serversock, &sockset)) {
					if(transfer_data(clients.list[i].serversock, clients.list[i].clientsock) == 0) {
						remove_client(&clients, i);
					}
				}
			}
		}
	}

	close(initsock);
	free(serverinfo);

	return 0;
}


void parse_options(int argc, char* argv[], int* localport, int* remoteport, char** remotehost) {
	char c;

	while ((c = getopt (argc, argv, OPT_STR)) != EOF) {
		switch (c) {
			case 'h': *remotehost = optarg;
				break;
			case 'l': *localport = atoi(optarg);
				break;
			case 'p': *remoteport = atoi(optarg);
				break;
			default: fprintf(stderr,"usage: %s [-l localport] [-h server] [-p server port]\n",argv[0]);
				exit(-1);
		}
	}
}


SOCKET make_initial_socket(int port) {
	SOCKET initsock = socket(AF_INET, SOCK_STREAM, 0);
	if(initsock < 0) {
		perror("Invalid initial socket created");
		exit(-1);
	}

	SOCKADDR_IN sin;
	bzero((void*) &sin, sizeof(sin));
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;

	if(bind(initsock, (SOCKADDR*) &sin, sizeof(sin)) < 0) {
		perror("Could not bind initial socket");
		exit(-1);
	}

	if(listen(initsock, 5) < 0) {
		perror("Error while listening to initial socket");
		exit(-1);
	}

	return initsock;
}


ADDRINFO* find_remote_server(char* remotehost, int remoteport) {
	ADDRINFO* ret;

	ADDRINFO hints;
	bzero((void*) &hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	char port[6]; // large enough to hold "65535\0"
	bzero(port, sizeof(port));
	sprintf(port, "%d", remoteport);

	int e;
	if( (e = getaddrinfo(remotehost, port, &hints, &ret)) != 0 ) {
		fprintf(stderr, "getaddrinfo: %s", gai_strerror(e));
		exit(-1);
	}

	return ret;
}


int add_client(int initsock, ADDRINFO* serverinfo, ClientList* clients, int* highest_fd) {
	SOCKADDR_IN csin; // why do we even create this?
	bzero((void*) &csin, sizeof(csin));
	socklen_t sizeofcsin = sizeof(csin);

	// create socket to client
	SOCKET newcsock = accept(initsock, (SOCKADDR*) &csin, &sizeofcsin);
	if(newcsock < 0)
		return 0;

	// setup socket to server for this client
	SOCKET newssock = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
	if(newssock < 0)
		return 0;
	if(connect(newssock, serverinfo->ai_addr, serverinfo->ai_addrlen) != 0)
		return 0;

	// adjust max fd if required
	*highest_fd = newcsock > *highest_fd ? newcsock : *highest_fd;
	*highest_fd = newssock > *highest_fd ? newssock : *highest_fd;

	// store everything
	// resize or create array?
	if(clients->count >= clients->space) {
		if(clients->space == 0)	clients->space = 16;
		else					clients->space*= 2;
		clients->list = (Client*)realloc(clients->list, clients->space * sizeof(Client));
	}

	// create client object
	clients->list[clients->count].clientsock = newcsock;
	clients->list[clients->count].serversock = newssock;
	clients->count+= 1;

	return 1;
}


int transfer_data(SOCKET source, SOCKET destination) {
	char buffer[BUFSIZE+1];

	int n = read(source, buffer, BUFSIZE);
	buffer[n] = '\0';
	printf("Transiting %i->%i (size=%i): %s\n", source, destination, n, buffer);

	if(write(destination, buffer, n) < 0) {
		perror("Error transmitting to destination");
		return 0;
	}

	// disconnection or error
	if(n <= 0) {
		if(n < 0) perror("Error in transmission");
		return 0;
	}

	return n;
}


void remove_client(ClientList* clients, int i) {
	// close socket with server
	close(clients->list[i].serversock);

	// close socket with client
	close(clients->list[i].clientsock);

	// delete client object
	memmove(clients->list+i, clients->list+i+1, (--clients->count-i) * sizeof(Client));

	// shrink array if needed
	if(clients->space / 4 > clients->count) {
		clients->space/= 2;
		clients->list = realloc(clients->list, clients->space);
	}
}
