#include <arpa/inet.h>  //close
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <sys/socket.h>
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h>  //close

#include <Socket.hpp>
#define TRUE 1
#define FALSE 0
#define PORT 80

FILE *log_file;

void sigintHandler(int dummpy) { Socket::cleanUp(); }

int initMasterSocket() {
  int master_socket;
  if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    fprintf(stderr, "master socket creation failed");
  return master_socket;
}

int initClientSockets();

int sets();

int main(int argc, char *argv[]) {
  signal(SIGINT, sigintHandler);
  Socket s;
  s.ready();
}