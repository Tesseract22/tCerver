#include <csignal>




#include <Socket.hpp>
#define TRUE 1
#define FALSE 0
#define PORT 80




int main(int argc, char *argv[]) {
  
  // signal(SIGINT, sigintHandler);
  Socket* s = new Socket;
  free(s);
  s->startListen();
}