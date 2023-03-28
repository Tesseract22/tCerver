#include <csignal>

#include <HTTPServer.hpp>

#define TRUE 1
#define FALSE 0
#define PORT 80

int main(int argc, char *argv[]) {

    // signal(SIGINT, sigintHandler);
    HTTPServer s(1, 1);
}