/*************************************************************************
 *
 * pg_proxy.c - PostgreSQL proxy
 *
 * by Robert (bob) Edwards, April 2016
 */

#include "pg_proxy.h"


int main (int argc, char *argv[]) {
	// for debug purposes
	setbuf(stdout, NULL);

	/* Arguments n stuff */
	int localport = PG_DEF_PORT;
	int remoteport = PG_DEF_PORT;
	char *remotehost = DEFAULT_HOST;
	parse_options(argc, argv, &localport, &remoteport, &remotehost);

	printf("%s - listening on TCP port %d and will some day connect to %s:%d\n",
		argv[0], localport, remotehost, remoteport);
	
	/* Make a few things */
	SOCKET initsock = make_initial_socket(localport);
	int num_clients = 0;
	int highest_fd = initsock; // for now
	
	Client clients[MAX_CLIENTS];
	
	fd_set sockset;
	
	while(1) {
		FD_ZERO(&sockset);
		
		FD_SET(initsock, &sockset);
		for(int i = 0; i < num_clients; ++i)
			FD_SET(clients[i].clientsock, &sockset);
		
		if(select(highest_fd+1, &sockset, NULL, NULL, NULL) < 0) {
			perror("Error at select");
			exit(-1);
		}
		
		// if it's our initial socket, add connection
		if(FD_ISSET(initsock, &sockset)) {
			if(add_client(initsock, clients, &num_clients, &highest_fd) == 0) {
				perror("Problem while accepting connection, skipping");
				continue;
			}
		}
		
		// otherwise some client is talking, find him, listen!
		else {
			for(int i = 0; i < num_clients; ++i) {
				if(FD_ISSET(clients[i].clientsock, &sockset)) {
					if(transfer_data(clients[i]) == 0) { // 0 -> client disconnected or error
						remove_client(clients, i, &num_clients);
					}
				}
			}
		}
	}
	
	return 0;
}


void parse_options(int argc, char* argv[], int* localport, int* remoteport, char** remotehost) {
	char c;

	while ((c = getopt (argc, argv, OPT_STR)) != EOF) {
		switch (c) {
			case 'h': *remotehost = optarg;
				break;
			case 'l': *localport = atoi (optarg);
				break;
			case 'p': *remoteport = atoi (optarg);
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
	memset((void*) &sin, 0, sizeof(sin));
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;
	
	if(bind(initsock, (SOCKADDR*) &sin, sizeof(sin)) < 0) {
		perror("Could not bind initial socket");
		exit(-1);
	}
	
	if(listen(initsock, MAX_CLIENTS) < 0) {
		perror("Error while listening to initial socket");
		exit(-1);
	}
	
	return initsock;
}


int add_client(int initsock, Client clients[], int* num_clients, int* highest_fd) {
	SOCKADDR_IN csin; // why do we even create this?
	memset((void*) &csin, 0, sizeof(csin));
	
	socklen_t sizeofcsin = sizeof(csin);
	SOCKET newcsock = accept(initsock, (SOCKADDR*) &csin, &sizeofcsin);
	if(newcsock < 0)
		return 0;
	
	// adjust max fd if required
	*highest_fd = newcsock > *highest_fd ? newcsock : *highest_fd;
	
	Client newc;
	newc.clientsock = newcsock;
	clients[(*num_clients)++] = newc;
	
	return 1;
}


int transfer_data(Client client) {
	char buffer[1024];
	
	int n = recv(client.clientsock, buffer, 1023, 0);
	if(n <= 0) {
		if(n != 0) perror("Error receiving from client");
		return 0;
	}
	
	buffer[n] = '\0';
	
	// THE PART WHERE WE TRANSMIT TO THE SERVER AND OTHER STUFF
	printf("We aren't transmitting yet, but here is the message: %s\n", buffer);
	
	return n;
}


void remove_client(Client clients[], int i, int* num_clients) {
	// close socket with server
	
	memmove(clients+i, clients+i+1, (*num_clients-i-1) * sizeof(Client));
	--(*num_clients);
}
