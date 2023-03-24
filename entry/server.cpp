#include <csignal>

#include <Socket.hpp>
#include <TCPServer.hpp>
#define TRUE 1
#define FALSE 0
#define PORT 80

int main(int argc, char *argv[]) {

    // signal(SIGINT, sigintHandler);
    TCPServer server;
    server.startListen();
}