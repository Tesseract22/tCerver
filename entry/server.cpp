#include <TCPServer.hpp>
#include <csignal>

#define TRUE 1
#define FALSE 0
#define PORT 80

int main(int argc, char *argv[]) {
    TCPServer server(1, 1);
    server.startListen();
    // signal(SIGINT, sigintHandler);
}