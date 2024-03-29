/*
 * a template header file for pg_proxy.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define DEFAULT_HOST "dubhe.anu.edu.au"
#define PG_DEF_PORT 5432

#define BUFSIZE 2000

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
typedef struct addrinfo ADDRINFO;

typedef struct {
	SOCKET clientsock;
	SOCKET serversock;
} Client;

typedef struct {
	size_t count;
	size_t space;
	Client* list;
} ClientList;

#define OPT_STR "h:l:p:"
void parse_options(int argc, char* argv[], int* localport, int* remoteport, char** remotehost);

SOCKET make_initial_socket(int port);

ADDRINFO* find_remote_server(char* remotehost, int remoteport);

int add_client(int initsock, ADDRINFO* serverinfo, ClientList* clients, int* highest_fd);

int transfer_data(SOCKET source, SOCKET destination);

void remove_client(ClientList* clients, int i);
